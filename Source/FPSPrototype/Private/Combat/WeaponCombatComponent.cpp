#include "Combat/WeaponCombatComponent.h"
#include "Combat/FPSCombatCharacter.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"

UWeaponCombatComponent::UWeaponCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UWeaponCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UWeaponCombatComponent, EquippedWeapon);
    DOREPLIFETIME(UWeaponCombatComponent, bReloading);
}

AFPSCombatCharacter* UWeaponCombatComponent::GetCombatCharacter() const
{
    return Cast<AFPSCombatCharacter>(GetOwner());
}

bool UWeaponCombatComponent::CanSendInput() const
{
    const auto* Character = GetCombatCharacter();
    return Character && (Character->HasAuthority() || Character->IsLocallyControlled());
}

bool UWeaponCombatComponent::CanUseWeapon() const
{
    const auto* Character = GetCombatCharacter();
    return Character && !Character->IsCombatDead() && IsValid(EquippedWeapon)
        && EquippedWeapon->GetOwner() == Character
        && EquippedWeapon->GetAttachParentActor() == Character
        && EquippedWeapon->GetWeaponDefinition();
}

bool UWeaponCombatComponent::SetEquippedWeapon(AWeaponBase* Weapon)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    if (Weapon && (!IsValid(Weapon) || Weapon->GetOwner() != GetOwner() || Weapon->GetAttachParentActor() != GetOwner())) return false;
    CancelCombat();
    EquippedWeapon = Weapon;
    GetOwner()->ForceNetUpdate();
    return true;
}

void UWeaponCombatComponent::StartFire()
{
    if (!CanSendInput()) return;
    if (GetOwner()->HasAuthority()) SetTriggerHeld(true);
    else ServerSetTriggerHeld(true);
}

void UWeaponCombatComponent::StopFire()
{
    if (!CanSendInput()) return;
    if (GetOwner()->HasAuthority()) SetTriggerHeld(false);
    else ServerSetTriggerHeld(false);
}

void UWeaponCombatComponent::ServerSetTriggerHeld_Implementation(bool bHeld)
{
    SetTriggerHeld(bHeld);
}

void UWeaponCombatComponent::SetTriggerHeld(bool bHeld)
{
    if (!GetOwner()->HasAuthority()) return;
    if (!bHeld)
    {
        bTriggerHeld = false;
        GetWorld()->GetTimerManager().ClearTimer(FireTimer);
        return;
    }
    if (bTriggerHeld || bReloading || !CanUseWeapon()) return;
    bTriggerHeld = true;
    const UWeaponDefinition* Definition = EquippedWeapon->GetWeaponDefinition();
    if (Definition->bAutomatic) FireAutomatic();
    else TryFireShot();
}

void UWeaponCombatComponent::FireAutomatic()
{
    if (!bTriggerHeld || bReloading || !CanUseWeapon() || EquippedWeapon->GetAmmoState().MagazineAmmo <= 0)
    {
        SetTriggerHeld(false);
        return;
    }
    const float RPM = EquippedWeapon->GetWeaponDefinition()->FireRateRPM;
    if (!FMath::IsFinite(RPM) || RPM <= 0.f) { SetTriggerHeld(false); return; }
    const bool bFired = TryFireShot();
    if (!bFired && GetWorld()->GetTimeSeconds() + 0.00001 >= NextAllowedFireTime)
    {
        SetTriggerHeld(false);
        return;
    }
    if (!bTriggerHeld || !CanUseWeapon()) return;
    const float Delay = FMath::Max(0.01f, static_cast<float>(NextAllowedFireTime - GetWorld()->GetTimeSeconds()));
    GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &ThisClass::FireAutomatic, Delay, false);
}

