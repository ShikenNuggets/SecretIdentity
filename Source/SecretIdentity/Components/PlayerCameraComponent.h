// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"

#include "SecretIdentity/SecretIdentity.h"

#include "PlayerCameraComponent.generated.h"

UCLASS()
class SECRETIDENTITY_API UPlayerCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:
	UPlayerCameraComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnPlayerStateChanged(EPlayerControlState State);
	void OnFlightSpeedChanged(bool IsNearMaxSpeed);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float DefaultFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FlightFOV = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FlightMaxSpeedFOV = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FOVChangeTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FOVMaxSpeedChangeTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FOVChangeDelay = 0.5f;

	bool bHasTargetFOV = false;
	float fStartFOV = DefaultFOV;
	float fTargetFOV = DefaultFOV;
	float fCurrentFOVChangeTime = FOVChangeTime;
	float fTimer = 0.0f;

	void SetTargetFOV(float Target);
	void SetTargetFOVWithDelay(float Target);
};
