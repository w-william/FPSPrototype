#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPSCombatHUD.generated.h"

UCLASS()
class FPSPROTOTYPE_API AFPSCombatHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
