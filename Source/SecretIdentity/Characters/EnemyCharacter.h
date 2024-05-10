// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "EnemyCharacter.generated.h"

class AEnemyCharacter;
class UCombatSkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyCharacterDeathDelegate, AEnemyCharacter*);

UCLASS()
class SECRETIDENTITY_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintCallable)
	void UpdateWalkSpeed(float NewWalkSpeed);

	FEnemyCharacterDeathDelegate OnDeathDelegate;

protected:
	virtual void BeginPlay() override;

private:
	UCombatSkeletalMeshComponent* uCombatMeshComponent;

	void OnDeath();
};