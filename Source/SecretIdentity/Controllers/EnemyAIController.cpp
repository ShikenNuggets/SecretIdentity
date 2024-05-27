// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "EnemyAIController.h"

#include "Kismet/GameplayStatics.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISenseConfig.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Characters/PlayableCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (PerceptionComponent != nullptr)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnTargetPerceptionUpdated);
	}
	
	WARN_IF_NULL(GetWorld());
	WARN_IF_NULL(PerceptionComponent);
	WARN_IF_NULL(EnemyBehaviorTree);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RunBehaviorTree(EnemyBehaviorTree);
}

void AEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	WARN_IF_NULL(Actor);
	WARN_IF_NULL(GetWorld());

	if (Actor == nullptr)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed() && Actor->ActorHasTag(TEXT("Player")))
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(EnemyTimerHandle);
		}
		
		SetBlackboardValues(true, Actor);
	}
	else
	{
		if (GetWorld())
		{
			FTimerDynamicDelegate delegate;
			delegate.BindUFunction(this, TEXT("OnStartEnemyTimer"));
			GetWorld()->GetTimerManager().SetTimer(EnemyTimerHandle, delegate, fLineOfSightTimer, false);
		}
	}
}

void AEnemyAIController::OnStartEnemyTimer()
{
	SetBlackboardValues(false, nullptr);
}

void AEnemyAIController::SetBlackboardValues(bool HasLineOfSight, AActor* TargetActor)
{
	if (Blackboard != nullptr)
	{
		Blackboard->SetValueAsBool(TEXT("HasLineOfSight"), HasLineOfSight);
		Blackboard->SetValueAsObject(TEXT("TargetActor"), TargetActor);
	}
}