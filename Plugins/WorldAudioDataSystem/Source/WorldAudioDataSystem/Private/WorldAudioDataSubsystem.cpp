// Copyright Epic Games, Inc. All Rights Reserved.


#include "WorldAudioDataSubsystem.h"
#include "WorldAudioDataSettings.h"
#include "WorldAudioDataVehAudioController.h"
#include "WorldAudioDataVehAudioPreset.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/AudioComponent.h"
#include "AudioDevice.h"
#include "WorldAudioDataClusterActor.h"
#include "WorldAudioDataSystem.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Sound/SoundSubmix.h"


void UContinuousSound::InitializeContinuousSound(const UObject* WorldContext, FString InDataKey, USoundBase* Sound)
{
	DataKey = InDataKey;

	if (AudioComponent == nullptr)
	{
		AudioComponent = UGameplayStatics::SpawnSoundAtLocation(WorldContext, Sound, ComponentLocation, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f);
		AudioComponent->FadeIn(2.0f, 1.0f, 0.0f);
	}
}

void UContinuousSound::UpdateAudioComponent(const TArray<FVector>& DataLocations, const FVector ListenerLocation)
{
	FVector CurrentDistance = ListenerLocation + FVector(1000000.0f);
	FVector LastDistance = CurrentDistance;
	FVector LastLocation = FVector::ZeroVector;

	for(auto DataLocation : DataLocations)
	{
		CurrentDistance = DataLocation - ListenerLocation;

		if(LastDistance.Length() >= CurrentDistance.Length())
		{
			LastDistance = CurrentDistance;
			LastLocation = DataLocation;
		}
	}

	ComponentLocation = LastLocation;

	if(AudioComponent)
	{
		AudioComponent->SetWorldLocation(ComponentLocation + FVector(0.0f, 0.0f, 500.0f));
	}
}

const FString& UContinuousSound::GetDataKey() const
{
	return DataKey;
}

bool UContinuousSound::NeedsNewAudioComponent() const
{
	return (AudioComponent == nullptr || AudioComponent->IsPlaying() == false);
}

void UWorldAudioDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("Subsystem initializing"));

	WorldAudioDataSettings = GetDefault<UWorldAudioDataSettings>();

	// Get plugin settings on Subsystem initialization
	if (WorldAudioDataSettings)
	{
		// Get presets for npc Vehicle Audio Controllers
		for (auto It = WorldAudioDataSettings->MASSTrafficCarConfigurationPresetMap.CreateConstIterator(); It; ++It)
		{
			if (UObject* ObjTryLoad = It.Value().TryLoad())
			{
				if (UWADVehAudioPreset* WADVehAudioPreset = Cast<UWADVehAudioPreset>(ObjTryLoad))
				{
					VehAudioControllerPresetMap.Add(It.Key(), WADVehAudioPreset);

					UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("WADVehAudioPreset %s added"), *WADVehAudioPreset->GetFullName());
				}
			}
		}

		// Get continuous pawn sound map
		ContinuousPawnSoundTags.Append(WorldAudioDataSettings->ContinuousPawnTags);
	}
	else
	{
		UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("Project settings invalid"));
	}

	bIsTickable = true;
}

void UWorldAudioDataSubsystem::Deinitialize()
{
	UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("Subsystem deinitializing"));

	bIsTickable = false;
}

bool UWorldAudioDataSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Check with parent first
	if (Super::ShouldCreateSubsystem(Outer) == false)
	{
		UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("Subsystem will not be created based on parent rules"));

		return false;
	}

	if (!GEngine || !GEngine->UseSound() || !GEngine->GetMainAudioDeviceRaw())
	{
		UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("GEngine unavailable for sound, subsystem will not be created"));

		return false;
	}

	// This world is not initialized yet, but it should be created; additionally, IsNetMode should be safe to call on a non-intialized world.
	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(Outer, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("This world prevented the creation of this subsystem"));

		return false;
	}

	UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("Audio device valid, will create subsystem"));
	return true;
}

