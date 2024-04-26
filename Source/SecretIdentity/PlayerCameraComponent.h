// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "SecretIdentity.h"
#include "PlayerCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayerCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:
	UPlayerCameraComponent();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnPlayerStateChanged(ControlState State);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float DefaultFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FlightFOV = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FOVChangeTime;

	bool bHasTargetFOV;
	float fStartFOV;
	float fTargetFOV;
	float fTimer;

	void SetTargetFOV(float Target);
};
