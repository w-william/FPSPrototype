#include "Weapons/WeaponBase.h"

#include "Weapons/WeaponDefinition.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "FPSPrototype.h"
#include "Net/UnrealNetwork.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	bOnlyRelevantToOwner = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetSimulatePhysics(false);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (!WeaponDefinition || GetMagazineCapacity() <= 0)
	{
		UE_LOG(LogFPSPrototype, Warning,
			TEXT("%s: Assign a WeaponDefinition with a positive MagazineCapacity."), *GetName());
		return;
	}

	AmmoState.MagazineAmmo = GetMagazineCapacity();
	AmmoState.ReserveAmmo = FMath::Max(0, WeaponDefinition->InitialReserveAmmo);
	BroadcastAmmoChanged();
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Set Owner to the possessing character when spawning the weapon on the server.
	DOREPLIFETIME_CONDITION(AWeaponBase, AmmoState, COND_OwnerOnly);
}

int32 AWeaponBase::GetMagazineCapacity() const
{
	return WeaponDefinition ? FMath::Max(0, WeaponDefinition->MagazineCapacity) : 0;
}

bool AWeaponBase::CanReload() const
{
	return WeaponDefinition
		&& AmmoState.ReserveAmmo > 0
		&& AmmoState.MagazineAmmo < GetMagazineCapacity();
}

bool AWeaponBase::TryConsumeAmmo(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0 || AmmoState.MagazineAmmo < Amount)
	{
		return false;
	}

	AmmoState.MagazineAmmo -= Amount;
	BroadcastAmmoChanged();
	ForceNetUpdate();
	return true;
}

int32 AWeaponBase::ReloadMagazine()
{
	if (!HasAuthority() || !CanReload())
	{
		return 0;
	}

	const int32 MissingAmmo = GetMagazineCapacity() - AmmoState.MagazineAmmo;
	const int32 AmmoToLoad = FMath::Min(MissingAmmo, AmmoState.ReserveAmmo);
	AmmoState.MagazineAmmo += AmmoToLoad;
	AmmoState.ReserveAmmo -= AmmoToLoad;

	BroadcastAmmoChanged();
	ForceNetUpdate();
	return AmmoToLoad;
}

FTransform AWeaponBase::GetMuzzleTransform() const
{
	return MuzzlePoint->GetComponentTransform();
}

void AWeaponBase::OnRep_AmmoState()
{
	BroadcastAmmoChanged();
}

void AWeaponBase::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(AmmoState.MagazineAmmo, AmmoState.ReserveAmmo);
}
