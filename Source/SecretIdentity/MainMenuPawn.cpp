// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "MainMenuPawn.h"

#include "Camera/CameraComponent.h"

#include "UE_Helpers.h"
#include "GameModes/ArcadeGameMode.h"

AMainMenuPawn::AMainMenuPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMainMenuPawn::BeginPlay()
{
	Super::BeginPlay();

	AArcadeGameMode* GameMode = Cast<AArcadeGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode != nullptr)
	{
		GameMode->OnBeginTransitionToPlayState.AddDynamic(this, &AMainMenuPawn::OnBeginTransitionToPlayState);
	}
}

void AMainMenuPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (aTransitionTarget != nullptr)
	{
		fTransitionTimer += DeltaTime;

		FVector NewLocation = FMath::Lerp(fStartLocation, aTransitionTarget->GetComponentLocation(), fTransitionTimer / fTransitionTime);
		FRotator NewRotation = FMath::Lerp(fStartRotation, aTransitionTarget->GetComponentRotation(), fTransitionTimer / fTransitionTime);
		SetActorLocationAndRotation(NewLocation, NewRotation);
	}
}

void AMainMenuPawn::UnPossessed()
{
	Super::UnPossessed();

	Destroy();
}

void AMainMenuPawn::OnBeginTransitionToPlayState(APawn* NewPawn, float TransitionTime)
{
	LOG_MSG("OnBegin");

	WARN_IF_NULL(NewPawn);

	if (NewPawn != nullptr)
	{
		aTransitionTarget = NewPawn->GetComponentByClass<UCameraComponent>();
	}
	
	fTransitionTime = FMath::Clamp(TransitionTime, 0.1f, 10.0f); //Things will get weird if this number is too low or too high
	fStartLocation = GetActorLocation();
	fStartRotation = GetActorRotation();
}