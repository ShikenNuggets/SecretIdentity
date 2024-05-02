// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Field/FieldSystemActor.h"
#include "UE_Helpers.h"
#include "CombatFieldSystemActor.generated.h"

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API ACombatFieldSystemActor : public AFieldSystemActor
{
	GENERATED_BODY()
	
public:
	ACombatFieldSystemActor();

	void SetFieldActive(bool Active);
	void DrawDebugInfo(bool DrawDebug);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool IsActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool IsDebug = false;
};