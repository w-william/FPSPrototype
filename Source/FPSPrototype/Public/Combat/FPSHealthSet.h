#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FPSHealthSet.generated.h"

#define FPS_ATTRIBUTE_ACCESSORS(Class, Property) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(Class, Property) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Property) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(Property) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(Property)

UCLASS()
class FPSPROTOTYPE_API UFPSHealthSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UFPSHealthSet();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Health")
    FGameplayAttributeData Health;
    FPS_ATTRIBUTE_ACCESSORS(UFPSHealthSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Health")
    FGameplayAttributeData MaxHealth;
    FPS_ATTRIBUTE_ACCESSORS(UFPSHealthSet, MaxHealth)

    // Server-only meta attribute. The damage effect writes here, then health is clamped.
    UPROPERTY()
    FGameplayAttributeData IncomingDamage;
    FPS_ATTRIBUTE_ACCESSORS(UFPSHealthSet, IncomingDamage)

private:
    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
};

#undef FPS_ATTRIBUTE_ACCESSORS
