// Copyright Carter Rennick, 2024. All Rights Reserved.


#include "DefaultGameMode.h"

ADefaultGameMode::ADefaultGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/Characters/BP_PlayableCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	else if(GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, TEXT("Could not find BP_PlayableCharacter"));
	}
}