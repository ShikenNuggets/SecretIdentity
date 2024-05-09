// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SecretIdentity/GameModes/DefaultGameMode.h"

#include "ArcadeGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API AArcadeGameMode : public ADefaultGameMode
{
	GENERATED_BODY()
	
public:
	AArcadeGameMode();

protected:
	virtual void BeginPlay() override;
};
