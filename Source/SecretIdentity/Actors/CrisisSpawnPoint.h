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

	Count UMETA(Hidden)
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
	bool IsPlayerInRange() const;
	void SpawnCrisis(TSubclassOf<ACharacter> ThugCharacterBP);

	//Overrides cooldown timer and forces all crisis actors to despawn and crisis to deactive immediately
	//This is only meant to be used in specific scenarios, do not call it if you're not 100% clear on what it's for
	//Only works if crisis is already resolved and player is out of range, otherwise does nothing
	void ForceCleanupNow();

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	float CooldownTime = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool DebugOnlyAllowOneSpawn = false;

	bool bIsCrisisActive = false;
	bool bIsActiveCrisisResolved = false;
	bool bIsCleaningUp = false;
	FDateTime fActiveCrisisStartTime;
	TArray<AActor*> tCrisisActors;
	APawn* aPlayerPawn;
	FTimerHandle fCooldownTimerHandle;

#if !UE_BUILD_SHIPPING
	bool bHasSpawnedOneCrisis = false;
#endif

	void ActivateCrisis();
	void ResolveCrisis();
	void DespawnAllCrisisActors();
};
