// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Info.h"

#include "CrisisSpawnPoint.generated.h"

class AArcadeGameMode;
class ACrisisSpawnPoint;
class AEnemyCharacter;

UENUM()
enum class CrisisType : uint8{
	ThugAttack,

	Count
};

DECLARE_MULTICAST_DELEGATE_OneParam(FCrisisResolvedDelegate, ACrisisSpawnPoint*);

//Spawn Point for Crisis Events
UCLASS(Blueprintable)
class SECRETIDENTITY_API ACrisisSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:
	ACrisisSpawnPoint();

	virtual void Tick(float DeltaTime) override;

	bool IsCrisisActive() const;
	bool IsCrisisResolved() const;
	bool IsCrisisActiveAndNotResolved() const;
	void SpawnCrisis(TSubclassOf<ACharacter> ThugCharacterBP);

	UFUNCTION()
	void OnCrisisActorEndPlay(AActor* ActorDestroyed, EEndPlayReason::Type Reason); //To be called when the Crisis actor is destroyed (removed from the level)

	UFUNCTION()
	void OnCrisisActorDead(AEnemyCharacter* Enemy); //To be called when the Crisis actor is dead/defeated, but not yet destroyed

	UFUNCTION()
	void OnPlayStateBegins(APawn* NewPawn);

	double GetSecondsSinceCrisisStarted() const;
	FVector GetAverageCrisisActorLocation() const;

	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetAllCrisisActorPositions() const;

	FCrisisResolvedDelegate OnCrisisResolved;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	CrisisType TypeToSpawn = CrisisType::ThugAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	float DespawnDistanceWhenResolved = 10000.0f;

	bool bIsCrisisActive = false;
	bool bIsActiveCrisisResolved = false;
	bool bIsCleaningUp = false;
	FDateTime fActiveCrisisStartTime;
	TArray<AActor*> tCrisisActors;
	APawn* aPlayerPawn;

	void ActivateCrisis();
	void ResolveCrisis();
	void DespawnAllCrisisActors();
};
