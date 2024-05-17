// Copyright Epic Games, Inc. All Rights Reserved.


#include "WorldAudioDataVehAudioController.h"

#include "AudioParameter.h"
#include "WorldAudioDataVehAudioPreset.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ScriptInterface.h"
#include "Sound/SoundBase.h"
#include "MetasoundSource.h"
#include "DrawDebugHelpers.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"



// Sets default values for this component's properties
UWorldAudioDataVehAudioController::UWorldAudioDataVehAudioController()
	: WADVehicleAudioPreset(nullptr)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UWorldAudioDataVehAudioController::SetPreset(UWADVehAudioPreset* VehicleAudioPreset, int32 Seed = INDEX_NONE)
{
	// If preset is invalid, early out
	if(VehicleAudioPreset == nullptr)
	{
		return;
	}

	USoundBase* Sound = Cast<USoundBase>(VehicleAudioPreset->EngineMetaSound);

	if (Sound == nullptr)
	{
		return;
	}

	// If engine sim metasound is invalid, early out
	if(VehicleAudioPreset->EngineMetaSound == nullptr)
	{
		return;
	}

	FVector PlayerPawnLocation = FVector::ZeroVector;

	if(const UWorld* World = GetWorld())
	{
		if(const APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			PlayerPawnLocation = Pawn->GetTransform().GetLocation();
		}
	}

	// Cache input settings
	WADVehicleAudioPreset = VehicleAudioPreset;
	RandomSeed = Seed;
	MinEnginePitchOffset = VehicleAudioPreset->MinEnginePitchOffset;
	MaxEnginePitchOffset = VehicleAudioPreset->MaxEnginePitchOffset;

	SpeedOfSound = FMath::Max(VehicleAudioPreset->SpeedOfSoundMPS * 100.0f, 1.0f);

	// Spawn main engine AC if not already running
	if(MainEngineAudioComponent == nullptr)
	{

		MainEngineAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, Sound, GetComponentTransform().GetLocation(), FRotator::ZeroRotator, 1.0f, CurrentPitchMod, 0.0f);
		MainEngineAudioComponent->FadeIn(2.0f, 1.0f, 0.0f);
		MainEngineAudioComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

	}
	// If AC is already playing a preset, stop and change the preset
	else
	{
		if(MainEngineAudioComponent->GetPlayState() != EAudioComponentPlayState::Stopped)
		{
			MainEngineAudioComponent->Stop();
		}

		MainEngineAudioComponent->SetSound(Sound);
		MainEngineAudioComponent->SetWorldLocation(GetComponentTransform().GetLocation());
		MainEngineAudioComponent->SetPitchMultiplier(CurrentPitchMod);

		MainEngineAudioComponent->FadeIn(2.0f, 1.0f, 0.0f);
	}

	// Call update calculations to set up existing parameters
	UpdateCalculations(0.066, PlayerPawnLocation);

	// Stomp Current Pitch Mod value since the first frame it will be off
	CurrentPitchMod = 1.0f;
}

void UWorldAudioDataVehAudioController::SetVelocity(float InSpeed)
{
	CurrentSpeed = InSpeed;
}

void UWorldAudioDataVehAudioController::StopController()
{
	if(MainEngineAudioComponent)
	{
		MainEngineAudioComponent->FadeOut(1.0f, 0.0f);
	}
}


// Called when the game starts
void UWorldAudioDataVehAudioController::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if(WADVehicleAudioPreset)
	{
		// Spawn main engine AC
		SetPreset(WADVehicleAudioPreset, RandomSeed);
	}
	
}

void UWorldAudioDataVehAudioController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if(MainEngineAudioComponent)
	{
		MainEngineAudioComponent->FadeOut(1.0f, 0.0f);
	}
}

void UWorldAudioDataVehAudioController::ManualUpdate(const float DeltaTime, const FVector PlayerPawnLocation)
{
	// First update calculations
	UpdateCalculations(DeltaTime, PlayerPawnLocation);

	// Next update the main engine metasound
	UpdateEngineMetaSound();

	// Next update the pitch mod simulating doppler shift on all ACs
	UpdateAudioComponentPitchMods();

	// Next update the location of all ACs
	UpdateAudioComponentLocations();

	// Attempt to honk horn @todo determine behavior for horn honk
	// HonkHorn();
}

