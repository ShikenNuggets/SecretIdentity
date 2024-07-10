// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayableCharacterController.h"

#include "Blueprint/UserWidget.h"

#include "EnhancedInputSubsystems.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/GameModes/ArcadeGameMode.h"

APlayableCharacterController::APlayableCharacterController()
{
	MenuInputLockMode = FInputModeGameAndUI();
	MenuInputLockMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);

	GameInputLockMode = FInputModeGameOnly();
	GameInputLockMode.SetConsumeCaptureMouseDown(false);
}

// Called when the game starts or when spawned
void APlayableCharacterController::BeginPlay()
{
	AArcadeGameMode* GameMode = Cast<AArcadeGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode != nullptr)
	{
		GameMode->OnStartMenuState.AddDynamic(this, &APlayableCharacterController::OnStartMenuState);
		GameMode->OnStartPlayState.AddDynamic(this, &APlayableCharacterController::OnStartPlayState);
		GameMode->OnGameOver.AddDynamic(this, &APlayableCharacterController::OnGameOverState);
		GameMode->OnPause.AddDynamic(this, &APlayableCharacterController::UnlockCursor);
		GameMode->OnUnpause.AddDynamic(this, &APlayableCharacterController::LockCursor);

		switch (GameMode->StartState)
		{
			case EArcadeGameState::Menu:
				OnStartMenuState(nullptr);
				break;
			case EArcadeGameState::Play:
				OnStartPlayState(nullptr);
				break;
			default:
				WARN_IF_MSG(true, "EArcadeGameState case not handled!");
				break;
		}

		if (HudWidgetClass != nullptr)
		{
			uHudWidget = CreateWidget(this, HudWidgetClass);
			if (uHudWidget != nullptr)
			{
				uHudWidget->AddToViewport();
			}
		}
	}

	WARN_IF_NULL(HudWidgetClass);
	WARN_IF_NULL(uHudWidget);
}

void APlayableCharacterController::OnStartMenuState(APawn* NewPawn)
{
	UnlockCursor();

	if (NewPawn != nullptr)
	{
		Possess(NewPawn);
	}
}

void APlayableCharacterController::OnStartPlayState(APawn* NewPawn)
{
	WARN_IF_NULL(NewPawn);

	LockCursor();

	if (NewPawn != nullptr)
	{
		Possess(NewPawn);
	}
}

void APlayableCharacterController::OnGameOverState()
{
	UnlockCursor();
}

void APlayableCharacterController::LockCursor()
{
	bShowMouseCursor = false;
	SetInputMode(GameInputLockMode);
}

void APlayableCharacterController::UnlockCursor()
{
	bShowMouseCursor = true;
	SetInputMode(MenuInputLockMode);
}