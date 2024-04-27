// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "Components/ActorComponent.h"
#include "SecretIdentity.h"
#include "MusicPlayer.generated.h"

UCLASS()
class SECRETIDENTITY_API UMusicPlayer : public USceneComponent
{
	GENERATED_BODY()
	
public:
	UMusicPlayer();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void OnPlayerStateChanged(EPlayerControlState State);

protected:
	virtual void BeginPlay() override;

	void StartCrossFade(USoundBase* NextSong, float Duration = 1.0f, EAudioFaderCurve Curve = EAudioFaderCurve::Linear);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (AllowPrivateAccess = "true"))
	USoundBase* CalmFlyingMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (AllowPrivateAccess = "true"))
	USoundBase* HeavyFlyingMusic;

	UPROPERTY()
	UAudioComponent* CurrentMusicComponent;

	UPROPERTY()
	UAudioComponent* NextMusicComponent;

	TStaticArray<USoundBase*, static_cast<uint32>(EPlayerControlState::Count)> tSongs;
	bool bIsCrossFading = false;
};