void UWorldAudioDataVehAudioController::UpdateCalculations(float DeltaTime, const FVector PawnLocation)
{
	// Setup Location Vectors
	const FVector CurrentWorldLocation = GetComponentTransform().GetLocation();
	
	// Find the Distances
	const FVector LocationDifference = CurrentWorldLocation - PawnLocation;
	const float CurrentDistanceFromPawn = LocationDifference.Size();

	// Find the rate of change of distance
	DeltaDistanceFromPawn = PreviousDistanceFromPawn - CurrentDistanceFromPawn;

	// Cache the distance for next frame
	PreviousDistanceFromPawn = CurrentDistanceFromPawn;

	// Find the change in distance over time
	const float DeltaDistanceRate = DeltaDistanceFromPawn / FMath::Clamp(DeltaTime, MinRefreshPeriod, MaxRefreshPeriod);

	// Find the pitch multiplier
	CurrentPitchMod = (DeltaDistanceRate + SpeedOfSound) / FMath::Max(SpeedOfSound, 1.0f);

	//
	
}

void UWorldAudioDataVehAudioController::UpdateEngineMetaSound()
{
	if(WADVehicleAudioPreset == nullptr)
	{
		return;
	}

	if(MainEngineAudioComponent == nullptr)
	{
		return;
	}

	if(MainEngineAudioComponent->GetPlayState() == EAudioComponentPlayState::Stopped
		|| MainEngineAudioComponent->GetPlayState() == EAudioComponentPlayState::Paused)
	{
		return;
	}

	// Batch update parameters on the sound
	MainEngineAudioComponent->SetParameters(
	{
		{ WADVehicleAudioPreset->VelocityInputName, CurrentSpeed },
		{ WADVehicleAudioPreset->RandomSeedInputName, RandomSeed },
		{ WADVehicleAudioPreset->MinEnginePitchOffsetName, MinEnginePitchOffset },
		{ WADVehicleAudioPreset->MaxEnginePitchOffsetName, MaxEnginePitchOffset }
	});
}

void UWorldAudioDataVehAudioController::UpdateAudioComponentPitchMods()
{
	if(HornHonkAudioComponent)
	{
		if(HornHonkAudioComponent->GetPlayState() != EAudioComponentPlayState::Stopped 
			&& HornHonkAudioComponent->GetPlayState() != EAudioComponentPlayState::Paused)
		{
			HornHonkAudioComponent->SetPitchMultiplier(CurrentPitchMod);
		}
	}

	if(MainEngineAudioComponent == nullptr)
	{
		return;
	}

	if (MainEngineAudioComponent->GetPlayState() == EAudioComponentPlayState::Stopped
		|| MainEngineAudioComponent->GetPlayState() == EAudioComponentPlayState::Paused)
	{
		return;
	}

	MainEngineAudioComponent->SetPitchMultiplier(CurrentPitchMod);
}

void UWorldAudioDataVehAudioController::UpdateAudioComponentLocations()
{
	if (HornHonkAudioComponent)
	{
		if (HornHonkAudioComponent->GetPlayState() != EAudioComponentPlayState::Stopped
			&& HornHonkAudioComponent->GetPlayState() != EAudioComponentPlayState::Paused)
		{
			HornHonkAudioComponent->SetWorldLocation(GetComponentTransform().GetLocation());
		}
	}

	if (MainEngineAudioComponent == nullptr)
	{
		return;
	}

	if (MainEngineAudioComponent->GetPlayState() == EAudioComponentPlayState::Stopped
		|| MainEngineAudioComponent->GetPlayState() == EAudioComponentPlayState::Paused)
	{
		return;
	}

	MainEngineAudioComponent->SetWorldLocation(GetComponentTransform().GetLocation());
}

void UWorldAudioDataVehAudioController::HonkHorn()
{
	if(WADVehicleAudioPreset == nullptr)
	{
		return;
	}

	if(WADVehicleAudioPreset->HonkingSound == nullptr)
	{
		return;
	}

	if(PreviousDistanceFromPawn < WADVehicleAudioPreset->HornHonkDistance)
	{
		if (HornHonkAudioComponent == nullptr)
		{

			HornHonkAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, WADVehicleAudioPreset->HonkingSound, GetComponentTransform().GetLocation(), FRotator::ZeroRotator, 1.0f, CurrentPitchMod, 0.0f);
			HornHonkAudioComponent->Play(0.0f);
			HornHonkAudioComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
			HornHonkAudioComponent->bAutoDestroy = true;

		}

	}
}

