#pragma once

#include "CoreMinimal.h"
#include "FPSPrototypeCharacter.h"
#include "AbilitySystemInterface.h"
#include "FPSCombatCharacter.generated.h"

class UAbilitySystemComponent;
class UFPSHealthSet;
class UWeaponCombatComponent;
class UInputAction;
class UInputMappingContext;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFPSHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFPSDied);

// BP_Character keeps its existing mesh, animation and weapon-spawn graph.
UCLASS()
class FPSPROTOTYPE_API AFPSCombatCharacter : public AFPSPrototypeCharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()
public:
    AFPSCombatCharacter();
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual void PostInitializeComponents() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_Controller() override;
    virtual FVector GetPawnViewLocation() const override;
    virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintPure, Category="Combat") float GetHealth() const;
    UFUNCTION(BlueprintPure, Category="Combat") float GetMaxHealth() const;
    UFUNCTION(BlueprintPure, Category="Combat") bool IsCombatDead() const;
    UFUNCTION(BlueprintPure, Category="Combat") UWeaponCombatComponent* GetCombatComponent() const { return Combat; }
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Combat") bool EquipCombatWeapon(AActor* Weapon);

    UPROPERTY(BlueprintAssignable, Category="Combat") FFPSHealthChanged OnHealthChanged;
    UPROPERTY(BlueprintAssignable, Category="Combat") FFPSDied OnDied;

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<UFPSHealthSet> HealthAttributes;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<UWeaponCombatComponent> Combat;

private:
    void HandleHealthChanged(const FOnAttributeChangeData& Data);
    void RemoveCombatMapping();
    bool bDeathHandled = false;
    UPROPERTY(Transient) TObjectPtr<UInputAction> CombatFireAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> CombatReloadAction;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> CombatMapping;
};
