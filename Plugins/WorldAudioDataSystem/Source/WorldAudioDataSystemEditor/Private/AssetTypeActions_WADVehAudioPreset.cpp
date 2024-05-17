// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_WADVehAudioPreset.h"
#include "WorldAudioDataVehAudioPreset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_WADVehAudioPreset::GetSupportedClass() const
{
	return UWADVehAudioPreset::StaticClass();
}

const TArray<FText>& FAssetTypeActions_WADVehAudioPreset::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetWorldaudiodataSubMenu", "WorldAudioData")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE