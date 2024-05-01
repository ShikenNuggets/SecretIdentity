// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayerCameraBoom.h"

#include "PlayerCameraComponent.h"
#include "UE_Helpers.h"

UPlayerCameraBoom::UPlayerCameraBoom() : FollowDistanceChangeTime(0.25f), bHasTargetFollowDistance(false), fTargetFollowDistance(0.0f), fTimer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bUsePawnControlRotation = true;
	TargetArmLength = DefaultFollowDistance;
}

void UPlayerCameraBoom::BeginPlay()
{
	Super::BeginPlay();
	WARN_IF(FMath::IsNearlyZero(FollowDistanceChangeTime) || FollowDistanceChangeTime < 0.0f);
}

void UPlayerCameraBoom::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

void UPlayerCameraBoom::OnPlayerStateChanged(EPlayerControlState State)
{
	switch (State)
	{
		case EPlayerControlState::Default: //Intentional fallthrough
		case EPlayerControlState::Sprinting:
		case EPlayerControlState::Punching:
			SetTargetFollowDistance(DefaultFollowDistance);
			break;

		case EPlayerControlState::TravelPower_Flight_Strafe: //Intentional fallthrough
		case EPlayerControlState::TravelPower_Flight_Forward:
			SetTargetFollowDistance(FlightFollowDistance);
			break;

		default:
			WARN_IF_MSG(true, "EPlayerControlState case not handled in UPlayerCameraBoom::OnPlayerStateChanged!");
			break;
	}
}