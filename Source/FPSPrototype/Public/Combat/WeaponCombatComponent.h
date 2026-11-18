#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponCombatComponent.generated.h"

class AWeaponBase;
class AFPSCombatCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFPSShotFired, FVector, Start, FVector, End, bool, bBlockingHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSHitConfirmed, float, Damage, bool, bKilled);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class FPSPROTOTYPE_API UWeaponCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UWeaponCombatComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintCallable, Category="Combat") void StartFire();
    UFUNCTION(BlueprintCallable, Category="Combat") void StopFire();
    UFUNCTION(BlueprintCallable, Category="Combat") void Reload();
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat") bool SetEquippedWeapon(AWeaponBase* Weapon);
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat") bool TryFireShot();
    UFUNCTION(BlueprintPure, Category="Combat") AWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }
    UFUNCTION(BlueprintPure, Category="Combat") bool IsReloading() const { return bReloading; }
    float GetLastConfirmedHitTime() const { return LastConfirmedHitTime; }
    void CancelCombat();

    UPROPERTY(BlueprintAssignable, Category="Combat") FFPSShotFired OnShotFired;
    UPROPERTY(BlueprintAssignable, Category="Combat") FFPSHitConfirmed OnHitConfirmed;
    // Temporary visible feedback until weapon VFX are assigned; never draws on dedicated servers.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Feedback") bool bDrawShotTraces = true;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Replicated) TObjectPtr<AWeaponBase> EquippedWeapon;
    UPROPERTY(Replicated) bool bReloading = false;
    UFUNCTION(Server, Reliable) void ServerSetTriggerHeld(bool bHeld);
    UFUNCTION(Server, Reliable) void ServerReload();
    UFUNCTION(NetMulticast, Unreliable) void MulticastShot(FVector_NetQuantize Start, FVector_NetQuantize End, bool bBlockingHit);
    UFUNCTION(Client, Unreliable) void ClientConfirmHit(float Damage, bool bKilled);
    void SetTriggerHeld(bool bHeld);
    void FireAutomatic();
    void BeginReload();
    void FinishReload();
    bool CanUseWeapon() const;
    bool CanSendInput() const;
    AFPSCombatCharacter* GetCombatCharacter() const;
    FTimerHandle FireTimer;
    FTimerHandle ReloadTimer;
    TWeakObjectPtr<AWeaponBase> ReloadingWeapon;
    double NextAllowedFireTime = 0.0;
    float LastConfirmedHitTime = -100.f;
    bool bTriggerHeld = false;
};