void UWorldAudioDataSubsystem::UpdateWorldAudioDataVehAudioControllers(const TArray<FWorldAudioDataVehicleInfo>& WorldAudioDataVehicleInfo)
{
	if(bVehAudioControllersActive == false)
	{
		return;
	}

	TMap <int32, UWorldAudioDataVehAudioController*> CurrentVehAudioControllers;

	UWorld* World = GetWorld();

	if(World == nullptr)
	{
		return;
	}

	// Construct new and current map
	for(auto It = WorldAudioDataVehicleInfo.CreateConstIterator(); It; ++It)
	{
		if(UWorldAudioDataVehAudioController** VehAudioControllerDoublePtr = VehAudioControllers.Find(It->Id))
		{
			if(UWorldAudioDataVehAudioController* VehAudioControllerPtr = *VehAudioControllerDoublePtr)
			{
				// Update Vehicle Audio Controller's location and linear velocity
				VehAudioControllerPtr->SetWorldLocation(It->Location);
				VehAudioControllerPtr->SetVelocity(It->LinearVelocity.Size());
				VehAudioControllerPtr->ManualUpdate(World->DeltaTimeSeconds, CurrentPawnLocation);
				CurrentVehAudioControllers.Add(It->Id, VehAudioControllerPtr);
				VehAudioControllers.Remove(It->Id);
				// {
				// 	DrawDebugPoint(World, It->Location, 500.0f, FColor::Emerald, false, World->DeltaTimeSeconds * 2.0f, 000);
				// }

			}
		}
		else if(UWADVehAudioPreset** VehAudioPresetDoublePtr = VehAudioControllerPresetMap.Find(It->AudioController))
		{
			if(UWADVehAudioPreset* VehAudioPresetPtr = *VehAudioPresetDoublePtr)
			{
				UWorldAudioDataVehAudioController* NewVehicleAudioController = nullptr;
				NewVehicleAudioController = NewObject<UWorldAudioDataVehAudioController>(World);
				NewVehicleAudioController->SetMobility(EComponentMobility::Movable);
				// NewVehicleAudioController->SetAbsolute(true, true, true);
				NewVehicleAudioController->SetPreset(VehAudioPresetPtr, It->Id);
				NewVehicleAudioController->SetWorldLocation(It->Location);
				NewVehicleAudioController->SetVelocity(It->LinearVelocity.Size());
				CurrentVehAudioControllers.Add(It->Id, NewVehicleAudioController);

				// {
				// 	DrawDebugPoint(World, It->Location, 1000.0f, FColor::Green, false, World->DeltaTimeSeconds * 2.0f, 000);
				// }


			}
		}
	}

	// Stop old Vehicle Audio Controllers
	for(auto It = VehAudioControllers.CreateConstIterator(); It; ++It)
	{
		if (It->Value != nullptr)
		{
			It->Value->StopController();
		}
	}

	// Clear and copy
	VehAudioControllers.Empty(CurrentVehAudioControllers.Num());
	VehAudioControllers = CurrentVehAudioControllers;
}

void UWorldAudioDataSubsystem::ActivateVehAudioControllers()
{
	bVehAudioControllersActive = true;
}

void UWorldAudioDataSubsystem::DeactivateVehAudioControllers()
{
	bVehAudioControllersActive = false;

	for(const auto VehAudioController : VehAudioControllers)
	{
		if(VehAudioController.Value)
		{
			VehAudioController.Value->StopController();
		}
	}
}

