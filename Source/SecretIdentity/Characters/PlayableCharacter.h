// Copyright Carter Rennick, 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "SecretIdentity/SecretIdentity.h"

#include "PlayableCharacter.generated.h"

struct FInputActionValue;
class ACombatFieldSystemActor;
class AWindDirectionalSource;
class UCameraComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UInputAction;
class UMusicAudioComponent;
class UPlayableAnimInstance;
class UPlayCharacterMovementComponent;
class USphereComponent;
class USpringArmComponent;
class UUserWidget;

class UCombatColliderComponent;
class UMusicPlayer;
class UPlayerCameraBoom;
class UPlayerCameraComponent;

//Main Player Character
UCLASS()
class SECRETIDENTITY_API APlayableCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayableCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void PossessedBy(AController* NewController) override;

	virtual void BeginPlay() override;

	// States
	void SwitchState(EPlayerControlState NewState);
	bool IsStateSwitchValid(EPlayerControlState OldState, EPlayerControlState NewState) const;

	void OnSwitchToNoneState();
	void OnSwitchToDefaultState();
	void OnSwitchToSprintingState();
	void OnSwitchToPunchState();
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

	// Combat Input
	void OnPunchInput(const FInputActionValue& Value);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Movement", meta = (AllowPrivateAccess = "true"))
	float FlightStrafeRotationTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight Movement", meta = (AllowPrivateAccess = "true"))
	float FlightForwardRotationTime = 0.5f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Global Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Global)", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* GlobalMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Global)", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Default Input ---------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* EnableTravelPowerAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Flight Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* FlightMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	UInputAction* FlightStrafeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	UInputAction* FlightForwardAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Combat Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Combat)", meta = (AllowPrivateAccess = "true"))
	UInputAction* PunchAction;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Camera ----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UPlayerCameraBoom* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UPlayerCameraComponent* FollowCamera;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Music -----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music", meta = (AllowPrivateAccess = "true"))
	UMusicPlayer* MusicPlayer;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Cape -----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cape", meta = (AllowPrivateAccess = "true"))
	AWindDirectionalSource* WindSource;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Combat ----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACombatFieldSystemActor> FieldSystemActorBP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatColliderComponent* RightHandCollider;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	ACombatFieldSystemActor* RightHandFieldSystem;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Internal --------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	FPlayerControlStateDelegate OnPlayerStateChangedDelegate;

	UPlayCharacterMovementComponent* uMovementComponent;
	UPlayableAnimInstance* uAnimInstance;
	UEnhancedInputLocalPlayerSubsystem* uInputSubsystem;
	EPlayerControlState eControlState;

	bool bHasTargetRotation;
	FRotator fStartRotation;
	FRotator fTargetRotation;
	float fRotationTimer;

	void OnControlBegins(); //Called when the pawn is possessed
	void SetTargetRotation(const FRotator& Target);

public:
	void OnAnimNotifyTriggerEnableHandCollision();
	void OnAnimNotifyTriggerDisableHandCollision();
	void OnAnimNotifyTriggerEndPunching();
};