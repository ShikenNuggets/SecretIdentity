// Copyright Carter Rennick, 2024. All Rights Reserved.


#include "EnemyAIController.h"

#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Characters/PlayableCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	/*if (PerceptionComponent != nullptr)
	{
		UAISenseConfig config = UAISenseConfig();
		PerceptionComponent->ConfigureSense(config);
	}*/
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	/*LOG_MSG("EnemyAIController - BeginPlay");

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayableCharacter::StaticClass(), FoundActors);

	AActor* Nearest = nullptr;
	if (FoundActors.Num() > 0)
	{
		Nearest = FoundActors[0];
	}

	if (Nearest != nullptr)
	{
		LOG_MSG("Moving towards player...");
		MoveToActor(Nearest);
	}
	else
	{
		LOG_MSG_WARNING("Could not find the nearest player character");
	}*/
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayableCharacter::StaticClass(), FoundActors);

	AActor* Nearest = nullptr;
	if (FoundActors.Num() > 0)
	{
		Nearest = FoundActors[0];
	}

	if (Nearest != nullptr)
	{
		MoveToActor(Nearest);
	}
	else
	{
		LOG_MSG_WARNING("Could not find the nearest player character");
	}*/
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	LOG_MSG("EnemyAIController - OnPossess");
	RunBehaviorTree(EnemyBehaviorTree);
}