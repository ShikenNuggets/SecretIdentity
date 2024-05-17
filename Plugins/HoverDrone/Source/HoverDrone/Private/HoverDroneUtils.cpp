// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDroneUtils.h"
#include "HoverDroneSpeedLimitBox.h"
#include "HoverDroneVolumeManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Volume.h"
#include "Engine/World.h"
#include "Engine/BlockingVolume.h"

namespace UEHoverDrone
{
	float MeasureAltitude(const AActor* Actor, FVector Offset)
	{
		if (Actor)
		{
			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(Reverb_HoverDrone_MeasureAltitude), true, Actor);
			FHitResult Hit;

			const FVector TraceStart = Actor->GetActorLocation() + Offset;
			const FVector TraceEnd = TraceStart - FVector::UpVector * 100000.f;
			const bool bHit = Actor->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams);
			if (bHit)
			{
		//		DrawDebugSphere(PawnOwner->GetWorld(), Hit.ImpactPoint, 30.f, 10, FColor::Yellow, false, 0.5f);
				return (Hit.ImpactPoint - TraceStart).Size();
			}
		}

		return 0.f;
	}

	void LimitControlAccelOnAxis(FVector::FReal& AxisAccel, float AxisPos, float LimitMin, float LimitMax)
	{
		if ((AxisAccel != 0.f) && (LimitMin < LimitMax))
		{
			if (AxisAccel < 0.f)
			{
				// invalidate controls if oob on the min
				if (AxisPos < LimitMin)
				{
					AxisAccel = 0.f;
				}
			}
			else
			{
				// invalidate controls if oob on the max
				if (AxisPos > LimitMax)
				{
					AxisAccel = 0.f;
				}
			}
		}
	}

	template<typename T>
	void FindClosestVolume(const TSet<T*>& Volumes, const FVector PlayerLoc, float& OutDistance, const T*& OutVolume)
	{
		for (const T* DroneVolume : Volumes)
		{
			const FBox TestBox = DroneVolume->GetBounds().GetBox();
			const FVector ClosestPoint = TestBox.GetClosestPointTo(PlayerLoc);
			const float TestDistance = FVector::Distance(PlayerLoc, ClosestPoint);

			if (TestDistance < OutDistance)
			{
				OutDistance = TestDistance;
				OutVolume = DroneVolume;
			}
		}
	};
	
	// TODO: This feels like it belongs in the Simulation.
	int32 ApplyDroneLimiters(const AActor* Actor, FVector& ControlAcceleration)
	{
		if (!Actor)
		{
			return INDEX_NONE;
		}

		UWorld* World = Actor->GetWorld();
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UHoverDroneVolumeManager* VolumeManager = GameInstance->GetSubsystem<UHoverDroneVolumeManager>();

		if (!VolumeManager)
		{
			return INDEX_NONE;
		}

		FVector const PlayerLoc = Actor->GetActorLocation();

		// @todo: On any future project we'll benefit from housing all functionality in a single volume type. This late in the Reverb game, we don't want to update
		// all existing blocking volumes to a new class (and we can't add the speed limiter variable to the existing one), so we add a second kind of volume,
		// which makes the logic more involved. 
		const ABlockingVolume* ClosestBlockVolume = nullptr;
		const AHoverDroneSpeedLimitBox* ClosestSpeedLimitBox = nullptr;
		float ClosestBlockDistance = FLT_MAX;
		float ClosestSpeedLimitDistance = FLT_MAX;

		FindClosestVolume<>(VolumeManager->GetSpeedLimitBoxes(), PlayerLoc, ClosestSpeedLimitDistance, ClosestSpeedLimitBox);
		FindClosestVolume<>(VolumeManager->GetBlockingVolumes(), PlayerLoc, ClosestBlockDistance, ClosestBlockVolume);

		if (ClosestBlockVolume)
		{
			const FBox Bounds = ClosestBlockVolume->GetBounds().GetBox();
			LimitControlAccelOnAxis(ControlAcceleration.X, PlayerLoc.X, Bounds.Min.X, Bounds.Max.X);
			LimitControlAccelOnAxis(ControlAcceleration.Y, PlayerLoc.Y, Bounds.Min.Y, Bounds.Max.Y);
			LimitControlAccelOnAxis(ControlAcceleration.Z, PlayerLoc.Z, Bounds.Min.Z, Bounds.Max.Z);
		}

		if (ClosestSpeedLimitBox)
		{
			TArray<AActor*> LimitBoxOverlaps;
			ClosestSpeedLimitBox->GetOverlappingActors(LimitBoxOverlaps, Actor->GetClass());

			for (AActor* OverlappedActor : LimitBoxOverlaps)
			{
				if (OverlappedActor == Actor)
				{
					return ClosestSpeedLimitBox->MaxAllowedSpeedIndex;
				}
			}
		}

		return INDEX_NONE;
	}
}