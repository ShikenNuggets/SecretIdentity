// Copyright Epic Games, Inc. All Rights Reserved.


#include "CrossfaderSubsystem.h"
#include "Crossfader.h"
#include "AudioModulationStatics.h"
#include "SoundControlBusMix.h"
#include "UObject/SoftObjectPath.h"
#include "Containers/UnrealString.h"

UCrossfaderSettings::UCrossfaderSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR
void UCrossfaderSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


void UCrossfaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Cache World reference
	World = GetWorld();

	// Get plugin settings on Subsystem initialization
	const UCrossfaderSettings* CrossfaderSettings = GetDefault<UCrossfaderSettings>();

	// Validate Settings
	if (CrossfaderSettings)
	{
		// Make sure plugin settings aren't empty
		if (!CrossfaderSettings->MixStateBanks.IsEmpty())
		{
			// Loop through settings to collect bank data
			for (auto It = CrossfaderSettings->MixStateBanks.CreateConstIterator(); It; ++It)
			{
				// Load to retrieve bank data
				if (UObject* BankObj = It->TryLoad())
				{
					UMixStateBank* MixStateBank = Cast<UMixStateBank>(BankObj);

					if (MixStateBank)
					{
						// If bank is loaded, append stored data to master bank
						AddBank(MixStateBank);
					}
					else
					{
						// Bank failed cast
						const FString ObjName = BankObj->GetFullName();
						UE_LOG(LogCrossfader, Warning, TEXT("Failed to cast %s to default MixStateBank during initialization."), *ObjName);
					}
				}
				else
				{
					// Failed to load SoftObjectPath
					const FString SoftObjPathName = It->GetAssetPathString();
					UE_LOG(LogCrossfader, Warning, TEXT("Failed to load default MixStateBank SoftObjectPath %s during initialization."), *SoftObjPathName);
				}
			}


		}

		// Cache mixes if not already activated
		if (!bDefaultMixesActivated && World)
		{
			// Get User Mix
			FSoftObjectPath UserMixPath = CrossfaderSettings->UserMix;

			// Try to load the Mix
			if (UObject* MixObj = UserMixPath.TryLoad())
			{
				// Cast to a USoundControlBusMix
				USoundControlBusMix* UserMix = Cast<USoundControlBusMix>(MixObj);

				// Cache if valid
				if (UserMix)
				{
					ActivatedUserMix = UserMix;
				}
			}

			for (auto It = CrossfaderSettings->BaseProjectMixes.CreateConstIterator(); It; ++It)
			{
				// Get Base Mix
				FSoftObjectPath BaseMixPath = *It;

				if (UObject* MixObj = BaseMixPath.TryLoad())
				{
					// Cast to a USoundControlBusMix
					USoundControlBusMix* BaseMix = Cast<USoundControlBusMix>(MixObj);

					// Cache if valid
					if (BaseMix)
					{
						ActivatedDefaultMixes.AddUnique(BaseMix);
					}
				}

			}
		}
	}

}

void UCrossfaderSubsystem::Deinitialize()
{
	// Cache World reference
	World = GetWorld();
	
	// Disable Tick
	bShouldTick = false;

	// Make sure World is still valid
	if (World)
	{
		// If Mixes weren't activated, then we don't need to deactivate
		if (bDefaultMixesActivated)
		{
			// Deactivate User Mix if valid
			if (ActivatedUserMix)
			{
				UAudioModulationStatics::DeactivateBusMix(World, ActivatedUserMix);
			}

			// Deactivate Default Mixes
			for (auto It = ActivatedDefaultMixes.CreateConstIterator(); It; ++It)
			{
				USoundControlBusMix* ActivatedMix = *It;

				// If valid bus mix
				if (ActivatedMix)
				{
					UAudioModulationStatics::DeactivateBusMix(World, ActivatedMix);
				}
			}

			// Remove references to mixes
			ActivatedUserMix = nullptr;
			ActivatedDefaultMixes.Empty();

			// Set flag
			bDefaultMixesActivated = false;
		}
	}
}

void UCrossfaderSubsystem::AddBank(const UMixStateBank* MixStateBank)
{
	if (MixStateBank)
	{
		// If Bank is valid, get soft object path to use as master bank key
		FSoftObjectPath BankPath = FSoftObjectPath(MixStateBank);

		// Append stored data to master bank
		TArray<FCrossfaderMixPair> BankData;
		BankData.Append(MixStateBank->MixStates);

		// Bank path is used as key so data can be removed and added easily
		MasterMixStateBank.Add(BankPath, BankData);
	}
}

