// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
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

protected:
	virtual void BeginPlay() override;

public:
	void UpdateTimer(float DeltaTime);
	void SetTargetFOV(float Target);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Camera", meta = (AllowPrivateAccess = "true"))
	float FOVChangeTime;

	bool bHasTargetFOV;
	float fStartFOV;
	float fTargetFOV;
	float fTimer;
};
