#include "Combat/FPSHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UFPSHealthSet::UFPSHealthSet()
{
    InitMaxHealth(100.f);
    InitHealth(100.f);
    InitIncomingDamage(0.f);
}

void UFPSHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UFPSHealthSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UFPSHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UFPSHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (!FMath::IsFinite(NewValue)) NewValue = 0.f;
    if (Attribute == GetMaxHealthAttribute()) NewValue = FMath::Max(1.f, NewValue);
    if (Attribute == GetHealthAttribute()) NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
}

void UFPSHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        const float Damage = GetIncomingDamage();
        SetIncomingDamage(0.f);
        if (FMath::IsFinite(Damage) && Damage > 0.f && GetHealth() > 0.f)
            SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute() || Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.f, FMath::Max(1.f, GetMaxHealth())));
    }
}

void UFPSHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSHealthSet, Health, OldValue);
}

void UFPSHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSHealthSet, MaxHealth, OldValue);
}