void UWorldAudioDataSubsystem::Tick(float DeltaTime)
{
	if(bIsTickable == false)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// Get plugin settings on Subsystem initialization
		if (WorldAudioDataSettings)
		{
			if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
			{
				CurrentPawnLocation = PlayerPawn->GetActorTransform().GetLocation();
			}

			if (FAudioDevice* AudioDevice = World->GetAudioDeviceRaw())
			{
				// Get available Listener Proxies
				TArray<FListenerProxy>& ListenerProxies = AudioDevice->ListenerProxies;

				// TODO: Handle Split Screen
				if (ListenerProxies.Num())
				{
					// Get Location and Direction from Listener
					const FTransform ListenerTransform = ListenerProxies[0].Transform;
					ListenerLocation = ListenerTransform.GetLocation();
					ListenerForward = ListenerTransform.GetRotation().GetForwardVector();
					ListenerUp = ListenerTransform.GetRotation().GetUpVector();
				}
			}

			const uint8 CurrentReverbValue = DetermineReverbValueAtLocation(ListenerLocation);

			if(CurrentReverbValue > 0 && bReverbEffectActive == false && ListenerLocation.Z < WorldAudioDataSettings->MaxEffectAltitude)
			{
				UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("Reverb %d"), CurrentReverbValue);

				for (const FMantleDataSubmixToEffectMapping& EffectMap : WorldAudioDataSettings->EffectMap)
				{
					TArray<USoundEffectSubmixPreset*> NewEffectChain;

					for (const FMantleDataIntensityToEffectPresetChain& EffectsToMap : EffectMap.IntensityToEffectMap)
					{
						if(CurrentReverbValue >= EffectsToMap.MinimumEffectRange)
						{
							for(const FSoftObjectPath& EffectPath : EffectsToMap.SubmixEffectPresetChain)
							{
								if(UObject* Object = EffectPath.TryLoad())
								{
									if(USoundEffectSubmixPreset* SubmixPreset = Cast<USoundEffectSubmixPreset>(Object))
									{
										NewEffectChain.Add(SubmixPreset);
									}
								}
							}

							break;
						}
					}

					if(NewEffectChain.Num() > 0)
					{
						if (UObject* Object = EffectMap.SoundSubmix.TryLoad())
						{
							if (USoundSubmix* SoundSubmix = Cast<USoundSubmix>(Object))
							{
								UAudioMixerBlueprintLibrary::SetSubmixEffectChainOverride(World, SoundSubmix, NewEffectChain, 1.0f);

								bReverbEffectActive = true;
							}
						}
					}
				}
			}
			else if((CurrentReverbValue == 0 && bReverbEffectActive) || (bReverbEffectActive && ListenerLocation.Z > WorldAudioDataSettings->MaxEffectAltitude))
			{
				for(const FMantleDataSubmixToEffectMapping& EffectMap : WorldAudioDataSettings->EffectMap)
				{
					if(UObject* Object = EffectMap.SoundSubmix.TryLoad())
					{
						if(USoundSubmix* SoundSubmix = Cast<USoundSubmix>(Object))
						{
							UAudioMixerBlueprintLibrary::ClearSubmixEffectChainOverride(World, SoundSubmix, 1.0f);

							bReverbEffectActive = false;
						}
					}
				}
			}

			// Update continuous pawn sounds
			for(const auto ContinuousPawnSoundCollection : ContinuousPawnSoundCollections)
			{
				if(ContinuousPawnSoundCollection)
				{
					ContinuousPawnSoundCollection->VectorCollection.Empty(1);
					ContinuousPawnSoundCollection->VectorCollection.Add(ListenerLocation);
				}
				else
				{
					UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("ContinuousPawnSoundCollection invalid."));
				}
			}

			// begin evaluation of continuous sounds
			TArray<UContinuousSoundSystemVectorCollection*> ContinuousSoundSystemVectorCollectionsToRemove;
			TMap<FString, TArray<FVector>> ActiveContinuousSoundVectorsMap;

			// crawl through current collection
			for (auto ContinuousSoundSystemVectorCollection : ContinuousSoundSystemVectorCollections)
			{
				// if collection is valid
				if (ContinuousSoundSystemVectorCollection && bContinuousSoundSystemActive)
				{
					// attempt to get collection's vector array
					if (TArray<FVector>* VectorArray = ActiveContinuousSoundVectorsMap.Find(ContinuousSoundSystemVectorCollection->PointcloudDataKey))
					{
						// if vector array is valid/current, append new collection's pointcloud data
						VectorArray->Append(ContinuousSoundSystemVectorCollection->VectorCollection);
					}
					else
					{
						// if vector array is not valid, add new entry
						ActiveContinuousSoundVectorsMap.Add(ContinuousSoundSystemVectorCollection->PointcloudDataKey, ContinuousSoundSystemVectorCollection->VectorCollection);
					}
				}
				else
				{
					// populate remove list for old pointers
					ContinuousSoundSystemVectorCollectionsToRemove.Add(ContinuousSoundSystemVectorCollection);
				}
			}

			// remove old pointers
			for (auto ContinuousSoundSystemVectorCollectionToRemove : ContinuousSoundSystemVectorCollectionsToRemove)
			{
				ContinuousSoundSystemVectorCollections.Remove(ContinuousSoundSystemVectorCollectionToRemove);
			}

			// Update active continuous sounds
			for (auto ActiveContinuousSoundVector : ActiveContinuousSoundVectorsMap)
			{
				bool bNewActiveContinuousSoundNeeded = true;

				for (const auto ActiveContinuousSound : ActiveContinuousSounds)
				{
					if (ActiveContinuousSound)
					{
						if (ActiveContinuousSound->GetDataKey() == ActiveContinuousSoundVector.Key)
						{
							if(ActiveContinuousSound->NeedsNewAudioComponent() == false)
							{
								bNewActiveContinuousSoundNeeded = false;

								ActiveContinuousSound->UpdateAudioComponent(ActiveContinuousSoundVector.Value, ListenerLocation);
							}
						}
					}
				}

				// No matching continuous sound found, add a new one project settings permitting
				if (bNewActiveContinuousSoundNeeded && bContinuousSoundSystemActive)
				{
					if(const FSoftObjectPath* ObjectPathPtr = WorldAudioDataSettings->ContinuousSoundMap.Find(ActiveContinuousSoundVector.Key))
					{
						if(UObject* Object = ObjectPathPtr->TryLoad())
						{
							if(USoundBase* SoundBase = Cast<USoundBase>(Object))
							{
								UContinuousSound* NewContinuousSound = NewObject<UContinuousSound>(World);
								NewContinuousSound->InitializeContinuousSound(World, ActiveContinuousSoundVector.Key, SoundBase);
								NewContinuousSound->UpdateAudioComponent(ActiveContinuousSoundVector.Value, ListenerLocation);

								ActiveContinuousSounds.Add(NewContinuousSound);
							}
						}
					}
				}
			}

			TArray<UContinuousSound*> ContinuousSoundsToKeep;

			// Get rid of old continuous sounds
			for (const auto ActiveContinuousSound : ActiveContinuousSounds)
			{
				if (ActiveContinuousSound)
				{
					if(ActiveContinuousSoundVectorsMap.Find(ActiveContinuousSound->GetDataKey()))
					{
						ContinuousSoundsToKeep.Add(ActiveContinuousSound);
					}
				}
			}

			// Clear out Active Sounds List
			ActiveContinuousSounds.Empty();

			// Add back only the current ones
			ActiveContinuousSounds.Append(ContinuousSoundsToKeep);


			if (WorldAudioDataSettings->WorldAudioDataGameplayScript)
			{

				if (WorldAudioDataGameplayScript == nullptr)
				{
					
					// Spawn Actor and Set parameters
					WorldAudioDataGameplayScript = World->SpawnActor<AWorldAudioDataGameplayScript>(WorldAudioDataSettings->WorldAudioDataGameplayScript);
					USceneComponent* ActorRootComponent = NewObject<USceneComponent>(WorldAudioDataGameplayScript, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
					ActorRootComponent->Mobility = EComponentMobility::Movable;
					WorldAudioDataGameplayScript->SetRootComponent(ActorRootComponent);
					WorldAudioDataGameplayScript->AddInstanceComponent(ActorRootComponent);
					WorldAudioDataGameplayScript->SetActorLocation(FVector::Zero());

					UE_LOG(LogWorldAudioDataSystem, Verbose, TEXT("WorldAudioDataGameplayScript %s, spawned in world %s"), *WorldAudioDataSettings->WorldAudioDataGameplayScript->GetFullName(), *World->GetFullName());

				}
			}
		}
	}
}

