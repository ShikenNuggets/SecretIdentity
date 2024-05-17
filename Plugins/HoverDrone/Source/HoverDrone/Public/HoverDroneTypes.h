// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "HoverDroneTypes.generated.h"

USTRUCT(BlueprintType)
struct FDroneSpeedParameters
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly)
	float LinearAccelScale;

	UPROPERTY(EditDefaultsOnly)
	float LinearDecelScale;

	UPROPERTY(EditDefaultsOnly)
	float RotAccelScale;

	UPROPERTY(EditDefaultsOnly)
	float RotDecelScale;

	UPROPERTY(EditDefaultsOnly)
	float MaxRotSpeedScale;

	UPROPERTY(EditDefaultsOnly)
	float HoverThrustScale;

	FDroneSpeedParameters(float InLinearAccelScale, float InLinearDecelScale, float InRotAccelScale, float InRotDecelScale, float InMaxRotSpeedScale, float InHoverThrustScale)
		: LinearAccelScale(InLinearAccelScale)
		, LinearDecelScale(InLinearDecelScale)
		, RotAccelScale(InRotAccelScale)
		, RotDecelScale(InRotDecelScale)
		, MaxRotSpeedScale(InMaxRotSpeedScale)
		, HoverThrustScale(InHoverThrustScale)
	{}

	FDroneSpeedParameters(float Scales)
		: LinearAccelScale(Scales)
		, LinearDecelScale(Scales)
		, RotAccelScale(Scales)
		, RotDecelScale(Scales)
		, MaxRotSpeedScale(Scales)
		, HoverThrustScale(Scales)
	{}

	FDroneSpeedParameters():
		FDroneSpeedParameters(1.f)
	{
		
	}
};