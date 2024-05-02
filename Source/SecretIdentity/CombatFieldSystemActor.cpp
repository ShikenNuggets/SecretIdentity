// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "CombatFieldSystemActor.h"

ACombatFieldSystemActor::ACombatFieldSystemActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ACombatFieldSystemActor::SetFieldActive(bool Active)
{
	IsActive = Active;
}

void ACombatFieldSystemActor::DrawDebugInfo(bool DrawDebug)
{
	#if !UE_BUILD_SHIPPING
		IsDebug = DrawDebug;
	#endif // !UE_BUILD_SHIPPING
}