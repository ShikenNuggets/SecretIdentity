// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "EnemyCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Components/CombatSkeletalMeshComponent.h"
#include "SecretIdentity/UObjects/PlayableAnimInstance.h"

AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCombatSkeletalMeshComponent>(ACharacter::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	uCombatMeshComponent = Cast<UCombatSkeletalMeshComponent>(GetMesh());

	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->MaxWalkSpeed = 400.0f;
		GetCharacterMovement()->bRequestedMoveUseAcceleration = true;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetMesh() != nullptr)
	{
		uAnimInstance = Cast<UPlayableAnimInstance>(GetMesh()->GetAnimInstance());
	}

	WARN_IF_NULL(GetCharacterMovement());
	WARN_IF_NULL(GetController());
	WARN_IF_NULL(GetCapsuleComponent());
	WARN_IF_NULL(GetMesh());
	WARN_IF_NULL(uCombatMeshComponent);
	WARN_IF_NULL(uAnimInstance);
}

void AEnemyCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (OtherActor != nullptr)
	{
		OnDeath();
	}
}

void AEnemyCharacter::UpdateWalkSpeed(float NewWalkSpeed)
{
	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->MaxWalkSpeed = NewWalkSpeed;
	}
}

void AEnemyCharacter::OnPatrol()
{
	UpdateWalkSpeed(PatrolSpeed);

	if (uAnimInstance != nullptr)
	{
		uAnimInstance->IsShooting = false;
	}
}

void AEnemyCharacter::OnChase()
{
	UpdateWalkSpeed(ChaseSpeed);
}

void AEnemyCharacter::OnAttack()
{
	if (uAnimInstance != nullptr)
	{
		uAnimInstance->IsShooting = true;
	}
}

void AEnemyCharacter::OnDeath()
{
	if (uCombatMeshComponent != nullptr)
	{
		uCombatMeshComponent->EnableRagdoll();
	}

	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	}
	
	if (GetController() != nullptr)
	{
		GetController()->UnPossess();
	}
	
	if (GetCapsuleComponent() != nullptr)
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->Deactivate();
	}

	bIsDead = true;
	OnDeathDelegate.Broadcast(this);
}