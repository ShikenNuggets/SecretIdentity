// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleDisplayEntitiesProcessor.h"
#include "MassRepresentationSubsystem.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassRepresentationTypes.h"
#include "MassLODSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace CitySample
{
	int32 GDisplayEntities = 0;
	FAutoConsoleVariableRef CVarCitySampleDisplayEntities(TEXT("CitySample.DisplayEntities"), GDisplayEntities, TEXT("Display all Mass entities that are being updated"));

	float GDisplayEntitiesFarClippingDistance = 300000.0f; // 3km
	FAutoConsoleVariableRef CVarCitySampleDisplayEntitiesFarClippingDistance(TEXT("CitySample.DisplayEntitiesFarClippingDistance"), GDisplayEntitiesFarClippingDistance, TEXT("Far clipping distance when displaying frosty entities"));

	float GDisplayEntitiesFOVAngleInDegrees = 50.0f; // degrees
	FAutoConsoleVariableRef CVarCitySampleFOVAngleInDegrees(TEXT("CitySample.DisplayEntitiesFOVAngleInDegrees"), GDisplayEntitiesFOVAngleInDegrees, TEXT("FOV half angle in degrees to cull entities"));
}

namespace UE::MassCrowd
{
	extern MASSCROWD_API int32 GCrowdTurnOffVisualization;
}

namespace UE::MassTraffic
{
	extern MASSTRAFFIC_API int32 GTrafficTurnOffVisualization;
}

void UCitySampleDisplayEntitiesProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
	this->ProcessingPhase = EMassProcessingPhase::EndPhysics;

	RepresentationSubsystem = UWorld::GetSubsystem<UMassRepresentationSubsystem>(Owner.GetWorld());
	ensureMsgf(RepresentationSubsystem != nullptr || Owner.GetWorld() == nullptr, TEXT("If the World is not Null we expect to have a RepresentationSubsystem instance available"));
	if (RepresentationSubsystem)
	{
		for (FCitySampleDisplayEntitiesConfig& Config : DisplayConfigs)
		{
			Config.StaticMeshDescHandle = RepresentationSubsystem->FindOrAddStaticMeshDesc(Config.StaticMeshInstanceDesc);
		}
	}
}

template <typename T>
void ConditionallyAddRequirement(FMassFragmentRequirements& MassRequirements, const EMassFragmentAccess AccessMode)
{
	TConstArrayView<FMassFragmentRequirementDescription> Requirements = MassRequirements.GetFragmentRequirements();
	const bool bDoesNotHaveRequirement = Requirements.FindByPredicate([](const FMassFragmentRequirementDescription& Item)
		{ return Item.StructType == T::StaticStruct(); }) == nullptr;
	if (bDoesNotHaveRequirement)
	{
		MassRequirements.AddRequirement<T>(AccessMode);
	}
}


