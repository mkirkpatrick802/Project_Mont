#include "TPPHUD.h"

void ATPPHUD::DrawHUD()
{
	Super::DrawHUD();

	if (GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		if(HUDPackage.CombatMode)
		{
			DrawCombatCrosshairs(ViewportCenter);
		}
		else
		{
			DrawIdleCrosshairs(ViewportCenter);
		}
	}
}

void ATPPHUD::DrawIdleCrosshairs(const FVector2D ViewportCenter)
{
	if (HUDPackage.CrosshairsCenter)
	{
		const FVector2D Spread(0.f, 0.f);
		DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairColor);
	}
}

void ATPPHUD::DrawCombatCrosshairs(const FVector2D ViewportCenter)
{
	const float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

	if (HUDPackage.CrosshairsCenter)
	{
		const FVector2D Spread(0.f, 0.f);
		DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairColor);
	}

	if (HUDPackage.CrosshairsLeft)
	{
		const FVector2D Spread(-SpreadScaled, 0);
		DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairColor);
	}

	if (HUDPackage.CrosshairsRight)
	{
		const FVector2D Spread(SpreadScaled, 0);
		DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairColor);
	}

	if (HUDPackage.CrosshairsTop)
	{
		const FVector2D Spread(0, -SpreadScaled);
		DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairColor);
	}

	if (HUDPackage.CrosshairsBottom)
	{
		const FVector2D Spread(0, SpreadScaled);
		DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairColor);
	}
}

void ATPPHUD::DrawCrosshair(UTexture2D* Texture, const FVector2D ViewportCenter, const FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX() * HUDPackage.CrosshairSize;
	const float TextureHeight = Texture->GetSizeY() * HUDPackage.CrosshairSize;
	const FVector2D TextureDrawPoint(ViewportCenter.X - TextureWidth / 2.f + Spread.X, ViewportCenter.Y - TextureHeight / 2.f + Spread.Y);

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.f, 0.f, 1.f, 1.f, CrosshairColor);
}
