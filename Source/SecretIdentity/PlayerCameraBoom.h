// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "SecretIdentity.h"
#include "PlayerCameraBoom.generated.h"

class UPlayerCameraComponent;

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayerCameraBoom : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	UPlayerCameraBoom();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnPlayerStateChanged(EPlayerControlState State);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera Boom", meta = (AllowPrivateAccess = "true"))
	float DefaultFollowDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera Boom", meta = (AllowPrivateAccess = "true"))
	float FlightFollowDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera Boom", meta = (AllowPrivateAccess = "true"))
	float FollowDistanceChangeTime;

	bool bHasTargetFollowDistance;
	float fStartFollowDistance;
	float fTargetFollowDistance;
	float fTimer;

	void SetTargetFollowDistance(float Target);
};
