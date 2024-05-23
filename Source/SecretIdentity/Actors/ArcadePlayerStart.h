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
	FORCEINLINE FRotator GetPlayerSpawnRotationOffset() const{ return PlayerSpawnRotationOffset; }
	
private:
	//TODO - Show some kind of visual representation for these in the editor
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = "Player Spawn", meta = (AllowPrivateAccess = "true"))
	FVector PlayerSpawnOffset = FVector(0.0f, 0.0f, -2000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Spawn", meta = (AllowPrivateAccess = "true"))
	FRotator PlayerSpawnRotationOffset = FRotator::ZeroRotator;
};
