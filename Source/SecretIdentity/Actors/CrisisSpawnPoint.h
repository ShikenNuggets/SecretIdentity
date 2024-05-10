// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Info.h"

#include "CrisisSpawnPoint.generated.h"

class AArcadeGameMode;

UENUM()
enum class CrisisType : uint8{
	ThugAttack,

	Count
};

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API ACrisisSpawnPoint : public AInfo
{
	GENERATED_BODY()
	
public:
	ACrisisSpawnPoint();

	bool IsCrisisActive() const;
	bool IsCrisisActiveAndNotResolved() const;
	void SpawnCrisis(TSubclassOf<ACharacter> ThugCharacterBP);

	UFUNCTION()
	void OnCrisisActorEndPlay(AActor* ActorDestroyed, EEndPlayReason::Type Reason);

	float GetTimeSinceCrisisStarted() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	CrisisType TypeToSpawn = CrisisType::ThugAttack;

	AArcadeGameMode* aArcadeGameMode = nullptr;

	bool bIsCrisisActive = false;
	bool bIsActiveCrisisResolved = false;
	FDateTime fActiveCrisisStartTime;

	void ActivateCrisis();
	void ResolveCrisis();
};
