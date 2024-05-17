// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_MixStateBank.h"
#include "MixStateBank.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_MixStateBank::GetSupportedClass() const
{
	return UMixStateBank::StaticClass();
}

const TArray<FText>& FAssetTypeActions_MixStateBank::GetSubMenus() const
{
	static const TArray<FText> SubMenus
	{
		LOCTEXT("AssetCrossfaderSubMenu", "Crossfader")
	};

	return SubMenus;
}

#undef LOCTEXT_NAMESPACE