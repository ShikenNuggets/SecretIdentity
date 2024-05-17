// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "HoverDroneSpeedLimitBox.generated.h"

/**
 * 
 */
UCLASS()
class HOVERDRONE_API AHoverDroneSpeedLimitBox : public AVolume
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly)
	int32 MaxAllowedSpeedIndex;
};
