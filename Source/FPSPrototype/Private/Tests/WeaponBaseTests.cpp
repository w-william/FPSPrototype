#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace WeaponBaseTests
{
	struct FScopedWorld
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

		~FScopedWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}

		AWeaponBase* SpawnWeapon(UWeaponDefinition* Definition)
		{
			AWeaponBase* Weapon = World ? World->SpawnActor<AWeaponBase>() : nullptr;
			if (!Weapon)
			{
				return nullptr;
			}

			// Emulate a Blueprint class default before beginning play, without a production setter.
			FObjectPropertyBase* DefinitionProperty = FindFProperty<FObjectPropertyBase>(
				AWeaponBase::StaticClass(), TEXT("WeaponDefinition"));
			check(DefinitionProperty);
			DefinitionProperty->SetObjectPropertyValue_InContainer(Weapon, Definition);
			Weapon->DispatchBeginPlay();
			return Weapon;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBaseAmmoConservationTest,
	"FPSPrototype.Weapons.AmmoConservationAndBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponBaseAmmoConservationTest::RunTest(const FString& Parameters)
{
	WeaponBaseTests::FScopedWorld Fixture;
	UWeaponDefinition* Definition = NewObject<UWeaponDefinition>(Fixture.World);
	AWeaponBase* Weapon = Fixture.SpawnWeapon(Definition);
	if (!TestNotNull(TEXT("Weapon spawned"), Weapon))
	{
		return false;
	}

	TestEqual(TEXT("Starts with a full magazine"), Weapon->GetAmmoState().MagazineAmmo, 30);
	TestEqual(TEXT("Starts with configured reserve"), Weapon->GetAmmoState().ReserveAmmo, 90);
	TestFalse(TEXT("Full magazine cannot reload"), Weapon->CanReload());
	TestEqual(TEXT("Full reload is a no-op"), Weapon->ReloadMagazine(), 0);

	TestFalse(TEXT("Zero consumption is rejected"), Weapon->TryConsumeAmmo(0));
	TestFalse(TEXT("Negative consumption cannot create ammo"), Weapon->TryConsumeAmmo(-1));
	TestFalse(TEXT("Overdraw is rejected"), Weapon->TryConsumeAmmo(31));
	TestEqual(TEXT("Rejected operations preserve magazine"), Weapon->GetAmmoState().MagazineAmmo, 30);

	TestTrue(TEXT("Accepted shot consumes one round"), Weapon->TryConsumeAmmo());
	TestEqual(TEXT("Shot changes magazine"), Weapon->GetAmmoState().MagazineAmmo, 29);
	TestEqual(TEXT("Shot preserves reserve"), Weapon->GetAmmoState().ReserveAmmo, 90);
	TestEqual(TEXT("Reload transfers only one round"), Weapon->ReloadMagazine(), 1);
	TestEqual(TEXT("Reload fills magazine"), Weapon->GetAmmoState().MagazineAmmo, 30);
	TestEqual(TEXT("Reload conserves remaining total ammo"),
		Weapon->GetAmmoState().MagazineAmmo + Weapon->GetAmmoState().ReserveAmmo, 119);

	TestTrue(TEXT("Can consume exactly the remaining magazine"), Weapon->TryConsumeAmmo(30));
	TestFalse(TEXT("Empty magazine refuses another shot"), Weapon->TryConsumeAmmo());
	TestEqual(TEXT("Ammo never becomes negative"), Weapon->GetAmmoState().MagazineAmmo, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBasePartialReloadTest,
	"FPSPrototype.Weapons.PartialReloadAndIndependentInstances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponBasePartialReloadTest::RunTest(const FString& Parameters)
{
	WeaponBaseTests::FScopedWorld Fixture;
	UWeaponDefinition* Definition = NewObject<UWeaponDefinition>(Fixture.World);
	Definition->MagazineCapacity = 10;
	Definition->InitialReserveAmmo = 3;

	AWeaponBase* First = Fixture.SpawnWeapon(Definition);
	AWeaponBase* Second = Fixture.SpawnWeapon(Definition);
	if (!TestNotNull(TEXT("First weapon"), First) || !TestNotNull(TEXT("Second weapon"), Second))
	{
		return false;
	}

	TestTrue(TEXT("First weapon fires seven rounds"), First->TryConsumeAmmo(7));
	TestEqual(TEXT("Partial reload consumes only available reserve"), First->ReloadMagazine(), 3);
	TestEqual(TEXT("Partial magazine is not magically filled"), First->GetAmmoState().MagazineAmmo, 6);
	TestEqual(TEXT("Reserve is exhausted"), First->GetAmmoState().ReserveAmmo, 0);
	TestFalse(TEXT("No reserve means no reload"), First->CanReload());
	TestEqual(TEXT("Repeated reload adds nothing"), First->ReloadMagazine(), 0);
	TestEqual(TEXT("Second magazine is independent"), Second->GetAmmoState().MagazineAmmo, 10);
	TestEqual(TEXT("Second reserve is independent"), Second->GetAmmoState().ReserveAmmo, 3);
	TestEqual(TEXT("Shared definition is not mutated"), Definition->InitialReserveAmmo, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBaseAuthorityTest,
	"FPSPrototype.Weapons.NonAuthorityCannotMutateAmmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponBaseAuthorityTest::RunTest(const FString& Parameters)
{
	WeaponBaseTests::FScopedWorld Fixture;
	UWeaponDefinition* Definition = NewObject<UWeaponDefinition>(Fixture.World);
	AWeaponBase* Weapon = Fixture.SpawnWeapon(Definition);
	if (!TestNotNull(TEXT("Weapon spawned"), Weapon))
	{
		return false;
	}

	TestTrue(TEXT("Server prepares a partially used magazine"), Weapon->TryConsumeAmmo(5));

	// Exercises C++ authority guards, not an end-to-end network transport test.
	Weapon->SetRole(ROLE_AutonomousProxy);
	TestFalse(TEXT("Client cannot consume authoritative ammo"), Weapon->TryConsumeAmmo());
	TestEqual(TEXT("Client cannot finish a reload"), Weapon->ReloadMagazine(), 0);
	TestEqual(TEXT("Client attempts preserve magazine"), Weapon->GetAmmoState().MagazineAmmo, 25);
	TestEqual(TEXT("Client attempts preserve reserve"), Weapon->GetAmmoState().ReserveAmmo, 90);

	Weapon->SetRole(ROLE_Authority);
	return true;
}

#endif
