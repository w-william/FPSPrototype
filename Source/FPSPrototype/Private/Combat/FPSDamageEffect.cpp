#include "Combat/FPSDamageEffect.h"
#include "Combat/FPSHealthSet.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_FPS_Data_Damage, "Data.Damage");

UFPSDamageEffect::UFPSDamageEffect()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UFPSHealthSet::GetIncomingDamageAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = TAG_FPS_Data_Damage;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
