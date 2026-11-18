#include "Combat/FPSCombatHUD.h"
#include "Combat/FPSCombatCharacter.h"
#include "Combat/WeaponCombatComponent.h"
#include "Weapons/WeaponBase.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void AFPSCombatHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* Character = PlayerOwner ? Cast<AFPSCombatCharacter>(PlayerOwner->GetPawn()) : nullptr;
    if (!Canvas || !Character) return;
    const float X = Canvas->ClipX * 0.5f, Y = Canvas->ClipY * 0.5f;
    UWeaponCombatComponent* Combat = Character->GetCombatComponent();
    const bool bRecentHit = GetWorld()->GetTimeSeconds() - Combat->GetLastConfirmedHitTime() < 0.15f;
    const FLinearColor Color = bRecentHit ? FLinearColor::Red : FLinearColor::White;
    DrawLine(X-9, Y, X-3, Y, Color, 1.5f);
    DrawLine(X+3, Y, X+9, Y, Color, 1.5f);
    DrawLine(X, Y-9, X, Y-3, Color, 1.5f);
    DrawLine(X, Y+3, X, Y+9, Color, 1.5f);
    DrawText(FString::Printf(TEXT("HP %.0f / %.0f"), Character->GetHealth(), Character->GetMaxHealth()),
        FLinearColor::White, 24.f, Canvas->ClipY-70.f, GEngine->GetMediumFont());
    if (AWeaponBase* Weapon = Combat->GetEquippedWeapon())
    {
        const FWeaponAmmoState Ammo = Weapon->GetAmmoState();
        DrawText(FString::Printf(TEXT("AMMO %d / %d%s"), Ammo.MagazineAmmo, Ammo.ReserveAmmo,
            Combat->IsReloading() ? TEXT("  RELOADING") : TEXT("")),
            FLinearColor::White, 24.f, Canvas->ClipY-45.f, GEngine->GetMediumFont());
    }
    if (Character->IsCombatDead())
        DrawText(TEXT("DEAD"), FLinearColor::Red, X-32.f, Y+35.f, GEngine->GetLargeFont());
}
