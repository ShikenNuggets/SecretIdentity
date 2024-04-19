// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayerCameraBoom.h"

#include "UE_Helpers.h"

UPlayerCameraBoom::UPlayerCameraBoom() : FollowDistanceChangeTime(0.25f), bHasTargetFollowDistance(false), fTargetFollowDistance(0.0f), fTimer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPlayerCameraBoom::BeginPlay()
{
	Super::BeginPlay();
	WARN_IF(FMath::IsNearlyZero(FollowDistanceChangeTime) || FollowDistanceChangeTime < 0.0f);
}

void UPlayerCameraBoom::UpdateTimer(float DeltaTime)
{
	if (bHasTargetFollowDistance)
	{
		fTimer += DeltaTime;

		TargetArmLength = FMath::Lerp(fStartFollowDistance, fTargetFollowDistance, fTimer / FollowDistanceChangeTime);
		
		if (fTimer >= FollowDistanceChangeTime)
		{
			bHasTargetFollowDistance = false;
			TargetArmLength = fTargetFollowDistance;
			fTimer = 0.0f;
		}
	}
}

void UPlayerCameraBoom::SetTargetFollowDistance(float Target)
{
	if (bHasTargetFollowDistance && fTargetFollowDistance == Target)
	{
		return;
	}

	bHasTargetFollowDistance = true;
	fStartFollowDistance = TargetArmLength;
	fTargetFollowDistance = Target;
	fTimer = 0.0f;
}