// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayCharacterMovementComponent.h"

#include "SecretIdentity/UE_Helpers.h"

UPlayCharacterMovementComponent::UPlayCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bOrientRotationToMovement = true;
	RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	AirControl = 0.35f;
	MinAnalogWalkSpeed = 20.0f;
	BrakingDecelerationWalking = 2000.0f;
	BrakingDecelerationFalling = 1500.0f;
	BrakingDecelerationFlying = 15000.0f;
	GravityScale = 2.5f;

	JumpZVelocity = DefaultJumpForce;
	MaxWalkSpeed = JogSpeed;

	fDefaultMaxAcceleration = MaxAcceleration;
	bIsNearMaxFlightSpeed = false;
}

void UPlayCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	WARN_IF(MaxSpeedEnterThreshold < MaxSpeedExitThreshold);
	WARN_IF(MaxSpeedEnterThreshold < 0.001f || MaxSpeedEnterThreshold > 0.999f);
	WARN_IF(MaxSpeedExitThreshold < 0.001f || MaxSpeedExitThreshold > 0.999f);
	MaxSpeedEnterThreshold = FMath::Clamp(MaxSpeedEnterThreshold, 0.001f, 0.999f);
	MaxSpeedExitThreshold = FMath::Clamp(MaxSpeedExitThreshold, 0.001f, 0.999f);
}

void UPlayCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementMode == EMovementMode::MOVE_Flying)
	{
		ApplyVelocityBraking(DeltaTime, BrakingFriction, BrakingDecelerationFlying);
		if (!bIsNearMaxFlightSpeed && Velocity.Size() >= MaxFlightForwardSpeed * MaxSpeedEnterThreshold)
		{
			bIsNearMaxFlightSpeed = true;
			OnFlightSpeedChanged.Broadcast(bIsNearMaxFlightSpeed);
		}
		else if(bIsNearMaxFlightSpeed && Velocity.Size() < MaxFlightForwardSpeed * MaxSpeedExitThreshold)
		{
			bIsNearMaxFlightSpeed = false;
			OnFlightSpeedChanged.Broadcast(bIsNearMaxFlightSpeed);
		}
	}
}

void UPlayCharacterMovementComponent::OnPlayerStateChanged(EPlayerControlState State)
{
	switch (State)
	{
		case EPlayerControlState::None:
			MaxWalkSpeed = 0.0f;
			bOrientRotationToMovement = true;
			bIsNearMaxFlightSpeed = false;
			SetMovementMode(EMovementMode::MOVE_None);
			break;

		case EPlayerControlState::Default:
			MaxWalkSpeed = JogSpeed;
			MaxAcceleration = fDefaultMaxAcceleration;
			bOrientRotationToMovement = true;
			bIsNearMaxFlightSpeed = false;
			SetMovementMode(EMovementMode::MOVE_Walking);
			break;

		case EPlayerControlState::Sprinting:
			MaxWalkSpeed = JogSpeed * DefaultSprintMultiplier;
			bOrientRotationToMovement = true;
			bIsNearMaxFlightSpeed = false;
			break;

		case EPlayerControlState::Punching:
			MaxWalkSpeed = JogSpeed;
			bOrientRotationToMovement = true;
			bIsNearMaxFlightSpeed = false;
			SetMovementMode(EMovementMode::MOVE_None);
			break;

		case EPlayerControlState::TravelPower_Flight_Strafe:
			MaxFlySpeed = JogSpeed;
			MaxAcceleration = fDefaultMaxAcceleration * 8.0f;
			Velocity.Z = 0.0f;
			bOrientRotationToMovement = false;
			bIsNearMaxFlightSpeed = false;
			SetMovementMode(EMovementMode::MOVE_Flying);
			break;

		case EPlayerControlState::TravelPower_Flight_Forward:
			MaxFlySpeed = MaxFlightForwardSpeed;
			MaxAcceleration = fDefaultMaxAcceleration * 8.0f;
			bOrientRotationToMovement = false;
			SetMovementMode(EMovementMode::MOVE_Flying);
			break;

		default:
			WARN_IF_MSG(true, "EPlayerControlState case not handled in UPlayCharacterMovementComponent::OnPlayerStateChanged!");
			break;
	}
}