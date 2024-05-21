// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"
#include "SecretIdentity/GameModes/DefaultGameMode.h"

#include "ArcadeGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartMenuStateDelegate, APawn*, NewPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartPlayStateDelegate, APawn*, NewPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateCrisisCountDelegate, int, NumActiveCrises);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateFearMeterDelegate, float, FearMeter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpdateSessionTimerDelegate, float, SessionTimeInSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameOverDelegate);

UENUM()
enum class EArcadeGameState : uint8
{
	Menu = 0,
	Play = 1,

	Count
};

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

	UFUNCTION(BlueprintCallable)
	void StartMenuState();

	UFUNCTION(BlueprintCallable)
	void StartPlayState();

	UFUNCTION(BlueprintCallable) FORCEINLINE bool IsInMenuState() const{ return eCurrentState == EArcadeGameState::Menu; }
	UFUNCTION(BlueprintCallable) FORCEINLINE bool IsInPlayState() const{ return eCurrentState == EArcadeGameState::Play; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	EArcadeGameState StartState = EArcadeGameState::Menu;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FStartMenuStateDelegate OnStartMenuState;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FStartPlayStateDelegate OnStartPlayState;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FUpdateCrisisCountDelegate OnUpdateCrisisCount;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FUpdateFearMeterDelegate OnUpdateFearMeter;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FUpdateSessionTimerDelegate OnUpdateSessionTimer;

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Delegates");
	FGameOverDelegate OnGameOver;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu State", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<APawn> MenuPawnBP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Play State", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<APawn> PlayPawnBP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	float StartSpawnTime = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACharacter> ThugEnemyClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crisis", meta = (AllowPrivateAccess = "true"))
	int NumActiveCrises = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	APawn* CurrentPawn = nullptr;

	APawn* aPlayStatePawn = nullptr;

	float fCurrentSpawnTime = StartSpawnTime;
	float fTimer = 0.0f;
	double fCurrentFearPercentage = 0.0f;
	EArcadeGameState eCurrentState = EArcadeGameState::Menu;
	AActor* aPlayerStart = nullptr;
	

	TArray<ACrisisSpawnPoint*> CrisisSpawnPoints;

	void SpawnCrisis();

	int GetNumActiveCrises();
	double GetTotalFear();
	double GetFearPercentage();

	void GameOver();

	void SpawnPlayerPawn(TSubclassOf<APawn> PawnToSpawn, double ZOffset = 0.0);
};
