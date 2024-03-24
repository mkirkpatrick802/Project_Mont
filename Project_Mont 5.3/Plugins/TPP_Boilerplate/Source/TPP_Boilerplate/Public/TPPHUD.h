#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPPHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

	bool CombatMode = false;

	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;

	float CrosshairSpread;
	float CrosshairSize = 1;

	FLinearColor CrosshairColor;
};

UCLASS()
class TPP_BOILERPLATE_API ATPPHUD : public AHUD
{
	GENERATED_BODY()
	
public:

	virtual void DrawHUD() override;

private:

	void DrawIdleCrosshairs(const FVector2D ViewportCenter);
	void DrawCombatCrosshairs(const FVector2D ViewportCenter);

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

private:

	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere, Category=Crosshairs)
	float CrosshairSpreadMax = 16.f;

public:

	FORCEINLINE void SetHudPackage(const FHUDPackage& Package) { HUDPackage = Package; }

};