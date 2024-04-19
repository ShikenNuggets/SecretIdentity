// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayerCameraComponent.h"

#include "UE_Helpers.h"

UPlayerCameraComponent::UPlayerCameraComponent() : FOVChangeTime(0.25f), bHasTargetFOV(false), fTargetFOV(0.0f), fTimer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	WARN_IF(FMath::IsNearlyZero(FOVChangeTime) || FOVChangeTime < 0.0f);
}

void UPlayerCameraComponent::UpdateTimer(float DeltaTime)
{
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