void UCrossfaderSubsystem::RemoveBank(const UMixStateBank* MixStateBank)
{
	if (MixStateBank)
	{
		// If bank is valid, get soft object path as key
		FSoftObjectPath BankKey = FSoftObjectPath(MixStateBank);

		if (MasterMixStateBank.Find(BankKey))
		{
			// If key is found, remove the bank data from the Master Bank
			MasterMixStateBank.Remove(BankKey);
		}
	}
}

bool UCrossfaderSubsystem::SetMixState(const UObject* WorldContextObject, FGameplayTag MixState, bool bFallBackToNearestParent, bool bDeactivateChildren)
{
	// Validate UWorld and Tag
	if (!WorldContextObject || !MixState.IsValid())
	{
		// Early out if either invalid
		return false;
	}

	uint32 MixStateTagDepth = GameplayTagDepth(MixState);

	// Check if incoming state is too small
	if (MixStateTagDepth < 1)
	{
		// Early out if incoming state is too small
		return false;
	}

	// Loop through find exact match, if no exact match is found, look for exact parent match
	TArray<TArray<FCrossfaderMixPair>> MasterListArray;
	MasterMixStateBank.GenerateValueArray(MasterListArray);

	// Set up variables for search
	USoundControlBusMix* BankMixToAdd = nullptr;
	FSoftObjectPath BankMixPath;
	bool bExactMatchFound = false;
	FGameplayTag MixStateParent = MixState.RequestDirectParent();
	FGameplayTag SelectedMixState;

	// Begin search
	for (auto ItA = MasterListArray.CreateConstIterator(); ItA; ++ItA)
	{
		TArray<FCrossfaderMixPair> BankArray = *ItA;
		
		for (auto ItB = BankArray.CreateConstIterator(); ItB; ++ItB)
		{
			FGameplayTag BankTag = ItB->MixState;
			
			if (MixState.MatchesTagExact(BankTag))
			{
				// Cache matching bank tag
				SelectedMixState = BankTag;

				// If we have an exact state match, set the BankMixPath
				BankMixPath = ItB->ControlBusMix;

				// Try to load the Bank Bus Mix
				if (UObject* BankObj = BankMixPath.TryLoad())
				{
					// Cast to a USoundControlBusMix
					BankMixToAdd = Cast<USoundControlBusMix>(BankObj);

					// We found a match, set flag to true
					bExactMatchFound = true;

					// We found a match, we can leave the loop
					break;
				}
			}
			else
			{
				// If we want to fall back to nearest parent, do so in this loop
				if (bFallBackToNearestParent)
				{
					FGameplayTag MixStateTag = MixState;

					// Search for nearest valid parent tag
					for (uint32 i = 0; i < MixStateTagDepth; ++i)
					{
						MixStateTag = MixStateTag.RequestDirectParent();

						if (MixStateTag.MatchesTagExact(BankTag))
						{
							// Cache matching bank tag
							SelectedMixState = BankTag;

							// If the MixStateTag matches, cache just in case we never find an exact match
							BankMixPath = ItB->ControlBusMix;

							// Try to load the Bank Bus Mix
							if (UObject* BankObj = BankMixPath.TryLoad())
							{
								// Cast to a USoundControlBusMix
								BankMixToAdd = Cast<USoundControlBusMix>(BankObj);
							}
						}
					}

				}
			}
		}

		if (bExactMatchFound)
		{
			// We found an exact match in the last sub-loop, we can leave the search
			break;
		}
	}

	if (!BankMixToAdd)
	{
		// No Mix has been found
		return false;
	}

	TArray<FGameplayTag> OldMixesToRemove;
	bool bMixAlreadyActive = false;
	bool bMixesAreSiblings = false;

	// Check Active Mixes to determine if we need to deactivate a current mix
	for (auto It = ActiveMixes.CreateConstIterator(); It; ++It)
	{
		FGameplayTag ActiveMixKey = It.Key();
		FGameplayTag ActiveMixKeyParent = ActiveMixKey.RequestDirectParent();
		int32 NumChildrenLeft = 0;
		FGameplayTag SelectedMixStateParent = SelectedMixState.RequestDirectParent();
		const int32 MixStateParentKeyTagDepth = GameplayTagDepth(SelectedMixStateParent.RequestDirectParent());
		const int32 SelectedMixStateTagDepth = GameplayTagDepth(SelectedMixState);

		do {

			const int32 ActiveMixKeyTagDepth = GameplayTagDepth(ActiveMixKey);
			// Update depth loop conditions
			NumChildrenLeft = bDeactivateChildren ? (ActiveMixKeyTagDepth - SelectedMixStateTagDepth) : 0;

			if ((ActiveMixKey.MatchesTagExact(SelectedMixStateParent))
				|| (MixStateParentKeyTagDepth == SelectedMixState.MatchesTagDepth(ActiveMixKey) && bDeactivateChildren && bFallBackToNearestParent)
				|| (ActiveMixKey.MatchesTagExact(SelectedMixState) && bDeactivateChildren)
				|| (SelectedMixStateParent.MatchesTagExact(ActiveMixKey))
				|| (SelectedMixStateTagDepth == SelectedMixState.MatchesTagDepth(ActiveMixKeyParent) && bDeactivateChildren && bFallBackToNearestParent))
			{
				// The Mix Key Parent matches the Mix State Parent or the Mix Key matches the Mix State Parent, that means they're in the same group
				const FCrossfaderMixBusStatePair MixPairToDeactivate = It.Value();
				FGameplayTag ActiveMixValue = MixPairToDeactivate.MixState;

				// If the Mix Key matches the Mix State exactly, the mix is already active, we do not need to remove it
				if (!ActiveMixValue.MatchesTagExact(SelectedMixState))
				{
					// Cache the Control Bus Mix
					USoundControlBusMix* OldBusMix = MixPairToDeactivate.ControlBusMix;

					if (OldBusMix)
					{
						// If the mix is still valid, deactivate it
						UAudioModulationStatics::DeactivateBusMix(WorldContextObject, OldBusMix);

						bMixesAreSiblings = ActiveMixKey == SelectedMixStateParent;

						if (!bMixesAreSiblings)
						{
							OldMixesToRemove.AddUnique(ActiveMixKey);
						}

						break;
					}
				}
				else
				{
					// If incoming matches an already Active mix, it's already active so there's no reason to add it
					bMixAlreadyActive = true;

					break;
				}

			}

			// We're working backward from the end of the tag
			ActiveMixKey = ActiveMixKey.RequestDirectParent();
			ActiveMixKeyParent = ActiveMixKey.RequestDirectParent();
		} while (NumChildrenLeft > 0);

	}

	if (!bMixAlreadyActive)
	{
		// If mix is not already active, activate the mix
		UAudioModulationStatics::ActivateBusMix(WorldContextObject, BankMixToAdd);

		// Set up Map Variables
		FCrossfaderMixBusStatePair MixBusStatePairToAdd;
		MixBusStatePairToAdd.ControlBusMix = BankMixToAdd;
		MixBusStatePairToAdd.MixState = MixState;

		// Init map key as group level
		FGameplayTag MixStateKeyToAdd = MixStateParent;
		
		if (!bExactMatchFound)
		{
			// Incoming Mix State is group level, so they will be the same
			MixStateKeyToAdd = MixState;
		}
		
		if (bMixesAreSiblings)
		{
			// Update Key with new incoming Pair
			ActiveMixes.Emplace(MixStateKeyToAdd) = MixBusStatePairToAdd;
		}
		else
		{
			// Add new mix to the map
			ActiveMixes.Add(MixStateKeyToAdd, MixBusStatePairToAdd);
		}
	}

	for (auto It = OldMixesToRemove.CreateConstIterator(); It; ++It)
	{
		// Remove old mix from ActiveMixes map
		if (It->IsValid())
		{
			ActiveMixes.Remove(*It);
		}

	}

	return true;
}

