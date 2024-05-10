// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"
#include "SecretIdentity/GameModes/DefaultGameMode.h"

#include "ArcadeGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateCrisisCountDelegate, int, NumActiveCrises);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateFearMeterDelegate, float, FearMeter);

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API AArcadeGameMode : public ADefaultGameMode
{
	GENERATED_BODY()
	
public:
	AArcadeGameMode();

	UFUNCTION()
	void OnCrisisResolved(ACrisisSpawnPoint* CSP);

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FUpdateCrisisCountDelegate OnUpdateCrisisCount;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FUpdateFearMeterDelegate OnUpdateFearMeter;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	float StartSpawnTime = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACharacter> ThugEnemyClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	int NumActiveCrises = 0;

	float fCurrentSpawnTime = StartSpawnTime;
	float fTimer = 0.0f;

	TArray<ACrisisSpawnPoint*> CrisisSpawnPoints;

	void SpawnCrisis();

	int GetNumActiveCrises();
	float GetTotalFear();
	float GetFearPercentage();
};
