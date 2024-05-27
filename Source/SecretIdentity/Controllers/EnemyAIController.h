// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

struct FAIStimulus;

UCLASS()
class SECRETIDENTITY_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AEnemyAIController();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime);

	virtual void OnPossess(APawn* InPawn);

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnStartEnemyTimer();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* EnemyBehaviorTree = nullptr;

	FTimerHandle EnemyTimerHandle;

	float fLineOfSightTimer = 0.0f;

	void SetBlackboardValues(bool HasLineOfSight, AActor* TargetActor);
};
