// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "SecretIdentity/SecretIdentity.h"

#include "PlayCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UPlayCharacterMovementComponent();

	void OnPlayerStateChanged(EPlayerControlState State);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float JogSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultSprintMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultJumpForce = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float MaxFlightForwardSpeed = 20000.0f;
};