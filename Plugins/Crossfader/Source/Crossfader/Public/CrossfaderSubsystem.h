// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DeveloperSettings.h"
#include "MixStateBank.h"
#include "Containers/Map.h"
#include "Tickable.h"
#include "CrossfaderSubsystem.generated.h"

class USoundControlBusMix;

/**
* Game Settings which allow the user to set Default Mixes which activate/deactivate with the Crossfader Subsystem 
* as well as define default MixStateBanks which are added to the Subsystem master bank on initialization. 
*/
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Crossfader"))
class CROSSFADER_API UCrossfaderSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:

	/** The Crossfader Subsystem will attempt to load and activate these SoundControlBusMixes on World creation */
	UPROPERTY(config, EditAnywhere, Category=Crossfader, meta = (AllowedClasses = "/Script/AudioModulation.SoundControlBusMix"))
	TArray<FSoftObjectPath> BaseProjectMixes;

	/** The Crossfader Subsystem will attempt to load and activate this SoundControlBusMix on World creation */
	UPROPERTY(config, EditAnywhere, Category = Crossfader, meta = (AllowedClasses = "/Script/AudioModulation.SoundControlBusMix"))
	FSoftObjectPath UserMix;

	/** The Crossfadfer Subsystem will add the data from these MixStateBanks to the master bank list on initialization */
	UPROPERTY(config, EditAnywhere, Category = Crossfader, meta = (AllowedClasses = "/Script/Crossfader.MixStateBank"))
	TArray<FSoftObjectPath> MixStateBanks;

public:

	// Beginning of UDeveloperSettings Interface
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("CrossfaderPlugin", "CrossfaderSettingsSection", "Crossfader"); };
#endif
	// End of UDeveloperSettings Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif


};

/** 
* This struct contains the MixState GameplayTag and the associated SoundControlBusMix pointer. 
*/
USTRUCT()
struct CROSSFADER_API FCrossfaderMixBusStatePair
{
	GENERATED_BODY()

	// Associated MixState
	FGameplayTag MixState;

	/** Pointer to the Associated SoundControlBus */
	UPROPERTY(EditAnywhere)
	USoundControlBusMix* ControlBusMix;

	// Default ptr to null
	FCrossfaderMixBusStatePair()
		: ControlBusMix(nullptr)
	{
	}
};

/**
 * CrossfaderSubsystem is the high-level manager that filters GameplayTag MixStates and activates/deactivates associated SoundControlBusMixes.
 */
UCLASS()
class CROSSFADER_API UCrossfaderSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem

private:
	// Cached World Pointer
	UWorld* World;

	// Cached pointer to User SoundControlBusMix
	UPROPERTY()
	USoundControlBusMix* ActivatedUserMix;
	
	// Cached pointers to Default SoundControlBusMixes
	UPROPERTY()
	TArray<USoundControlBusMix*> ActivatedDefaultMixes;

	// Flag indicating default mixes have been activated (only need to activate on first world)
	bool bDefaultMixesActivated = false;

public:
	// Crossfader Subsystem API

	/** 
	* Add bank data to the Crossfader Master Bank 
	* @param MixStateBank The MixStateBank to add to the Crossfader Master Bank
	*/
	UFUNCTION(BlueprintCallable, Category = Crossfader)
	void AddBank(const UMixStateBank* MixStateBank);

	/** 
	* Add bank data to the Crossfader Master Bank 
	* @param MixStateBank The MixStateBank to remove from the Crossfader Master Bank
	*/
	UFUNCTION(BlueprintCallable, Category = Crossfader)
	void RemoveBank(const UMixStateBank* MixStateBank);

	/** 
	* Set a new Mix State.
	* If two MixStates share the same Parent Namespace, they will be mutually exclusive. For example
	* if you have an active MixState whose namespace is Crossfader.Location.A and you set the MixState to
	* Crossfader.Location.B, then Crossfader.Location.A will be deactivated and Crossfader.Location.B will
	* take its place. However if they don't share the same parent, they will not be mutually exclusive.
	* @param MixState The new MixState to set on the Crossfader Subsystem
	* @param bFallBackToNearestParent When set to true, it will attempt to move up the MixState namespace until it finds
	* a match. So for example, if you set Crossfader.Location.A and that Bank data isn't found, it will try Crossfader.Location, and continue
	* up until a match is found or there are no more namespaces.
	* @param bDeactivateChildren When set to true, as the state is set, that state's parent namespace and all children beneath it will
	* be deactivated (not just siblings).
	* @return Will return true if a MixState was set, otherwise it will return false if no matching MixState was found.
	*/
	UFUNCTION(BlueprintCallable, Category = Crossfader, meta = (WorldContext = "WorldContextObject", Categories = "Crossfader"))
	bool SetMixState(const UObject* WorldContextObject, FGameplayTag MixState, bool bFallBackToNearestParent = false, bool bDeactivateChildren = true);

	/** Clear an active Mix State.
	* This function will look for a specific MixState and clear it. If it cannot find it, then there was no need to clear the state.
	* @param MixState The MixState to clear.
	* @param bDeactivateChildren When set to true, this function will also clear all associated siblings and children of the stated MixState. 
	* Calling clear at the top MixState namespace when this is true, will clear all active MixStates.
	*/
	UFUNCTION(BlueprintCallable, Category = Crossfader, meta = (WorldContext = "WorldContextObject", Categories = "Crossfader"))
	void ClearMixState(const UObject* WorldContextObject, FGameplayTag MixState, bool bDeactivateChildren = true);

private:
	/** The master list of bank data is stored as F Objects only (FSoftObjectPaths and FGameplayTags), no UObjects are stored in this list. */
	TMap<FSoftObjectPath, TArray<FCrossfaderMixPair>> MasterMixStateBank;

	/** A Map of Active Mixes, the Mix State Parent (x.y) is used as a key to a struct containing both the Active State (x.y.z or x.y) and a Control Bus Mix. */
	UPROPERTY()
	TMap<FGameplayTag, FCrossfaderMixBusStatePair> ActiveMixes;

	// Helper funcction to determine how many tags are in the GameplayTag (e.g. x.y will return 2, x.y.z will return 3, etc.)
	uint32 GameplayTagDepth(FGameplayTag GameplayTag);

public:
	// Begin FTickableGameObject
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	bool IsTickableInEditor() const override;
	bool IsTickableWhenPaused() const override;
	TStatId GetStatId() const override;
	// End FTickableGameObject

private:
	bool bShouldTick = true;

};
