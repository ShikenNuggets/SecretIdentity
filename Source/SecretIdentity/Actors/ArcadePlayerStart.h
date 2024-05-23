// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "ArcadePlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API AArcadePlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	FORCEINLINE constexpr FVector GetPlayerSpawnOffset() const{ return PlayerSpawnOffset; }
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Play State", meta = (AllowPrivateAccess = "true"))
	FVector PlayerSpawnOffset = FVector(0.0f, 0.0f, -2000.0f); //TODO - Add some kind of visual representation for this in the editor
};
