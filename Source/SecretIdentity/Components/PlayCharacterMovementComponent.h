// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "SecretIdentity/SecretIdentity.h"

#include "PlayCharacterMovementComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerFlightMaxSpeedDelegate, bool); //bIsNearMaxSpeed

UCLASS()
class SECRETIDENTITY_API UPlayCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UPlayCharacterMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnPlayerStateChanged(EPlayerControlState State);

	FPlayerFlightMaxSpeedDelegate OnFlightSpeedChanged;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float JogSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultSprintMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float DefaultJumpForce = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float MaxFlightForwardSpeed = 7500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float MaxSpeedEnterThreshold = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character Movement", meta = (AllowPrivateAccess = "true"))
	float MaxSpeedExitThreshold = 0.3f;

	float fDefaultMaxAcceleration;
	bool bIsNearMaxFlightSpeed;
};