// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "CombatColliderComponent.generated.h"

//Collider specifically used for combat
UCLASS()
class SECRETIDENTITY_API UCombatColliderComponent : public USphereComponent
{
	GENERATED_BODY()
	
public:
	UCombatColliderComponent();

	void Enable();
	void Disable();
};
