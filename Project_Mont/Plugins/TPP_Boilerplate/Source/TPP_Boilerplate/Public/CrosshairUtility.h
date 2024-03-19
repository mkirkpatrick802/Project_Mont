#pragma once

#include "CoreMinimal.h"

class ATPPCharacter;
struct FHUDPackage;
class ATPPHUD;

#define TRACE_LENGTH 80000

class TPP_BOILERPLATE_API CrosshairUtility
{
public:

	CrosshairUtility();
	~CrosshairUtility();

	static void TraceUnderCrosshairs(const ATPPCharacter* Character, FHitResult& TraceHitResult, bool OffsetStart = true, float OffsetLength = 200, float TraceLength = TRACE_LENGTH, ECollisionChannel TraceChannel = ECC_Visibility);
};
