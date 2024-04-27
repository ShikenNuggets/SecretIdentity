// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EPlayerControlState : uint8
{
	Default = 0,
	Sprinting,
	TravelPower_Flight_Strafe,
	TravelPower_Flight_Forward,

	Count UMETA(Hidden)
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerControlStateDelegate, EPlayerControlState);