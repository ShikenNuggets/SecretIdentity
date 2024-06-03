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

class AArcadeGameMode;
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
	void OnPauseInput(const FInputActionValue& Value);
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
	float FlightForwardRotationTime = 0.15f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Global Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Global)", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* GlobalMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Global)", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Global)", meta = (AllowPrivateAccess = "true"))
	UInputAction* PauseAction = nullptr;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Default Input ---------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Default State)", meta = (AllowPrivateAccess = "true"))
	UInputAction* EnableTravelPowerAction = nullptr;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Flight Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* FlightMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	UInputAction* FlightStrafeAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	UInputAction* FlightForwardAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Flight)", meta = (AllowPrivateAccess = "true"))
	float ControllerSensitivityDivisorWhileFlying = 1.5f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Combat Input ----------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input (Combat)", meta = (AllowPrivateAccess = "true"))
	UInputAction* PunchAction = nullptr;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Camera ----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UPlayerCameraBoom* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UPlayerCameraComponent* FollowCamera = nullptr;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Music -----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Music", meta = (AllowPrivateAccess = "true"))
	UMusicPlayer* MusicPlayer = nullptr;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Cape -----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cape", meta = (AllowPrivateAccess = "true"))
	AWindDirectionalSource* WindSource = nullptr;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Combat ----------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACombatFieldSystemActor> FieldSystemActorBP = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatColliderComponent* RightHandCollider = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	ACombatFieldSystemActor* RightHandFieldSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float PunchMagnetRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float PunchMagnetMoveTime = 0.25f;

	//-----------------------------------------------------------------------------------------------------//
	//----------------------- Internal --------------------------------------------------------------------//
	//-----------------------------------------------------------------------------------------------------//
	FPlayerControlStateDelegate OnPlayerStateChangedDelegate;

	UPlayCharacterMovementComponent* uMovementComponent = nullptr;
	UPlayableAnimInstance* uAnimInstance = nullptr;
	UEnhancedInputLocalPlayerSubsystem* uInputSubsystem = nullptr;
	AArcadeGameMode* aGameMode = nullptr;
	EPlayerControlState eControlState = EPlayerControlState::None;

	bool bHasTargetPosition = true;
	bool bHasTargetRotation = false;
	FVector fStartPosition = FVector::Zero();
	AActor* fTargetForPosition = nullptr;
	FRotator fStartRotation = FRotator::ZeroRotator;
	FRotator fTargetRotation = FRotator::ZeroRotator;
	float fPositionTimer = 0.0f;
	float fRotationTimer = 0.0f;
	bool bIsHoldingSprintKey = false;

	void OnControlBegins(); //Called when the pawn is possessed
	void SetTargetForPosition(AActor* Target);
	void SetTargetRotation(const FRotator& Target);

public:
	void OnAnimNotifyTriggerEnableHandCollision();
	void OnAnimNotifyTriggerDisableHandCollision();
	void OnAnimNotifyTriggerEndPunching();
};