void UCrossfaderSubsystem::ClearMixState(const UObject* WorldContextObject, FGameplayTag MixState, bool bDeactivateChildren)
{
	// Validate UWorld and Tag
	if (WorldContextObject || MixState.IsValid())
	{
		TArray<FGameplayTag> OldMixesToRemove;
		bool bMixAlreadyActive = false;

		// Check Active Mixes to determine if we need to deactivate a current mix
		for (auto It = ActiveMixes.CreateConstIterator(); It; ++It)
		{
			FGameplayTag ActiveMixKey = It.Key();
			FGameplayTag ActiveMixKeyParent = ActiveMixKey.RequestDirectParent();
			int32 NumChildrenLeft = 0;
			FGameplayTag SelectedMixStateParent = MixState.RequestDirectParent();
			const int32 MixStateParentKeyTagDepth = GameplayTagDepth(SelectedMixStateParent.RequestDirectParent());
			const int32 SelectedMixStateTagDepth = GameplayTagDepth(MixState);

			// Cache keys for loop
			FGameplayTag LoopMixKey = ActiveMixKey;

			// Cache Active Mix Value
			const FCrossfaderMixBusStatePair MixPairToDeactivate = It.Value();
			const FGameplayTag ActiveMixValue = MixPairToDeactivate.MixState;

			do {

				const int32 ActiveMixKeyTagDepth = GameplayTagDepth(LoopMixKey);
				// Update depth loop conditions
				NumChildrenLeft = bDeactivateChildren ? (ActiveMixKeyTagDepth - (int32)SelectedMixStateTagDepth) : 0;

				// If current Mix Key matches Selected Mix State Parent,
				// Or if the current Mix Key matches the incoming MixState and Deactivate Children is true, then continue
				if ((LoopMixKey.MatchesTagExact(SelectedMixStateParent))
					|| (LoopMixKey.MatchesTagExact(MixState) && bDeactivateChildren)
					)
				{
					// If we are not deactivating children, we want to make sure the value is an exact match
					// If we are deactivating children, then can continue
					if (bDeactivateChildren != ActiveMixValue.MatchesTagExact(MixState)
						|| bDeactivateChildren)
					{
						// Cache the Control Bus Mix
						USoundControlBusMix* OldBusMix = MixPairToDeactivate.ControlBusMix;

						if (OldBusMix)
						{
							// If the mix is still valid, deactivate it
							UAudioModulationStatics::DeactivateBusMix(WorldContextObject, OldBusMix);

							OldMixesToRemove.AddUnique(ActiveMixKey);

							break;
						}

					}
				}

				// We're working backward from the end of the tag
				LoopMixKey = LoopMixKey.RequestDirectParent();
			} while (NumChildrenLeft > 0);

		}

		// Remove mixes
		for (auto It = OldMixesToRemove.CreateConstIterator(); It; ++It)
		{
			// Remove old mix from ActiveMixes map
			if (It->IsValid())
			{
				ActiveMixes.Remove(*It);
			}
		}
	}
}

