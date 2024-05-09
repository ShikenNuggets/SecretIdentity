// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "DefaultGameMode.h"

#include "SecretIdentity/UE_Helpers.h"

ADefaultGameMode::ADefaultGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/Characters/BP_PlayableCharacter"));
	static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/Blueprints/Controllers/BP_PlayableCharacterController"));

	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	if (PlayerControllerBPClass.Class != NULL)
	{
		PlayerControllerClass = PlayerControllerBPClass.Class;
	}

	WARN_IF_NULL(PlayerPawnBPClass.Class);
	WARN_IF_NULL(PlayerControllerBPClass.Class);
}