bool UWeaponCombatComponent::TryFireShot()
{
    AFPSCombatCharacter* Character = GetCombatCharacter();
    if (!Character || !Character->HasAuthority() || bReloading || !CanUseWeapon()) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now + 0.00001 < NextAllowedFireTime) return false;
    const UWeaponDefinition* Definition = EquippedWeapon->GetWeaponDefinition();
    if (!FMath::IsFinite(Definition->Damage) || Definition->Damage < 0.f
        || !FMath::IsFinite(Definition->MaxRange) || Definition->MaxRange <= 0.f
        || !FMath::IsFinite(Definition->FireRateRPM) || Definition->FireRateRPM <= 0.f) return false;

    // Origin, aim, range and damage are all derived from server-owned state.
    const FVector ViewStart = Character->GetPawnViewLocation();
    const FVector AimDirection = Character->GetBaseAimRotation().Vector();
    const FVector Muzzle = EquippedWeapon->GetMuzzleTransform().GetLocation();
    if (ViewStart.ContainsNaN() || AimDirection.ContainsNaN() || Muzzle.ContainsNaN()) return false;
    if (!EquippedWeapon->TryConsumeAmmo()) return false;
    NextAllowedFireTime = Now + FMath::Max(0.01f, 60.f / Definition->FireRateRPM);

    FCollisionQueryParams Query(SCENE_QUERY_STAT(FPSWeaponShot), false, Character);
    Query.AddIgnoredActor(EquippedWeapon);
    FHitResult ViewHit;
    const FVector RangeEnd = ViewStart + AimDirection * Definition->MaxRange;
    GetWorld()->LineTraceSingleByChannel(ViewHit, ViewStart, RangeEnd, ECC_Visibility, Query);
    const FVector AimPoint = ViewHit.bBlockingHit ? ViewHit.ImpactPoint : RangeEnd;

    FHitResult Hit;
    // Also guard a muzzle pushed through nearby cover. An off-centre muzzle must not shoot through walls.
    const bool bMuzzleBehindCover = GetWorld()->LineTraceSingleByChannel(Hit, ViewStart, Muzzle, ECC_Visibility, Query);
    if (!bMuzzleBehindCover)
    {
        if (ViewHit.bBlockingHit && FVector::DotProduct(AimPoint - Muzzle, AimDirection) <= 0.f)
            Hit = ViewHit;
        else if (!GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, AimPoint, ECC_Visibility, Query))
            Hit = ViewHit;
    }

    const FVector End = Hit.bBlockingHit ? Hit.ImpactPoint : AimPoint;
    if (AActor* Target = Hit.GetActor(); IsValid(Target) && Target != Character && Target->CanBeDamaged())
    {
        const float Applied = UGameplayStatics::ApplyPointDamage(Target, Definition->Damage,
            (End - Muzzle).GetSafeNormal(), Hit, Character->GetController(), EquippedWeapon, UDamageType::StaticClass());
        if (Applied > 0.f)
        {
            const auto* TargetCharacter = Cast<AFPSCombatCharacter>(Target);
            ClientConfirmHit(Applied, TargetCharacter && TargetCharacter->IsCombatDead());
        }
    }
    MulticastShot(Muzzle, End, Hit.bBlockingHit);
    return true;
}

void UWeaponCombatComponent::Reload()
{
    if (!CanSendInput()) return;
    if (GetOwner()->HasAuthority()) BeginReload();
    else ServerReload();
}

void UWeaponCombatComponent::ServerReload_Implementation() { BeginReload(); }

void UWeaponCombatComponent::BeginReload()
{
    if (!GetOwner()->HasAuthority() || bReloading || !CanUseWeapon() || !EquippedWeapon->CanReload()) return;
    const float Duration = EquippedWeapon->GetWeaponDefinition()->ReloadDuration;
    if (!FMath::IsFinite(Duration) || Duration <= 0.f) return;
    SetTriggerHeld(false);
    bReloading = true;
    ReloadingWeapon = EquippedWeapon;
    GetOwner()->ForceNetUpdate();
    GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &ThisClass::FinishReload, FMath::Max(0.01f, Duration), false);
}

void UWeaponCombatComponent::FinishReload()
{
    if (GetOwner()->HasAuthority() && bReloading && CanUseWeapon() && ReloadingWeapon.Get() == EquippedWeapon)
        EquippedWeapon->ReloadMagazine();
    bReloading = false;
    ReloadingWeapon.Reset();
    GetOwner()->ForceNetUpdate();
}

void UWeaponCombatComponent::CancelCombat()
{
    bTriggerHeld = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimer);
        World->GetTimerManager().ClearTimer(ReloadTimer);
    }
    if (GetOwner() && GetOwner()->HasAuthority()) bReloading = false;
    ReloadingWeapon.Reset();
}

void UWeaponCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelCombat();
    Super::EndPlay(EndPlayReason);
}

void UWeaponCombatComponent::MulticastShot_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End, bool bBlockingHit)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    OnShotFired.Broadcast(Start, End, bBlockingHit);
    if (bDrawShotTraces)
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.12f, 0, 0.6f);
        if (bBlockingHit) DrawDebugPoint(GetWorld(), End, 6.f, FColor::Red, false, 0.2f);
    }
}

void UWeaponCombatComponent::ClientConfirmHit_Implementation(float Damage, bool bKilled)
{
    LastConfirmedHitTime = GetWorld()->GetTimeSeconds();
    OnHitConfirmed.Broadcast(Damage, bKilled);
}
