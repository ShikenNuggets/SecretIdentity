// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/Vector.h"

class AActor;

namespace UEHoverDrone
{
	float HOVERDRONE_API MeasureAltitude(const AActor* Actor, FVector Offset = FVector(ForceInitToZero));
	int32 HOVERDRONE_API ApplyDroneLimiters(const AActor* Actor, FVector& ControlAcceleration);
}