bool UWorldAudioDataSubsystem::IsTickable() const
{
	return bIsTickable;
}

bool UWorldAudioDataSubsystem::IsTickableInEditor() const
{
	return false;
}

bool UWorldAudioDataSubsystem::IsTickableWhenPaused() const
{
	return true;
}

TStatId UWorldAudioDataSubsystem::GetStatId() const
{
	return TStatId();
}

void UWorldAudioDataSubsystem::AddContinuousSoundSystemVectorCollection(
	const TArray<UContinuousSoundSystemVectorCollection*>& ContinuousSoundSystemVectorCollectionsIn)
{
	if(ContinuousSoundSystemVectorCollectionsIn.Num())
	{
		ContinuousSoundSystemVectorCollections.Append(ContinuousSoundSystemVectorCollectionsIn);
	}
}

void UWorldAudioDataSubsystem::ActivateContinuousSoundSystem()
{
	UWorld* World = GetWorld();

	if(bContinuousSoundSystemActive == false && World)
	{
		for(auto ContinuousSoundTag : ContinuousPawnSoundTags)
		{
			if(ContinuousSoundTag != "" && ContinuousSoundTag != "None")
			{
				UContinuousSoundSystemVectorCollection* ContinuousPawnVectorCollection = NewObject<UContinuousSoundSystemVectorCollection>(World);

				ContinuousPawnVectorCollection->PointcloudDataKey = ContinuousSoundTag;
				ContinuousPawnVectorCollection->VectorCollection.Add(ListenerLocation);

				ContinuousPawnSoundCollections.Add(ContinuousPawnVectorCollection);
			}
		}

		ContinuousSoundSystemVectorCollections.Append(ContinuousPawnSoundCollections);
	}


	bContinuousSoundSystemActive = true;
}