void UCitySampleDisplayEntitiesProcessor::ConfigureQueries()
{
	for (FCitySampleDisplayEntitiesConfig& Config : DisplayConfigs)
	{
		if (Config.EntityQuery.GetFragmentRequirements().Num() == 0)
		{
			ConditionallyAddRequirement<FTransformFragment>(Config.EntityQuery,EMassFragmentAccess::ReadOnly);
			if (Config.TagFilter.GetScriptStruct())
			{
				Config.EntityQuery.AddTagRequirement(*Config.TagFilter.GetScriptStruct(), EMassFragmentPresence::All);
			}
		}
	}

	ProcessorRequirements.AddSubsystemRequirement<UMassLODSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UCitySampleDisplayEntitiesProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE::MassTraffic::GTrafficTurnOffVisualization = UE::MassCrowd::GCrowdTurnOffVisualization = CitySample::GDisplayEntities;
	if (!CitySample::GDisplayEntities)
	{
		return;
	}

	const UMassLODSubsystem& LODSubsystem = Context.GetSubsystemChecked<UMassLODSubsystem>();

	FVector LocalViewerLocation;
	FVector LocalViewerDirection;
	const float CosFOVAngleToDriveVisibility = FMath::Cos(FMath::DegreesToRadians(CitySample::GDisplayEntitiesFOVAngleInDegrees));
	const float MaxRenderDistSquared = FMath::Square(CitySample::GDisplayEntitiesFarClippingDistance);
	const TArray<FViewerInfo>& Viewers = LODSubsystem.GetViewers();
	for (const FViewerInfo& Viewer : Viewers)
	{
		const APlayerController* ViewerAsPlayerController = Viewer.GetPlayerController();
		if (ViewerAsPlayerController && ViewerAsPlayerController->IsLocalController())
		{
			FVector PlayerCameraLocation(ForceInitToZero);
			FRotator PlayerCameraRotation(FRotator::ZeroRotator);
			ViewerAsPlayerController->GetPlayerViewPoint(PlayerCameraLocation, PlayerCameraRotation);
			LocalViewerLocation = PlayerCameraLocation;
			LocalViewerDirection = PlayerCameraRotation.Vector();
			break;
		}
	}

	check(RepresentationSubsystem);
	FMassInstancedStaticMeshInfoArrayView ISMInfo = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

	// @hacky solution to how UCitySampleDisplayEntitiesProcessor's queries are configured. DO NOT DUPLICATE THE PATTERN.
	// The issue boils down to queries not being registered (or even registrable) as queries owned by UCitySampleDisplayEntitiesProcessor.
	// Registrable queries are required to be direct class members of a given processor. Queries in DisplayConfigs are
	// stored in array which breaks this requirement. 
	// UCitySampleDisplayEntitiesProcessor::Execute is called as part of Phase Processing, which runs in EMassExecutionContextType::Processor 
	// mode) and as such requires queries run to be owned by processors in order for us to properly arrange execution order.
	Context.SetExecutionType(EMassExecutionContextType::Local);

	for (FCitySampleDisplayEntitiesConfig& Config : DisplayConfigs)
	{
		Config.EntityQuery.ForEachEntityChunk(EntityManager, Context, [&Config, &ISMInfo, &LocalViewerLocation, &LocalViewerDirection, &CosFOVAngleToDriveVisibility, &MaxRenderDistSquared](FMassExecutionContext& Context)
		{
			TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();

			const int32 NumEntities = Context.GetNumEntities();
			for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				const FTransformFragment& TransformFragment = TransformList[EntityIdx];
				const FVector ViewerToEntity = TransformFragment.GetTransform().GetLocation() - LocalViewerLocation;
				const float DistToViewerSquared = ViewerToEntity.SizeSquared();
				if (DistToViewerSquared > MaxRenderDistSquared)
				{
					continue;
				}

				const FVector ViewerToEntityNorm = ViewerToEntity * FMath::InvSqrt(DistToViewerSquared);
				const float Dot = LocalViewerDirection | ViewerToEntityNorm;

				// Skip entities not in frustum
				if (Dot < CosFOVAngleToDriveVisibility)
				{
					continue;
				}

				if (ISMInfo[Config.StaticMeshDescHandle.ToIndex()].ShouldUseTransformOffset())
				{
					const FTransform& TransformOffset = ISMInfo[Config.StaticMeshDescHandle.ToIndex()].GetTransformOffset();
					const FTransform SMTransform = TransformOffset * TransformFragment.GetTransform();

					ISMInfo[Config.StaticMeshDescHandle.ToIndex()].AddBatchedTransform(Context.GetEntity(EntityIdx), SMTransform, SMTransform, 0.0f);
				}
				else
				{
					ISMInfo[Config.StaticMeshDescHandle.ToIndex()].AddBatchedTransform(Context.GetEntity(EntityIdx)
						, TransformFragment.GetTransform(), TransformFragment.GetTransform(), 0.0f);
				}
			}
		});
	}

	Context.SetExecutionType(EMassExecutionContextType::Processor);
}
