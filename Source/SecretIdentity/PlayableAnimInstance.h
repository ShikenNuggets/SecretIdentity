// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayableAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essential Movement Data", meta = (AllowPrivateAccess = "true"))
	bool IsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essential Movement Data", meta = (AllowPrivateAccess = "true"))
	float FlightMoveSpeed = 0.0f;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essential Movement Data", meta = (AllowPrivateAccess = "true"))
	float SprintSpeedIncreasor = 0.25f;
};