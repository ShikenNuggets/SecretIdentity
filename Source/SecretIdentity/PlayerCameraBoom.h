// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "PlayerCameraBoom.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayerCameraBoom : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	UPlayerCameraBoom();

protected:
	virtual void BeginPlay() override;

public:
	void UpdateTimer(float DeltaTime);
	void SetTargetFollowDistance(float Target);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerCameraBoom", meta = (AllowPrivateAccess = "true"))
	float FollowDistanceChangeTime;

	bool bHasTargetFollowDistance;
	float fStartFollowDistance;
	float fTargetFollowDistance;
	float fTimer;
};