void UWorldAudioDataSubsystem::DeactivateContinuousSoundSystem()
{
	if(bContinuousSoundSystemActive)
	{
		ContinuousPawnSoundCollections.Empty();
	}

	bContinuousSoundSystemActive = false;
}

bool UWorldAudioDataSubsystem::GetWorldAudioDataGameplayScript(
	AWorldAudioDataGameplayScript*& WorldAudioDataGameplayScriptOut)
{
	WorldAudioDataGameplayScriptOut = nullptr;

	if(WorldAudioDataGameplayScript)
	{
		WorldAudioDataGameplayScriptOut = WorldAudioDataGameplayScript;

		return true;
	}

	return false;
}

void UWorldAudioDataSubsystem::AddReverbDataCollection(UWorldAudioReverbDataCollection* ReverbDataCollection)
{
	TArray<UWorldAudioReverbDataCollection*> ReverbDataCollectionsToKeep;

	for (auto WorldAudioReverbDataCollection : WorldAudioReverbDataCollections)
	{
		if (WorldAudioReverbDataCollection)
		{
			ReverbDataCollectionsToKeep.Add(WorldAudioReverbDataCollection);
		}
	}

	WorldAudioReverbDataCollections.Empty();
	WorldAudioReverbDataCollections.Append(ReverbDataCollectionsToKeep);
	WorldAudioReverbDataCollections.Add(ReverbDataCollection);
}

uint8 UWorldAudioDataSubsystem::DetermineReverbValueAtLocation(FVector InLocation)
{
	uint8 TotalWeighting = 0;
	uint8 TotalValue = 0;

	for(const auto WorldAudioReverbDataCollection : WorldAudioReverbDataCollections)
	{
		if(WorldAudioReverbDataCollection)
		{
			uint8 Weighting = 0;
			const uint8 Value = WorldAudioReverbDataCollection->EvaluateDataAtLocation(InLocation, Weighting);

			if(Weighting > 0)
			{
				TotalValue = TotalValue + (Value * Weighting);
				TotalWeighting += Weighting;
			}
		}
	}

	return FMath::FloorToInt(TotalValue / FMath::Max(1.0f, TotalWeighting));
}
