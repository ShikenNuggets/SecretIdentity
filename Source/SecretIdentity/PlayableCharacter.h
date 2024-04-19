// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayableCharacter.generated.h"

struct FInputActionValue;
class UCameraComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UInputAction;
class UMusicAudioComponent;
class UPlayableAnimInstance;
class USpringArmComponent;
class AWindDirectionalSource;

class UPlayerCameraBoom;
class UPlayerCameraComponent;

enum class ControlState : uint8
{
	Default = 0,
	Sprinting,
	TravelPower_Flight_Strafe,
	TravelPower_Flight_Forward
};

UCLASS()
class SECRETIDENTITY_API APlayableCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayableCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// States
	void SwitchState(ControlState NewState);
	bool IsStateSwitchValid(ControlState OldState, ControlState NewState);

	void OnSwitchToDefaultState();
	void OnSwitchToSprintingState();
	void OnSwitchToTravelPowerFlightStrafeState();
	void OnSwitchToTravelPowerFlightForwardState();

	// Default Input
	void OnLookInput(const FInputActionValue& Value);
	void OnMoveInput(const FInputActionValue& Value);
	void OnSprintInput(const FInputActionValue& Value);
	void OnEnableTravelPowerInput(const FInputActionValue& Value);

	// Flight Input
	void OnFlightStrafeInput(const FInputActionValue& Value);
	void OnFlightForwardInput(const FInputActionValue& Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Player Movement Values ------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float JogSpeed = 600.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float DefaultSprintMultiplier = 2.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float DefaultJumpForce = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float DefaultAirControl = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float MaxFlightForwardSpeed = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float FlightStrafeRotationTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Playable Character", meta = (AllowPrivateAccess = "true"))
	float FlightForwardRotationTime = 0.5f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Global Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* GlobalMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Default Input ---------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* EnableTravelPowerAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Flight Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* FlightMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* FlightStrafeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* FlightForwardAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Camera ----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UPlayerCameraBoom* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UPlayerCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float DefaultFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float FlightFOV = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float DefaultFollowDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float FlightFollowDistance = 300.0f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Music -----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* CalmFlyingMusicComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* HeavyFlyingMusicComponent;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Cape -----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cape", meta = (AllowPrivateAccess = "true"))
	AWindDirectionalSource* WindSource;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Internal Only Values --------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPlayableAnimInstance* uAnimInstance;
	UEnhancedInputLocalPlayerSubsystem* uInputSubsystem;
	ControlState eControlState;

	bool bHasTargetRotation;
	FRotator fStartRotation;
	FRotator fTargetRotation;
	float fRotationTimer;

	void SetTargetRotation(const FRotator& Target);
};