// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "SecretIdentity/SecretIdentity.h"

#include "PlayCharacterMovementComponent.generated.h"

struct FPlayCharacterMovementOptions
{
	float JogSpeed = 600.0f;
	float DefaultSprintMultiplier = 2.5f;
	float DefaultJumpForce = 1200.0f;
	float DefaultAirControl = 0.35f;
	float MaxFlightForwardSpeed = 20000.0f;
	float FlightStrafeRotationTime = 0.15f;
	float FlightForwardRotationTime = 0.5f;
};

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UPlayCharacterMovementComponent();

	void SetOptions(const FPlayCharacterMovementOptions& Options);
	void OnPlayerStateChanged(EPlayerControlState State);

private:
	FPlayCharacterMovementOptions fOptions;
};