// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldAudioMassEntityTrait.h"
#include "MassEntityManager.h"
#include "MassEntityTemplateRegistry.h"
#include "Engine/World.h"
#include "MassEntityUtils.h"


void UWorldAudioDataMassEntityTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	// Add config as shared fragment
	FWorldAudioDataAudioControllerParameters AudioControllerFragment;
	AudioControllerFragment.AudioController = AudioController;
	const FConstSharedStruct AudioControllerSharedFragment = EntityManager.GetOrCreateConstSharedFragment(AudioControllerFragment);
	BuildContext.AddConstSharedFragment(AudioControllerSharedFragment);
}

