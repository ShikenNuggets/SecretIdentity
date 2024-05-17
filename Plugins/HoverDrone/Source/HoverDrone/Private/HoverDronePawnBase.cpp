// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDronePawnBase.h"

#include "Components/SphereComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogHoverDrone)

const FName AHoverDronePawnBase::CameraComponentName(TEXT("CameraComponent0"));

AHoverDronePawnBase::AHoverDronePawnBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bFindCameraComponentWhenViewTarget = true;
	bAddDefaultMovementBindings = false;
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	CameraComponent->SetupAttachment(GetCollisionComponent());

	BaseTurnRate = 112.0f;
	BaseLookUpRate = 80.0f;
}

void AHoverDronePawnBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//if (ClientSettings)
	{
		//SetActorEnableCollision(SpectatingPC->IsInThirdPersonCam() ? ClientSettings->GetThirdPersonCameraCollision() : true);
		SetActorEnableCollision(true);
	}
}

void AHoverDronePawnBase::UnPossessed()
{
	Super::UnPossessed();
	SetActorEnableCollision(false);
}

void AHoverDronePawnBase::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	check(InInputComponent);
	Super::SetupPlayerInputComponent(InInputComponent);

	{
		InInputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
		InInputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);
		//NEEDED - JB
		InInputComponent->BindAxis("MoveUp", this, &ADefaultPawn::MoveUp_World);
		InInputComponent->BindAxis("Turn", this, &ADefaultPawn::AddControllerYawInput);
		InInputComponent->BindAxis("LookUp", this, &ADefaultPawn::AddControllerPitchInput);
	}
}

void AHoverDronePawnBase::TurnAtRate(float Rate)
{
	if (Rate != 0.0f)
	{
		UWorld* const World = GetWorld();
		AWorldSettings* const WorldSettings = World ? World->GetWorldSettings() : nullptr;

		float CurrentTimeDilation = 1.0f;

		if (World && WorldSettings)
		{
			CurrentTimeDilation = FMath::Max(WorldSettings->GetEffectiveTimeDilation(), SMALL_NUMBER);
		}

		AddControllerYawInput(Rate * BaseTurnRate * (GetWorld()->GetDeltaSeconds() / CurrentTimeDilation));
	}
}

void AHoverDronePawnBase::LookUpAtRate(float Rate)
{
	if (Rate != 0.0f)
	{
		UWorld* const World = GetWorld();
		AWorldSettings* const WorldSettings = World ? World->GetWorldSettings() : nullptr;

		float CurrentTimeDilation = 1.0f;

		if (World && WorldSettings)
		{
			CurrentTimeDilation = FMath::Max(WorldSettings->GetEffectiveTimeDilation(), SMALL_NUMBER);
		}

		AddControllerPitchInput(Rate * BaseTurnRate * (GetWorld()->GetDeltaSeconds() / CurrentTimeDilation));
	}
}
