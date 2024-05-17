// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldAudioMassProcessor.h"
#include "WorldAudioDataSystem.h"
#include "WorldAudioMassEntityTrait.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "MassCommonFragments.h"
#include "MassCrowdFragments.h"
#include "MassEntityManager.h"
#include "MassMovementFragments.h"
#include "MassRepresentationFragments.h"
#include "MassTrafficFragments.h"
#include "MassLODFragments.h"

UWorldAudioMassProcessor::UWorldAudioMassProcessor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NearbyTrafficVehicleEntityQuery(*this)
	, CrowdAgentEntityQuery(*this)
	, DummyWorldAccessingQuery(*this)
{
	bRequiresGameThreadExecution = true; // due to mutating access to UWorldAudioDataSubsystem
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Client);
	ExecutionOrder.ExecuteInGroup = TEXT("WorldAudioData");
	ExecutionOrder.ExecuteAfter.Add(TEXT("Traffic")); 
}

void UWorldAudioMassProcessor::ConfigureQueries()
{
	// temporary query to mark this processor as world accessing in terms of requirements and their thread-safety
	DummyWorldAccessingQuery.RequireMutatingWorldAccess();

	// Medium or High LOD traffic vehicle agents which should be near the player
	NearbyTrafficVehicleEntityQuery.AddTagRequirement<FMassTrafficVehicleTag>(EMassFragmentPresence::All);
	NearbyTrafficVehicleEntityQuery.AddRequirement<FMassTrafficPIDVehicleControlFragment>(EMassFragmentAccess::None); // Only Medium & High simulation LOD vehicles have PID control fragments
	NearbyTrafficVehicleEntityQuery.AddRequirement<FMassTrafficVehicleControlFragment>(EMassFragmentAccess::ReadOnly);
	NearbyTrafficVehicleEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	NearbyTrafficVehicleEntityQuery.AddRequirement<FMassTrafficVehicleDamageFragment>(EMassFragmentAccess::ReadOnly);
	NearbyTrafficVehicleEntityQuery.AddRequirement<FMassViewerInfoFragment>(EMassFragmentAccess::ReadOnly); 
	NearbyTrafficVehicleEntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	NearbyTrafficVehicleEntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
	NearbyTrafficVehicleEntityQuery.AddConstSharedRequirement<FWorldAudioDataAudioControllerParameters>();

	// Crowd query
	CrowdAgentEntityQuery.AddTagRequirement<FMassCrowdTag>(EMassFragmentPresence::All);
	CrowdAgentEntityQuery.AddTagRequirement<FMassVisibilityCulledByDistanceTag>(EMassFragmentPresence::None); // Cull out everything that is not even close to be visible
	CrowdAgentEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	CrowdAgentEntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);

	ProcessorRequirements.AddSubsystemRequirement<USoundscapeSubsystem>(EMassFragmentAccess::ReadWrite);
	ProcessorRequirements.AddSubsystemRequirement<UWorldAudioDataSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UWorldAudioMassProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	USoundscapeSubsystem* SoundscapeSubsystem = Context.GetMutableSubsystem<USoundscapeSubsystem>();
	if (!SoundscapeSubsystem)
	{
		return;
	}
	
	if (!MovingVehicleColorPoint.IsValid())
	{
		UE_LOG(LogWorldAudioDataSystem, Error, TEXT("Invalid gameplay tag set for UWorldAudioMassProcessor::MovingVehicleColorPoint"));
		return;
	}
	if (!StoppedVehicleColorPoint.IsValid())
	{
		UE_LOG(LogWorldAudioDataSystem, Error, TEXT("Invalid gameplay tag set for UWorldAudioMassProcessor::StoppedVehicleColorPoint"));
		return;
	}
	if (!MovingPedestrianColorPoint.IsValid())
	{
		UE_LOG(LogWorldAudioDataSystem, Error, TEXT("Invalid gameplay tag set for UWorldAudioMassProcessor::MovingPedestrianColorPoint"));
		return;
	}
	if (!StoppedPedestrianColorPoint.IsValid())
	{
		UE_LOG(LogWorldAudioDataSystem, Error, TEXT("Invalid gameplay tag set for UWorldAudioMassProcessor::StoppedPedestrianColorPoint"));
		return;
	}

	bool bUpdateColorPointHashMapCollection = false;

	// Countdown timer
	if (ColorPointHashUpdateTimeSecondsRemaining > 0.0f)
	{
		ColorPointHashUpdateTimeSecondsRemaining -= Context.GetDeltaTimeSeconds();
	}
	else
	{
		// Allow update to proceed and reset countdown timer
		ColorPointHashUpdateTimeSecondsRemaining += ColorPointHashUpdateTimeSeconds;
		bUpdateColorPointHashMapCollection = true;
	}

	// Create or reset color point hash map
	if (ColorPointHashMapCollection)
	{
		ColorPointHashMapCollection->ClearColorPointHashMapCollection();
	}
	else
	{
		ColorPointHashMapCollection = NewObject<USoundscapeColorPointHashMapCollection>(this);
		ColorPointHashMapCollection->InitializeCollection();
		
		SoundscapeSubsystem->AddColorPointHashMapCollection(ColorPointHashMapCollection);
	}

	// Reset scratch buffers
	MovingVehicleColorPointLocations.Reset();
	StoppedVehicleColorPointLocations.Reset();
	MovingPedestrianColorPointLocations.Reset();
	StoppedPedestrianColorPointLocations.Reset();
	IndividuallyAudibleVehicles.Reset();

	// Find traffic vehicle agents
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("Vehicles"))

		NearbyTrafficVehicleEntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& QueryContext)
		{
			const FWorldAudioDataAudioControllerParameters& AudioControllerSharedFragment = QueryContext.GetConstSharedFragment<FWorldAudioDataAudioControllerParameters>();  
			const TConstArrayView<FTransformFragment> TransformFragments = QueryContext.GetFragmentView<FTransformFragment>();
			const TConstArrayView<FMassTrafficVehicleControlFragment> VehicleControlFragments = QueryContext.GetFragmentView<FMassTrafficVehicleControlFragment>();
			const TConstArrayView<FMassTrafficVehicleDamageFragment> VehicleDamageFragments = QueryContext.GetFragmentView<FMassTrafficVehicleDamageFragment>();
			const TConstArrayView<FMassViewerInfoFragment> ViewerInfoFragments = QueryContext.GetFragmentView<FMassViewerInfoFragment>();
			const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODFragments = QueryContext.GetFragmentView<FMassRepresentationLODFragment>();
			const TConstArrayView<FMassVelocityFragment> LinearVelocityFragments = QueryContext.GetFragmentView<FMassVelocityFragment>();

			const int32 NumEntities = QueryContext.GetNumEntities();
			for (int32 EntityIndex = 0; EntityIndex < NumEntities; EntityIndex++)
			{
				const FTransformFragment& TransformFragment = TransformFragments[EntityIndex];
				const FMassTrafficVehicleControlFragment& VehicleControlFragment = VehicleControlFragments[EntityIndex];
				const FMassTrafficVehicleDamageFragment& VehicleDamageFragment = VehicleDamageFragments[EntityIndex];
				const FMassViewerInfoFragment& ViewerInfoFragment = ViewerInfoFragments[EntityIndex];
				const FMassRepresentationLODFragment& RepresentationLODFragment = RepresentationLODFragments[EntityIndex];
				const FMassVelocityFragment& LinearVelocityFragment = LinearVelocityFragments[EntityIndex];

				// Add moving or stopped traffic vehicle color point location
				FVector Location = TransformFragment.GetTransform().GetLocation();
				if (VehicleControlFragment.Speed >= VehicleMovingSpeedThreshold)
				{
					MovingVehicleColorPointLocations.Add(Location);
				}
				else
				{
					StoppedVehicleColorPointLocations.Add(Location);
				}

				// Capture vehicle info for higher fidelity sound on the nearest vehicles. This list is later sorted and
				// culled 
				IndividuallyAudibleVehicles.Add({
					/*Id*/ QueryContext.GetEntity(EntityIndex).Index,
					/*ClosestViewerDistanceSq*/ ViewerInfoFragment.ClosestViewerDistanceSq,
					/*LODSignificance*/RepresentationLODFragment.LODSignificance,
					/*AudioController*/ AudioControllerSharedFragment.AudioController,
					/*Location*/ Location,
					/*LinearVelocity*/ LinearVelocityFragment.Value,
					/*VehicleDamageState*/ static_cast<uint8>(VehicleDamageFragment.VehicleDamageState)
				});
			}
		});
	}

	if (bUpdateColorPointHashMapCollection)
	{
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("Crowd"))

			// Find crowd agents
			CrowdAgentEntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& QueryContext)
			{
				const TConstArrayView<FTransformFragment> TransformFragments = QueryContext.GetFragmentView<FTransformFragment>();
				const TConstArrayView<FMassMoveTargetFragment> MoveTargetFragments = QueryContext.GetFragmentView<FMassMoveTargetFragment>();

				const int32 NumEntities = QueryContext.GetNumEntities();
				for (int32 EntityIndex = 0; EntityIndex < NumEntities; EntityIndex++)
				{
					const FTransformFragment& TransformFragment = TransformFragments[EntityIndex];
					const FMassMoveTargetFragment& MoveTargetFragment = MoveTargetFragments[EntityIndex];

					// Add moving or stopped crowd color point location
					switch (MoveTargetFragment.GetCurrentAction())
					{
						case EMassMovementAction::Move:
							MovingPedestrianColorPointLocations.Add(TransformFragment.GetTransform().GetLocation());
							break;
						case EMassMovementAction::Animate:
						case EMassMovementAction::Stand:
							StoppedPedestrianColorPointLocations.Add(TransformFragment.GetTransform().GetLocation());
							break;
						default:
							checkf(false, TEXT("Unsupported movmeent action"));

					}
				}
			});
		}

		// Add color point locations to hash map
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("Update hash map"))
			ColorPointHashMapCollection->AddColorPointArrayToHashMapCollection(MovingVehicleColorPointLocations, MovingVehicleColorPoint);
			ColorPointHashMapCollection->AddColorPointArrayToHashMapCollection(StoppedVehicleColorPointLocations, StoppedVehicleColorPoint);
			ColorPointHashMapCollection->AddColorPointArrayToHashMapCollection(MovingPedestrianColorPointLocations, MovingPedestrianColorPoint);
			ColorPointHashMapCollection->AddColorPointArrayToHashMapCollection(StoppedPedestrianColorPointLocations, StoppedPedestrianColorPoint);
		}
	}

	// Temp Array
	TArray<FWorldAudioDataVehicleInfo> IndividuallyAudibleVehiclesTemp;

	// Remove totalled vehicles from the running
	for(const FWorldAudioDataVehicleInfo& IndividuallyAudibleVehicle : IndividuallyAudibleVehicles)
	{
		if(IndividuallyAudibleVehicle.VehicleDamageState != static_cast<uint8>(EMassTrafficVehicleDamageState::Totaled))
		{
			IndividuallyAudibleVehiclesTemp.Add(IndividuallyAudibleVehicle);
		}
	}

	// Sort NearestVehicles by ClosestViewerDistanceSq and limit to MaxIndividuallyAudibleVehicles
	IndividuallyAudibleVehiclesTemp.Sort([](const FWorldAudioDataVehicleInfo& LHS, const FWorldAudioDataVehicleInfo& RHS)
	{
		return LHS.LODSignificance < RHS.LODSignificance;
	});
	if (IndividuallyAudibleVehiclesTemp.Num() > (MaxIndividuallyAudibleVehicles * 0.5f))
	{
		IndividuallyAudibleVehiclesTemp.SetNum(MaxIndividuallyAudibleVehicles * 0.5f);
	}

	// Sort NearestVehicles by ClosestViewerDistanceSq and limit to MaxIndividuallyAudibleVehicles
	IndividuallyAudibleVehicles.Sort([](const FWorldAudioDataVehicleInfo& LHS, const FWorldAudioDataVehicleInfo& RHS)
	{
		return LHS.ClosestViewerDistanceSq < RHS.ClosestViewerDistanceSq;
	});

	for(auto It = IndividuallyAudibleVehiclesTemp.CreateConstIterator(); It; ++It)
	{
		bool bAddElement = true;

		for (auto Jt = IndividuallyAudibleVehicles.CreateConstIterator(); Jt; ++Jt)
		{
			if (It->Id == Jt->Id)
			{
				bAddElement = false;
				break;
			}
		}

		if(bAddElement)
		{
			IndividuallyAudibleVehicles.Add(*It);
		}
	}

	if (IndividuallyAudibleVehicles.Num() > MaxIndividuallyAudibleVehicles)
	{
		IndividuallyAudibleVehicles.SetNum(MaxIndividuallyAudibleVehicles);
	}

	if (UWorldAudioDataSubsystem* WorldAudioDataSubsystem = Context.GetMutableSubsystem<UWorldAudioDataSubsystem>())
	{
		WorldAudioDataSubsystem->UpdateWorldAudioDataVehAudioControllers(IndividuallyAudibleVehicles);
	}
}
