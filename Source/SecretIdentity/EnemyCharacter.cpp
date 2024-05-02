// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "EnemyCharacter.h"

#include "CombatSkeletalMeshComponent.h"
#include "UE_Helpers.h"

AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCombatSkeletalMeshComponent>(ACharacter::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	uCombatMeshComponent = Cast<UCombatSkeletalMeshComponent>(GetMesh());
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	WARN_IF_NULL(uCombatMeshComponent);
}

void AEnemyCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (OtherActor != nullptr)
	{
		if (uCombatMeshComponent != nullptr)
		{
			uCombatMeshComponent->EnableRagdoll();
		}
	}
}