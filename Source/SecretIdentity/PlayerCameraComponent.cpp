// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayerCameraComponent.h"

#include "UE_Helpers.h"

UPlayerCameraComponent::UPlayerCameraComponent() : FOVChangeTime(0.25f), bHasTargetFOV(false), fTargetFOV(0.0f), fTimer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bUsePawnControlRotation = false;
	FieldOfView = DefaultFOV;
}

void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	WARN_IF(FMath::IsNearlyZero(FOVChangeTime) || FOVChangeTime < 0.0f);
}

void UPlayerCameraComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHasTargetFOV)
	{
		fTimer += DeltaTime;

		FieldOfView = FMath::Lerp(fStartFOV, fTargetFOV, fTimer / FOVChangeTime);
		
		if (fTimer >= FOVChangeTime)
		{
			bHasTargetFOV = false;
			FieldOfView = fTargetFOV;
			fTimer = 0.0f;
		}
	}
}

void UPlayerCameraComponent::SetTargetFOV(float Target)
{
	if (bHasTargetFOV && fTargetFOV == Target)
	{
		return;
	}

	bHasTargetFOV = true;
	fStartFOV = FieldOfView;
	fTargetFOV = Target;
	fTimer = 0.0f;
}

void UPlayerCameraComponent::OnPlayerStateChanged(ControlState State)
{
	switch (State)
	{
		case ControlState::Default: //Intentional fallthrough
		case ControlState::Sprinting:
			SetTargetFOV(DefaultFOV);
			break;

		case ControlState::TravelPower_Flight_Strafe: //Intentional fallthrough
		case ControlState::TravelPower_Flight_Forward:
			SetTargetFOV(FlightFOV);
			break;

		default:
			WARN_IF_MSG(true, "ControlState case not handled!");
			break;
	}
}