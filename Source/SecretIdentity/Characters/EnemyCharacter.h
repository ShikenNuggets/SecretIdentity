// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "EnemyCharacter.generated.h"

class AEnemyCharacter;
class UCombatSkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyCharacterDeathDelegate, AEnemyCharacter*);

//Base Class for Enemy Characters
UCLASS()
class SECRETIDENTITY_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintCallable)
	void UpdateWalkSpeed(float NewWalkSpeed);

	UFUNCTION(BlueprintCallable)
	void OnPatrol();

	UFUNCTION(BlueprintCallable)
	void OnChase();

	bool IsDead() const{ return bIsDead; }

	FEnemyCharacterDeathDelegate OnDeathDelegate;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float PatrolSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float ChaseSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float AttackRangeMin = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float AttackRangeMax = 1000.0f;

	UCombatSkeletalMeshComponent* uCombatMeshComponent;
	bool bIsDead;

	void OnDeath();
};