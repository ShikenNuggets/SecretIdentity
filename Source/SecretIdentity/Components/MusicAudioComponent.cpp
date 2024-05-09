// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "MusicAudioComponent.h"

#include "Sound/SoundCue.h"

#include "SecretIdentity/UE_Helpers.h"

UMusicAudioComponent::UMusicAudioComponent() : eMusicState(MusicState::None)
{
	SetSound(nullptr);
}

void UMusicAudioComponent::SwitchState(MusicState State)
{
	if (eMusicState == State)
	{
		return;
	}

	FadeOut(1.0f, 1.0f);

	eMusicState = State;
	switch(eMusicState)
	{
		case MusicState::None:
			SetSound(nullptr);
			break;
		case MusicState::CalmFlying:
			SetSound(Cast<USoundBase>(CalmFlyingSoundCue));
			break;
		case MusicState::HeavyFlying:
			SetSound(Cast<USoundBase>(HeavyFlyingSoundCue));
			break;
		default:
			WARN_IF_MSG(true, "Unhandled Switch case in UMusicAudioComponent::SwitchState");
			break;
	}

	FadeIn(1.0f);
}

void UMusicAudioComponent::BeginPlay()
{
	WARN_IF_NULL(CalmFlyingSoundCue);
	WARN_IF_NULL(HeavyFlyingSoundCue);
}