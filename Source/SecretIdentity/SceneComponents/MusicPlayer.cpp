// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "MusicPlayer.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

#include "SecretIdentity/UE_Helpers.h"

UMusicPlayer::UMusicPlayer()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	CurrentMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioSource1"));
	if (CurrentMusicComponent)
	{
		CurrentMusicComponent->SetupAttachment(this);
	}

	NextMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioSource2"));
	if (NextMusicComponent)
	{
		NextMusicComponent->SetupAttachment(this);
	}

	for (int32 i = 0; i <= tSongs.Num(); i++)
	{
		tSongs[i] = nullptr;
	}
}

void UMusicPlayer::BeginPlay()
{
	Super::BeginPlay();

	tSongs[static_cast<uint32>(EPlayerControlState::TravelPower_Flight_Strafe)] = CalmFlyingMusic;
	tSongs[static_cast<uint32>(EPlayerControlState::TravelPower_Flight_Forward)] = HeavyFlyingMusic;

	if (CurrentMusicComponent != nullptr)
	{
		CurrentMusicComponent->Stop();
	}

	if (NextMusicComponent != nullptr)
	{
		NextMusicComponent->Stop();
	}

	WARN_IF_NULL(CurrentMusicComponent);
	WARN_IF_NULL(NextMusicComponent);
}

void UMusicPlayer::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsCrossFading && CurrentMusicComponent != nullptr && !CurrentMusicComponent->IsPlaying())
	{
		Swap(CurrentMusicComponent, NextMusicComponent);
		bIsCrossFading = false;
	}
}

void UMusicPlayer::OnPlayerStateChanged(EPlayerControlState State)
{
	WARN_IF(State >= EPlayerControlState::Count);

	if (State < EPlayerControlState::Count && static_cast<int32>(State) < tSongs.Num())
	{
		StartCrossFade(tSongs[static_cast<uint32>(State)]);
	}
	else
	{
		WARN_IF_MSG(true, "EPlayerControlState case not handled in UMusicPlayer::OnPlayerStateChanged!");
	}
}

void UMusicPlayer::StartCrossFade(USoundBase* NextSong, float Duration, EAudioFaderCurve Curve)
{
	if (CurrentMusicComponent == nullptr || NextMusicComponent == nullptr)
	{
		WARN_IF_MSG(true, "Missing audio component(s), need at least two for crossfading!");
		return;
	}

	if (bIsCrossFading)
	{
		Swap(CurrentMusicComponent, NextMusicComponent);
	}

	bIsCrossFading = true;
	NextMusicComponent->Stop();
	NextMusicComponent->SetSound(NextSong);

	CurrentMusicComponent->FadeOut(Duration, 0.0f, Curve);

	if (NextSong != nullptr)
	{
		NextMusicComponent->FadeIn(Duration, 1.0f, 0.0f, Curve);
	}
}