uint32 UCrossfaderSubsystem::GameplayTagDepth(FGameplayTag GameplayTag)
{
	// Convert to string
	FString GameplayTagString = GameplayTag.ToString();

	// Create an Array
	TArray<FString> ParsedArray;

	// Parse string out into Array using "." as delimter
	GameplayTagString.ParseIntoArray(ParsedArray, TEXT("."), true);

	// Return num of elements in parsed array
	return ParsedArray.Num();
}

void UCrossfaderSubsystem::Tick(float DeltaTime)
{
	// Just in case
	if (bShouldTick)
	{
		World = GetWorld();

		// Activate cached base mixes if not already activated
		if (!bDefaultMixesActivated && World)
		{
			// Activate User Mix
			if (ActivatedUserMix)
			{
					UAudioModulationStatics::ActivateBusMix(World, ActivatedUserMix);
			}

			// Activate Base Mixes
			for (auto It = ActivatedDefaultMixes.CreateConstIterator(); It; ++It)
			{
				USoundControlBusMix* BaseMix = *It;

				if (BaseMix)
				{
					UAudioModulationStatics::ActivateBusMix(World, BaseMix);
				}
			}

			// Update activation state
			bDefaultMixesActivated = true;

			// Once activated, it no longer needs to tick
			bShouldTick = false;
		}
	}
}

bool UCrossfaderSubsystem::IsTickable() const
{
	return bShouldTick;
}

bool UCrossfaderSubsystem::IsTickableInEditor() const
{
	return false;
}

bool UCrossfaderSubsystem::IsTickableWhenPaused() const
{
	return false;
}

TStatId UCrossfaderSubsystem::GetStatId() const
{
	return TStatId();
}