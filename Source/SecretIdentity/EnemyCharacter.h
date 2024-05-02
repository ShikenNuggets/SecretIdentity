// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UCombatSkeletalMeshComponent;

UCLASS()
class SECRETIDENTITY_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;

private:
	UCombatSkeletalMeshComponent* uCombatMeshComponent;
};