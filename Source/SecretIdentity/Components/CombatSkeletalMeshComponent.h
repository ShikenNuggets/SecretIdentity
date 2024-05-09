// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

#include "CombatSkeletalMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UCombatSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UCombatSkeletalMeshComponent();

	void EnableRagdoll();
};
