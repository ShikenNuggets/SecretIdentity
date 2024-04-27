// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayCharacterMovementComponent.h"

#include "UE_Helpers.h"

UPlayCharacterMovementComponent::UPlayCharacterMovementComponent()
{
	bOrientRotationToMovement = true;
	RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	AirControl = 0.35f;
	MinAnalogWalkSpeed = 20.0f;
	BrakingDecelerationWalking = 2000.0f;
	BrakingDecelerationFalling = 1500.0f;
	BrakingDecelerationFlying = 7500.0f;
	GravityScale = 2.5f;

	SetOptions(FPlayCharacterMovementOptions());
}

void UPlayCharacterMovementComponent::SetOptions(const FPlayCharacterMovementOptions& Options)
{
	fOptions = Options;

	JumpZVelocity = fOptions.DefaultJumpForce;
	MaxWalkSpeed = fOptions.JogSpeed;
}

void UPlayCharacterMovementComponent::OnPlayerStateChanged(ControlState State)
{
	switch (State)
	{
		case ControlState::Default:
			MaxWalkSpeed = fOptions.JogSpeed;
			bOrientRotationToMovement = true;
			SetMovementMode(EMovementMode::MOVE_Falling);
			break;

		case ControlState::Sprinting:
			MaxWalkSpeed = fOptions.JogSpeed * fOptions.DefaultSprintMultiplier;
			bOrientRotationToMovement = true;
			break;

		case ControlState::TravelPower_Flight_Strafe:
			MaxFlySpeed = fOptions.JogSpeed;
			Velocity.Z = 0.0f;
			bOrientRotationToMovement = false;
			SetMovementMode(EMovementMode::MOVE_Flying);
			break;

		case ControlState::TravelPower_Flight_Forward:
			MaxFlySpeed = fOptions.MaxFlightForwardSpeed;
			bOrientRotationToMovement = false;
			SetMovementMode(EMovementMode::MOVE_Flying);
			break;

		default:
			break;
	}
}