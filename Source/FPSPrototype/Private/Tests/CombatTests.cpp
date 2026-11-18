#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/FPSCombatCharacter.h"
#include "Combat/WeaponCombatComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace FPSCombatTests
{
struct FFixture
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AFPSCombatCharacter* Shooter = nullptr;
    AFPSCombatCharacter* Target = nullptr;
    AWeaponBase* Weapon = nullptr;
    UWeaponDefinition* Definition = nullptr;

    FFixture()
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
        Context.OwningGameInstance = GameInstance;
        World->SetGameInstance(GameInstance);
        World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
        World->SetGameMode(FURL());
        World->InitializeActorsForPlay(FURL());
        World->BeginPlay();
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.bDeferConstruction = true;
        Shooter = World->SpawnActor<AFPSCombatCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        Target = World->SpawnActor<AFPSCombatCharacter>(FVector(500, 0, 0), FRotator::ZeroRotator, Params);
        for (auto* Character : {Shooter, Target})
        {
            Character->GetFirstPersonCameraComponent()->AttachToComponent(Character->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            Character->FinishSpawning(Character->GetActorTransform());
            Character->GetCharacterMovement()->DisableMovement();
        }
        Shooter->GetFirstPersonCameraComponent()->SetWorldLocation(FVector::ZeroVector);
        Weapon = World->SpawnActor<AWeaponBase>(Params);
        Definition = NewObject<UWeaponDefinition>(World);
        auto* Property = FindFProperty<FObjectPropertyBase>(AWeaponBase::StaticClass(), TEXT("WeaponDefinition"));
        Property->SetObjectPropertyValue_InContainer(Weapon, Definition);
        Weapon->SetOwner(Shooter);
        Weapon->SetInstigator(Shooter);
        Weapon->AttachToComponent(Shooter->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Weapon->FinishSpawning(Weapon->GetActorTransform());
        Shooter->EquipCombatWeapon(Weapon);
    }
    ~FFixture() { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
    UWeaponCombatComponent* Combat() { return Shooter->GetCombatComponent(); }
    void Tick(float Seconds)
    {
        while (Seconds > 0.f)
        {
            const float Delta = FMath::Min(Seconds, 0.02f);
            ++GFrameCounter;
            World->Tick(LEVELTICK_All, Delta);
            Seconds -= Delta;
        }
    }
    void Wall()
    {
        AActor* Actor = World->SpawnActor<AActor>();
        auto* Box = NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(10, 120, 120));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        Actor->SetActorLocation(FVector(250, 0, 0));
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSCombatHitTest, "FPSPrototype.Combat.HitscanDamageAndRateLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSCombatHitTest::RunTest(const FString& Parameters)
{
    FPSCombatTests::FFixture F;
    TestEqual(TEXT("Initial GAS health"), F.Target->GetHealth(), 100.f);
    TestTrue(TEXT("First server shot accepted"), F.Combat()->TryFireShot());
    TestEqual(TEXT("Ray hit applies 25 GAS damage"), F.Target->GetHealth(), 75.f);
    TestEqual(TEXT("Exactly one bullet consumed"), F.Weapon->GetAmmoState().MagazineAmmo, 29);
    TestFalse(TEXT("Same-frame second shot rejected"), F.Combat()->TryFireShot());
    TestEqual(TEXT("Rejected shot cannot damage"), F.Target->GetHealth(), 75.f);
    F.Tick(0.11f);
    TestTrue(TEXT("Shot after cooldown accepted"), F.Combat()->TryFireShot());
    TestEqual(TEXT("Second hit"), F.Target->GetHealth(), 50.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSCombatCoverTest, "FPSPrototype.Combat.CoverMissAndRange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSCombatCoverTest::RunTest(const FString& Parameters)
{
    {
        FPSCombatTests::FFixture F;
        F.Wall();
        TestTrue(TEXT("A shot into a wall is still a shot"), F.Combat()->TryFireShot());
        TestEqual(TEXT("Wall blocks damage"), F.Target->GetHealth(), 100.f);
        TestEqual(TEXT("Wall hit consumes ammo"), F.Weapon->GetAmmoState().MagazineAmmo, 29);
        F.Weapon->SetActorRelativeLocation(FVector(350, 0, 0));
        F.Tick(0.11f);
        F.Combat()->TryFireShot();
        TestEqual(TEXT("Muzzle through a wall cannot bypass cover"), F.Target->GetHealth(), 100.f);
    }
    {
        FPSCombatTests::FFixture F;
        F.Definition->MaxRange = 100.f;
        F.Combat()->TryFireShot();
        TestEqual(TEXT("Out-of-range target is unharmed"), F.Target->GetHealth(), 100.f);
        F.Definition->MaxRange = 10000.f;
        F.Target->SetActorLocation(FVector(500, 500, 0));
        F.Tick(0.11f);
        F.Combat()->TryFireShot();
        TestEqual(TEXT("Miss is harmless"), F.Target->GetHealth(), 100.f);
        TestEqual(TEXT("Miss still consumes ammo"), F.Weapon->GetAmmoState().MagazineAmmo, 28);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSCombatAuthorityTest, "FPSPrototype.Combat.AuthorityOwnershipAndDeath", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSCombatAuthorityTest::RunTest(const FString& Parameters)
{
    FPSCombatTests::FFixture F;
    F.Shooter->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Client cannot execute authoritative shot"), F.Combat()->TryFireShot());
    F.Shooter->SetRole(ROLE_Authority);
    F.Target->SetRole(ROLE_SimulatedProxy);
    TestEqual(TEXT("Client cannot directly damage health"), UGameplayStatics::ApplyDamage(F.Target, 25.f, nullptr, F.Weapon, nullptr), 0.f);
    TestEqual(TEXT("Client damage preserved health"), F.Target->GetHealth(), 100.f);
    F.Target->SetRole(ROLE_Authority);
    TestFalse(TEXT("Another pawn cannot equip this weapon"), F.Target->EquipCombatWeapon(F.Weapon));
    F.Weapon->SetOwner(F.Target);
    TestFalse(TEXT("Lost ownership prevents firing"), F.Combat()->TryFireShot());
    F.Weapon->SetOwner(F.Shooter);
    TestEqual(TEXT("Negative damage cannot heal"), UGameplayStatics::ApplyDamage(F.Target, -20.f, nullptr, F.Weapon, nullptr), 0.f);
    TestEqual(TEXT("Overkill returns actual lost health"), UGameplayStatics::ApplyDamage(F.Target, 500.f, nullptr, F.Weapon, nullptr), 100.f);
    TestEqual(TEXT("Health clamps to zero"), F.Target->GetHealth(), 0.f);
    TestTrue(TEXT("Death state reached"), F.Target->IsCombatDead());
    TestEqual(TEXT("Dead target cannot take further damage"), UGameplayStatics::ApplyDamage(F.Target, 20.f, nullptr, F.Weapon, nullptr), 0.f);
    UGameplayStatics::ApplyDamage(F.Shooter, 100.f, nullptr, F.Target, nullptr);
    TestFalse(TEXT("Dead shooter cannot fire"), F.Combat()->TryFireShot());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSCombatTriggerTest, "FPSPrototype.Combat.AutomaticSemiAutoAndReload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSCombatTriggerTest::RunTest(const FString& Parameters)
{
    FPSCombatTests::FFixture F;
    F.Target->SetActorLocation(FVector(500, 500, 0));
    F.Combat()->StartFire();
    F.Combat()->StartFire();
    F.Tick(0.25f);
    TestEqual(TEXT("Held automatic trigger fires at 600 RPM"), F.Weapon->GetAmmoState().MagazineAmmo, 27);
    F.Combat()->StopFire();
    F.Tick(0.25f);
    TestEqual(TEXT("Release stops firing"), F.Weapon->GetAmmoState().MagazineAmmo, 27);
    F.Definition->bAutomatic = false;
    F.Combat()->StartFire();
    F.Tick(0.25f);
    TestEqual(TEXT("Held semi-auto trigger fires once"), F.Weapon->GetAmmoState().MagazineAmmo, 26);
    F.Combat()->StopFire();
    F.Combat()->Reload();
    TestTrue(TEXT("Reload begins"), F.Combat()->IsReloading());
    TestFalse(TEXT("Cannot shoot during reload"), F.Combat()->TryFireShot());
    F.Tick(1.f);
    F.Combat()->Reload();
    TestEqual(TEXT("No instant reload"), F.Weapon->GetAmmoState().MagazineAmmo, 26);
    F.Tick(1.1f);
    TestFalse(TEXT("Reload finishes once"), F.Combat()->IsReloading());
    TestEqual(TEXT("Magazine refilled"), F.Weapon->GetAmmoState().MagazineAmmo, 30);
    TestEqual(TEXT("Reserve transfers exactly missing ammo"), F.Weapon->GetAmmoState().ReserveAmmo, 86);
    F.Weapon->TryConsumeAmmo(30);
    TestFalse(TEXT("Empty gun cannot fire"), F.Combat()->TryFireShot());
    F.Combat()->Reload();
    F.Shooter->EquipCombatWeapon(nullptr);
    F.Tick(2.2f);
    TestEqual(TEXT("Unequipping cancels pending reload"), F.Weapon->GetAmmoState().MagazineAmmo, 0);
    return true;
}
#endif
