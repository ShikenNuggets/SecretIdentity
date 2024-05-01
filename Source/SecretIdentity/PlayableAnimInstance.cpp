// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayableAnimInstance.h"

FName UPlayableAnimInstance::GetCurrentMontageName() const
{
	UAnimMontage* Montage = GetCurrentActiveMontage();
	if (Montage != nullptr)
	{
		return Montage->GetFName();
	}

	return TEXT("");
}