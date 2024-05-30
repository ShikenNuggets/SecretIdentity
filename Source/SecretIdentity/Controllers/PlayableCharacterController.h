// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "PlayableCharacterController.generated.h"

UCLASS()
class SECRETIDENTITY_API APlayableCharacterController : public APlayerController
{
	GENERATED_BODY()

public:
	APlayableCharacterController();
	
	UFUNCTION(BlueprintCallable)
	void OnStartMenuState(APawn* NewPawn);

	UFUNCTION(BlueprintCallable)
	void OnStartPlayState(APawn* NewPawn);

	UFUNCTION(BlueprintCallable)
	void OnGameOverState();

	UFUNCTION(BlueprintCallable)
	void LockCursor(bool Locked);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> HudWidgetClass;

	FInputModeGameAndUI MenuInputLockMode;
	FInputModeGameOnly GameInputLockMode;

	UUserWidget* uHudWidget;
};
