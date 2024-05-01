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

void UPlayCharacterMovementComponent::OnPlayerStateChanged(EPlayerControlState State)
{
	switch (State)
	{
		case EPlayerControlState::Default:
			MaxWalkSpeed = fOptions.JogSpeed;
			bOrientRotationToMovement = true;
			SetMovementMode(EMovementMode::MOVE_Walking);
			break;

		case EPlayerControlState::Sprinting:
			MaxWalkSpeed = fOptions.JogSpeed * fOptions.DefaultSprintMultiplier;
			bOrientRotationToMovement = true;
			break;

		case EPlayerControlState::Punching:
			MaxWalkSpeed = fOptions.JogSpeed;
			bOrientRotationToMovement = true;
			SetMovementMode(EMovementMode::MOVE_None);
			break;

		case EPlayerControlState::TravelPower_Flight_Strafe:
			MaxFlySpeed = fOptions.JogSpeed;
			Velocity.Z = 0.0f;
			bOrientRotationToMovement = false;
			SetMovementMode(EMovementMode::MOVE_Flying);
			break;

		case EPlayerControlState::TravelPower_Flight_Forward:
			MaxFlySpeed = fOptions.MaxFlightForwardSpeed;
			bOrientRotationToMovement = false;
			SetMovementMode(EMovementMode::MOVE_Flying);
			break;

		default:
			WARN_IF_MSG(true, "EPlayerControlState case not handled in UPlayCharacterMovementComponent::OnPlayerStateChanged!");
			break;
	}
}