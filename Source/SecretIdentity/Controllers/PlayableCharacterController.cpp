// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayableCharacterController.h"

#include "Blueprint/UserWidget.h"

#include "SecretIdentity/UE_Helpers.h"

// Called when the game starts or when spawned
void APlayableCharacterController::BeginPlay()
{
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