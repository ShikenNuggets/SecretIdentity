// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayableCharacterController.h"

#include "Blueprint/UserWidget.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/GameModes/ArcadeGameMode.h"

// Called when the game starts or when spawned
void APlayableCharacterController::BeginPlay()
{
	AArcadeGameMode* GameMode = Cast<AArcadeGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode != nullptr)
	{
		GameMode->OnStartMenuState.AddDynamic(this, &APlayableCharacterController::OnStartMenuState);
		GameMode->OnStartPlayState.AddDynamic(this, &APlayableCharacterController::OnStartPlayState);

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
	}

	if (HudWidgetClass != nullptr)
	{
		uHudWidget = CreateWidget(this, HudWidgetClass);
		if (uHudWidget != nullptr)
		{
			uHudWidget->AddToViewport();
		}
	}

	WARN_IF_NULL(HudWidgetClass);
	WARN_IF_NULL(uHudWidget);
}

void APlayableCharacterController::OnStartMenuState(APawn* NewPawn)
{
	bShowMouseCursor = true;

	if (NewPawn != nullptr)
	{
		Possess(NewPawn);
	}
}

void APlayableCharacterController::OnStartPlayState(APawn* NewPawn)
{
	WARN_IF_NULL(NewPawn);

	bShowMouseCursor = false;

	if (NewPawn != nullptr)
	{
		Possess(NewPawn);
	}
}