// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleHLODBuilderMeshApproximate.h"

#include "Algo/Transform.h"

#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHandle.h"
#include "WorldPartition/HLOD/HLODActor.h"
#include "WorldPartition/HLOD/HLODSourceActorsFromCell.h"

#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"

#include "AssetCompilingManager.h"

UCitySampleHLODBuilderMeshApproximate::UCitySampleHLODBuilderMeshApproximate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TArray<UActorComponent*> UCitySampleHLODBuilderMeshApproximate::Build(const FHLODBuildContext& InHLODBuildContext, const TArray<UActorComponent*>& InSourceComponents) const
{
	UWorldPartition* WorldPartition = InHLODBuildContext.World->GetWorldPartition();

	TSet<AActor*> SourceActors;
	
	// Filter the input components
	for (UActorComponent* SourceComponent : InSourceComponents)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SourceComponent))
		{
			if (PrimitiveComponent->HLODBatchingPolicy == EHLODBatchingPolicy::None)
			{
				SourceActors.Add(PrimitiveComponent->GetOwner());
			}
		}
	}

	// Building actors starts with "BLDG_N", collision actors with "BLDG_COLL_N"...
	const TCHAR* BuildingActorLabelPrefix = TEXT("BLDG_N");
	const TCHAR* BuildingCollLabelPrefix = TEXT("BLDG_COLL_N");
	
	TMap<FName, FGuid> BuildingsCollisions;
	for (UActorDescContainerInstance::TIterator<> ActorDescIterator(WorldPartition->GetActorDescContainerInstance()); ActorDescIterator; ++ActorDescIterator)
	{
		if (ActorDescIterator->GetActorLabel().ToString().StartsWith(BuildingCollLabelPrefix))
		{
			BuildingsCollisions.Emplace(ActorDescIterator->GetActorLabel(), ActorDescIterator->GetGuid());
		}
	}

	// Extract building collision meshes & use them as input for the mesh approximation algo.
	TArray<FWorldPartitionReference> CollisionSubActors;
	for (AActor* SourceActor : SourceActors)
	{
		if (AWorldPartitionHLOD* HLODActor = Cast<AWorldPartitionHLOD>(SourceActor))
		{
			if (UWorldPartitionHLODSourceActorsFromCell* HLODSourceActors = Cast<UWorldPartitionHLODSourceActorsFromCell>(HLODActor->GetSourceActors()))
			{
				for (const FWorldPartitionRuntimeCellObjectMapping& SubActor : HLODSourceActors->GetActors())
				{
					if (!SubActor.ContainerID.IsMainContainer())
					{
						continue;
					}

					FWorldPartitionReference ActorRef(WorldPartition, SubActor.ActorInstanceGuid);
					if (ActorRef.IsValid())
					{
						FWorldPartitionActorDescInstance* ActorDescInstance = ActorRef.GetInstance();

						if (ActorDescInstance->GetActorLabel().ToString().StartsWith(BuildingActorLabelPrefix))
						{
							FName CollisionActorLabel(ActorDescInstance->GetActorLabel().ToString().Replace(BuildingActorLabelPrefix, BuildingCollLabelPrefix));

							FGuid CollisionActorGuid = BuildingsCollisions.FindRef(CollisionActorLabel);
							if (CollisionActorGuid.IsValid())
							{
								FWorldPartitionReference CollisionActorRef(WorldPartition, CollisionActorGuid);
								if (CollisionActorRef.IsValid())
								{
									CollisionSubActors.Add(MoveTemp(CollisionActorRef));
								}
							}
						}
					}
				}
			}
		}
	}

	TArray<UActorComponent*> SourceComponents = InSourceComponents;

	if (!CollisionSubActors.IsEmpty())
	{
		// Before capturing the scene, make sure all assets are finished compiling
		FAssetCompilingManager::Get().FinishAllCompilation();

		Algo::Transform(CollisionSubActors, SourceComponents, [](const FWorldPartitionReference& Ref) { return Ref.GetActor()->FindComponentByClass<UPrimitiveComponent>(); });
	}

	return Super::Build(InHLODBuildContext, SourceComponents);
}
