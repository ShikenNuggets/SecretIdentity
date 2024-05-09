// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "PlayableAnimInstance.generated.h"

DECLARE_MULTICAST_DELEGATE(FAnimNotifyDelegate);

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UPlayableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Movement Data ---------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essential Movement Data", meta = (AllowPrivateAccess = "true"))
	bool IsSprinting = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essential Movement Data", meta = (AllowPrivateAccess = "true"))
	float FlightMoveSpeed = 0.0f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Combat Data -----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Data", meta = (AllowPrivateAccess = "true"))
	bool IsPunching = false;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Anim Notify Triggers --------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	FAnimNotifyDelegate OnTriggerEnableHandCollision;
	FAnimNotifyDelegate OnTriggerDisableHandCollision;
	FAnimNotifyDelegate OnTriggerEndPunching;

protected:
	UFUNCTION(BlueprintCallable)
	void TriggerEnableHandCollision(){ OnTriggerEnableHandCollision.Broadcast(); }

	UFUNCTION(BlueprintCallable)
	void TriggerDisableHandCollision(){ OnTriggerDisableHandCollision.Broadcast(); }

	UFUNCTION(BlueprintCallable)
	void TriggerEndPunching(){ OnTriggerEndPunching.Broadcast(); }

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Essential Movement Data", meta = (AllowPrivateAccess = "true"))
	float SprintSpeedIncreasor = 0.25f;
};