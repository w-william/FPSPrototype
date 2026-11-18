#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "NativeGameplayTags.h"
#include "FPSDamageEffect.generated.h"

FPSPROTOTYPE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_FPS_Data_Damage);

UCLASS()
class FPSPROTOTYPE_API UFPSDamageEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UFPSDamageEffect();
};
