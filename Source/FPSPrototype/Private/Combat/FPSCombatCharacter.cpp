#include "Combat/FPSCombatCharacter.h"
#include "Combat/FPSHealthSet.h"
#include "Combat/FPSDamageEffect.h"
#include "Combat/WeaponCombatComponent.h"
#include "Weapons/WeaponBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DamageEvents.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AFPSCombatCharacter::AFPSCombatCharacter()
{
    bReplicates = true;
    AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystem->SetIsReplicated(true);
    AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    HealthAttributes = CreateDefaultSubobject<UFPSHealthSet>(TEXT("HealthAttributes"));
    Combat = CreateDefaultSubobject<UWeaponCombatComponent>(TEXT("Combat"));
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

UAbilitySystemComponent* AFPSCombatCharacter::GetAbilitySystemComponent() const { return AbilitySystem; }
float AFPSCombatCharacter::GetHealth() const { return HealthAttributes->GetHealth(); }
float AFPSCombatCharacter::GetMaxHealth() const { return HealthAttributes->GetMaxHealth(); }
bool AFPSCombatCharacter::IsCombatDead() const { return GetHealth() <= 0.f; }

void AFPSCombatCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    AbilitySystem->AddAttributeSetSubobject(HealthAttributes.Get());
    AbilitySystem->InitAbilityActorInfo(this, this);
    AbilitySystem->GetGameplayAttributeValueChangeDelegate(UFPSHealthSet::GetHealthAttribute())
        .AddUObject(this, &ThisClass::HandleHealthChanged);
    AbilitySystem->GetGameplayAttributeValueChangeDelegate(UFPSHealthSet::GetMaxHealthAttribute())
        .AddUObject(this, &ThisClass::HandleHealthChanged);
}

void AFPSCombatCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    AbilitySystem->InitAbilityActorInfo(this, this);
}

void AFPSCombatCharacter::OnRep_Controller()
{
    Super::OnRep_Controller();
    AbilitySystem->InitAbilityActorInfo(this, this);
}

FVector AFPSCombatCharacter::GetPawnViewLocation() const
{
    return GetFirstPersonCameraComponent() ? GetFirstPersonCameraComponent()->GetComponentLocation() : Super::GetPawnViewLocation();
}

bool AFPSCombatCharacter::EquipCombatWeapon(AActor* Weapon)
{
    if (!HasAuthority()) return false;
    AWeaponBase* TypedWeapon = Cast<AWeaponBase>(Weapon);
    if (Weapon && !TypedWeapon) return false;
    return Combat->SetEquippedWeapon(TypedWeapon);
}

float AFPSCombatCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority() || IsCombatDead() || !CanBeDamaged() || !FMath::IsFinite(Damage) || Damage <= 0.f) return 0.f;
    const float Accepted = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
    if (!FMath::IsFinite(Accepted) || Accepted <= 0.f) return 0.f;
    UAbilitySystemComponent* Source = EventInstigator ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EventInstigator->GetPawn()) : nullptr;
    if (!Source) Source = AbilitySystem;
    FGameplayEffectContextHandle Context = Source->MakeEffectContext();
    Context.AddInstigator(EventInstigator ? EventInstigator->GetPawn() : DamageCauser, DamageCauser);
    Context.AddSourceObject(DamageCauser);
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
        Context.AddHitResult(static_cast<const FPointDamageEvent&>(DamageEvent).HitInfo);
    FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UFPSDamageEffect::StaticClass(), 1.f, Context);
    if (!Spec.IsValid()) return 0.f;
    Spec.Data->SetSetByCallerMagnitude(TAG_FPS_Data_Damage, Accepted);
    const float Before = GetHealth();
    Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), AbilitySystem);
    ForceNetUpdate();
    return FMath::Max(0.f, Before - GetHealth());
}

void AFPSCombatCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
    if (IsCombatDead() && !bDeathHandled)
    {
        bDeathHandled = true;
        Combat->CancelCombat();
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
        if (HasAuthority()) SetCanBeDamaged(false);
        OnDied.Broadcast();
    }
}

void AFPSCombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    auto* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    auto* PC = Cast<APlayerController>(GetController());
    auto* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Subsystem = LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
    if (!Enhanced || !Subsystem) return;
    RemoveCombatMapping();
    CombatFireAction = NewObject<UInputAction>(this);
    CombatReloadAction = NewObject<UInputAction>(this);
    CombatMapping = NewObject<UInputMappingContext>(this);
    CombatFireAction->ValueType = EInputActionValueType::Boolean;
    CombatReloadAction->ValueType = EInputActionValueType::Boolean;
    CombatMapping->MapKey(CombatFireAction, EKeys::LeftMouseButton);
    CombatMapping->MapKey(CombatReloadAction, EKeys::R);
    Enhanced->BindAction(CombatFireAction, ETriggerEvent::Started, Combat.Get(), &UWeaponCombatComponent::StartFire);
    Enhanced->BindAction(CombatFireAction, ETriggerEvent::Completed, Combat.Get(), &UWeaponCombatComponent::StopFire);
    Enhanced->BindAction(CombatFireAction, ETriggerEvent::Canceled, Combat.Get(), &UWeaponCombatComponent::StopFire);
    Enhanced->BindAction(CombatReloadAction, ETriggerEvent::Started, Combat.Get(), &UWeaponCombatComponent::Reload);
    Subsystem->AddMappingContext(CombatMapping, 1);
}

void AFPSCombatCharacter::RemoveCombatMapping()
{
    if (!CombatMapping) return;
    if (auto* PC = Cast<APlayerController>(GetController()))
        if (auto* LocalPlayer = PC->GetLocalPlayer())
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
                Subsystem->RemoveMappingContext(CombatMapping);
}

void AFPSCombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveCombatMapping();
    Super::EndPlay(EndPlayReason);
}
