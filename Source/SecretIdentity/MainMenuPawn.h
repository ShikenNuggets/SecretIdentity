// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MainMenuPawn.generated.h"

class UCameraComponent;

UCLASS()
class SECRETIDENTITY_API AMainMenuPawn : public APawn
{
	GENERATED_BODY()

public:
	AMainMenuPawn();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnBeginTransitionToPlayState(APawn* NewPawn, float TransitionTime);

protected:
	virtual void BeginPlay() override;

	virtual void UnPossessed() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Play State Transition", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* TransitionCurveAsset;

	UCameraComponent* aTransitionTarget = nullptr;
	float fTransitionTime = 0.0f;
	
	float fTransitionTimer = 0.0f;
	FVector fStartLocation = FVector::Zero();
	FRotator fStartRotation = FRotator::ZeroRotator;
};