// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayerCameraComponent.h"

#include "SecretIdentity/UE_Helpers.h"

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
	WARN_IF(FMath::IsNearlyZero(FOVChangeTime) || FOVChangeTime <= 0.0f);
	WARN_IF(FMath::IsNearlyZero(FOVMaxSpeedChangeTime) || FOVMaxSpeedChangeTime <= 0.0f);
}

void UPlayerCameraComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bHasTargetFOV)
	{
		WARN_IF(FMath::IsNearlyZero(fCurrentFOVChangeTime) || fCurrentFOVChangeTime <= 0.0f);
		fTimer += DeltaTime;

		FieldOfView = FMath::Lerp(fStartFOV, fTargetFOV, fTimer / fCurrentFOVChangeTime);
		
		if (fTimer >= fCurrentFOVChangeTime)
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
	fCurrentFOVChangeTime = FOVChangeTime;
	fTimer = 0.0f;
}

void UPlayerCameraComponent::SetTargetFOVWithDelay(float Target)
{
	WARN_IF_NULL(GetWorld());

	if (GetWorld())
	{
		FTimerHandle handle;
		GetWorld()->GetTimerManager().SetTimer(handle, FTimerDelegate::CreateLambda([&, Target]
		{
			SetTargetFOV(Target);
			fCurrentFOVChangeTime = FOVMaxSpeedChangeTime;
		}), FOVChangeDelay, false);
	}
}

void UPlayerCameraComponent::OnPlayerStateChanged(EPlayerControlState State)
{
	switch (State)
	{
		case EPlayerControlState::None: //Intentional fallthrough
		case EPlayerControlState::Default:
		case EPlayerControlState::Sprinting:
		case EPlayerControlState::Punching:
			SetTargetFOV(DefaultFOV);
			break;

		case EPlayerControlState::TravelPower_Flight_Strafe: //Intentional fallthrough
		case EPlayerControlState::TravelPower_Flight_Forward:
			SetTargetFOV(FlightFOV);
			break;

		default:
			WARN_IF_MSG(true, "EPlayerControlState case not handled in UPlayerCameraComponent::OnPlayerStateChanged!");
			break;
	}
}

void UPlayerCameraComponent::OnFlightSpeedChanged(bool IsNearMaxSpeed)
{
	if (IsNearMaxSpeed)
	{
		SetTargetFOVWithDelay(FlightMaxSpeedFOV);
	}
	else
	{
		SetTargetFOV(FlightFOV);
	}
}