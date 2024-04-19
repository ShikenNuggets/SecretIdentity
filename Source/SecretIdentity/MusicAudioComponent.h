// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "MusicAudioComponent.generated.h"

enum class MusicState{
	None = 0,
	CalmFlying,
	HeavyFlying
};

/**
 * 
 */
UCLASS()
class SECRETIDENTITY_API UMusicAudioComponent : public UAudioComponent
{
	GENERATED_BODY()
	
public:
	UMusicAudioComponent();

	void SwitchState(MusicState State);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music", meta = (AllowPrivateAccess = "true"))
	class USoundCue* CalmFlyingSoundCue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music", meta = (AllowPrivateAccess = "true"))
	class USoundCue* HeavyFlyingSoundCue;

	MusicState eMusicState;
};
