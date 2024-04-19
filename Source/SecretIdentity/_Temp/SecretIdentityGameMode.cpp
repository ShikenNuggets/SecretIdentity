// Copyright Epic Games, Inc. All Rights Reserved.

#include "SecretIdentityGameMode.h"
#include "SecretIdentityCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASecretIdentityGameMode::ASecretIdentityGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/_Temp/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}