#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class UWeaponDefinition;

/** Replicated together so consumers receive one coherent ammo update. */
USTRUCT(BlueprintType)
struct FPSPROTOTYPE_API FWeaponAmmoState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 MagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 ReserveAmmo = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnWeaponAmmoChanged, int32, MagazineAmmo, int32, ReserveAmmo);

/**
 * A server-spawned weapon instance with mesh, muzzle and owner-only ammunition.
 * The combat component/abilities validate actions and own firing/reload timing.
 */
UCLASS()
class FPSPROTOTYPE_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UWeaponDefinition* GetWeaponDefinition() const { return WeaponDefinition.Get(); }

	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	FWeaponAmmoState GetAmmoState() const { return AmmoState; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	int32 GetMagazineCapacity() const;

	/** Ammo-only check; the combat layer also checks death, equip and reload state. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	bool CanReload() const;

	/** Call on the server after accepting a shot. This function is not an RPC. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon|Ammo")
	bool TryConsumeAmmo(int32 Amount = 1);

	/**
	 * Transfers reserve ammo into the magazine and returns the amount loaded.
	 * Call on the server only after a reload finishes successfully.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon|Ammo")
	int32 ReloadMagazine();

	/** The authoritative combat layer must ensure the parent transform is current. */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FTransform GetMuzzleTransform() const;

	/** Bind then read GetAmmoState once, since the initial update may already have arrived. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Ammo")
	FOnWeaponAmmoChanged OnAmmoChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<USceneComponent> MuzzlePoint;

	/** Set in the weapon Blueprint defaults; not replaced dynamically at runtime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Config")
	TObjectPtr<UWeaponDefinition> WeaponDefinition;

private:
	UPROPERTY(ReplicatedUsing = OnRep_AmmoState)
	FWeaponAmmoState AmmoState;

	UFUNCTION()
	void OnRep_AmmoState();

	void BroadcastAmmoChanged();
};
