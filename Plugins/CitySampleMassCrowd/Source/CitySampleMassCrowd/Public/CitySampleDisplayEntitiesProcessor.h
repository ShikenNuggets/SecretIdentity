// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InstancedStruct.h"
#include "MassRepresentationTypes.h"
#include "MassEntityQuery.h"
#include "MassProcessor.h"
#include "MassEntityManager.h"

#include "CitySampleDisplayEntitiesProcessor.generated.h"

class UMassRepresentationSubsystem;
class UMassLODSubsystem;

USTRUCT()
struct FCitySampleDisplayEntitiesConfig
{
	GENERATED_BODY()

	/** Component Tag that will be used to associate LOD config */
	UPROPERTY(EditAnywhere, Category = "CitySample|DisplayEntities", config, meta = (BaseStruct = "/Script/MassEntity.MassTag"))
	FInstancedStruct TagFilter;

	/** Instanced static mesh information for this agent */
	UPROPERTY(EditAnywhere, Category = "CitySample|DisplayEntities", config)
	FStaticMeshInstanceVisualizationDesc StaticMeshInstanceDesc;

	/** Runtime data for this config*/
	FMassEntityQuery EntityQuery;
	FStaticMeshInstanceVisualizationDescHandle StaticMeshDescHandle;
};

/**
 * Display a simple representation of all the mass entities handled in CitySample
 */
 UCLASS()
class CITYSAMPLEMASSCROWD_API UCitySampleDisplayEntitiesProcessor : public UMassProcessor
{
	GENERATED_BODY()

protected:
	virtual void Initialize(UObject& Owner) override;
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	// @todo we do this only because UCitySampleDisplayEntitiesProcessor has a non-standard query setup. This should be 
	// looked into, as in - this type of query setup should also be supported, since it affects dependency calculations now as well.
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const { return false; }

	UPROPERTY(EditAnywhere, Category = "CitySample|DisplayEntities", config)
	TArray<FCitySampleDisplayEntitiesConfig> DisplayConfigs;

	UPROPERTY(Transient)
	UMassRepresentationSubsystem* RepresentationSubsystem;
};
