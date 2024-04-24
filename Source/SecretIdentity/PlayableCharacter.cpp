// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayableCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/WindDirectionalSource.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "PlayableAnimInstance.h"
#include "PlayerCameraBoom.h"
#include "PlayerCameraComponent.h"
#include "MusicAudioComponent.h"
#include "UE_Helpers.h"

// Sets default values
APlayableCharacter::APlayableCharacter()
{
	if (GetCapsuleComponent() != nullptr)
	{
		GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	}

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

		GetCharacterMovement()->JumpZVelocity = DefaultJumpForce;
		GetCharacterMovement()->AirControl = 0.35f;
		GetCharacterMovement()->MaxWalkSpeed = JogSpeed;
		GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
		GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
		GetCharacterMovement()->BrakingDecelerationFlying = 7500.0f;
		GetCharacterMovement()->GravityScale = 2.5f;
	}

	CameraBoom = CreateDefaultSubobject<UPlayerCameraBoom>(TEXT("CameraBoom"));
	if (CameraBoom != nullptr)
	{
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f)); //Camera should target the shoulders rather than the back
		CameraBoom->TargetArmLength = DefaultFollowDistance;
		CameraBoom->bUsePawnControlRotation = true;

		FollowCamera = CreateDefaultSubobject<UPlayerCameraComponent>(TEXT("FollowCamera"));
		if (FollowCamera != nullptr)
		{
			FollowCamera->SetupAttachment(CameraBoom, UPlayerCameraBoom::SocketName);
			FollowCamera->bUsePawnControlRotation = false;
			FollowCamera->FieldOfView = DefaultFOV;
		}
	}

	CalmFlyingMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CalmFlyingMusic"));
	HeavyFlyingMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HeavyFlyingMusic"));

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		uInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (uInputSubsystem)
		{
			uInputSubsystem->AddMappingContext(GlobalMappingContext, 99);
		}
	}

	if (GetMesh())
	{
		uAnimInstance = Cast<UPlayableAnimInstance>(GetMesh()->GetAnimInstance());
	}

	if (CalmFlyingMusicComponent)
	{
		CalmFlyingMusicComponent->Stop();
	}

	if (HeavyFlyingMusicComponent)
	{
		HeavyFlyingMusicComponent->Stop();
	}

	FVector windSourcePos = FVector(100.0f, 0.0f, 0.0f);
	FRotator windSourceRot = FRotator(-15.0f, -180.0f, 0.0f);
	if (GetWorld() != nullptr)
	{
		WindSource = GetWorld()->SpawnActor<AWindDirectionalSource>(windSourcePos, windSourceRot);
		if (WindSource != nullptr)
		{
			WindSource->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			if (WindSource->GetComponent() != nullptr)
			{
				WindSource->GetComponent()->Speed = 7.5f;
				WindSource->GetComponent()->Strength = 20.0f;
			}
			
		}
	}

	WARN_IF_NULL(GetWorld());
	WARN_IF_NULL(GetMesh());
	WARN_IF_NULL(GetCapsuleComponent());
	WARN_IF_NULL(GetCharacterMovement());

	WARN_IF_NULL(GlobalMappingContext);
	WARN_IF_NULL(DefaultMappingContext);
	WARN_IF_NULL(MoveAction);
	WARN_IF_NULL(LookAction);
	WARN_IF_NULL(JumpAction);
	WARN_IF_NULL(SprintAction);
	WARN_IF_NULL(EnableTravelPowerAction);

	WARN_IF_NULL(FlightMappingContext);
	WARN_IF_NULL(FlightStrafeAction);
	WARN_IF_NULL(FlightForwardAction);

	WARN_IF_NULL(CameraBoom);
	WARN_IF_NULL(FollowCamera);

	WARN_IF_NULL(CalmFlyingMusicComponent);
	WARN_IF_NULL(HeavyFlyingMusicComponent);

	WARN_IF_NULL(WindSource);

	WARN_IF_NULL(uAnimInstance);
	WARN_IF_NULL(uInputSubsystem);

	SwitchState(ControlState::Default);
}

// Called every frame
void APlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (eControlState == ControlState::TravelPower_Flight_Forward)
	{
		float velocityMagnitude = GetCharacterMovement()->Velocity.Size();

		if (uAnimInstance)
		{
			uAnimInstance->FlightMoveSpeed = velocityMagnitude;
		}

		if (FMath::IsNearlyZero(velocityMagnitude, 1.0f))
		{
			SwitchState(ControlState::TravelPower_Flight_Strafe);
		}
	}

	if (CameraBoom != nullptr)
	{
		CameraBoom->UpdateTimer(DeltaTime);
	}

	if (FollowCamera != nullptr)
	{
		FollowCamera->UpdateTimer(DeltaTime);
	}

	if (bHasTargetRotation)
	{
		fRotationTimer += DeltaTime;

		float fRotationChangeTime = FlightStrafeRotationTime;
		if (eControlState == ControlState::TravelPower_Flight_Forward)
		{
			fRotationChangeTime = FlightForwardRotationTime;
		}

		SetActorRotation(UKismetMathLibrary::Quat_Slerp(fStartRotation.Quaternion(), fTargetRotation.Quaternion(), fRotationTimer / fRotationChangeTime));
		
		if (fRotationTimer >= fRotationChangeTime)
		{
			bHasTargetRotation = false;
			SetActorRotation(fTargetRotation);
			fRotationTimer = 0.0f;
		}
	}
}

void APlayableCharacter::SwitchState(ControlState NewState)
{
	if (!IsStateSwitchValid(eControlState, NewState))
	{
		return;
	}

	eControlState = NewState;

	switch (eControlState)
	{
		case ControlState::Default:
			//LOG_MSG("Switching to default state");
			OnSwitchToDefaultState();
			break;

		case ControlState::Sprinting:
			//LOG_MSG("Switching to sprinting state");
			OnSwitchToSprintingState();
			break;

		case ControlState::TravelPower_Flight_Strafe:
			//LOG_MSG("Switching to flight strafing state");
			OnSwitchToTravelPowerFlightStrafeState();
			break;

		case ControlState::TravelPower_Flight_Forward:
			//LOG_MSG("Switching to flying forward state");
			OnSwitchToTravelPowerFlightForwardState();
			break;

		default:
			WARN_IF_MSG(true, "Unhandled eControlState case in APlayableCharacter::SwitchState!");
			break;
	}
}

bool APlayableCharacter::IsStateSwitchValid(ControlState OldState, ControlState NewState)
{
	//Cannot start sprinting while flying
	if ((OldState == ControlState::TravelPower_Flight_Strafe || OldState == ControlState::TravelPower_Flight_Forward) && NewState == ControlState::Sprinting)
	{
		return false;
	}

	//Must move from flight strafe to flight forward, cannot move from other states directly to flying forward
	if (OldState != ControlState::TravelPower_Flight_Strafe && NewState == ControlState::TravelPower_Flight_Forward)
	{
		return false;
	}

	return true;
}

void APlayableCharacter::OnSwitchToDefaultState()
{
	bHasTargetRotation = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = JogSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	if (CameraBoom)
	{
		CameraBoom->SetTargetFollowDistance(DefaultFollowDistance);
	}

	if (FollowCamera != nullptr)
	{
		FollowCamera->SetTargetFOV(DefaultFOV);
	}

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = false;
	}

	if (uInputSubsystem)
	{
		uInputSubsystem->RemoveMappingContext(FlightMappingContext);
		uInputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	if (CalmFlyingMusicComponent != nullptr)
	{
		CalmFlyingMusicComponent->FadeOut(1.0f, 0.0f);
	}

	if (HeavyFlyingMusicComponent != nullptr)
	{
		HeavyFlyingMusicComponent->FadeOut(1.0f, 0.0f);
	}
}

void APlayableCharacter::OnSwitchToSprintingState()
{
	bHasTargetRotation = false;

	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->MaxWalkSpeed = JogSpeed * DefaultSprintMultiplier;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = true;
	}
}

void APlayableCharacter::OnSwitchToTravelPowerFlightStrafeState()
{
	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->MaxFlySpeed = JogSpeed;
		GetCharacterMovement()->Velocity.Z = 0.0f;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	}
	
	if (CameraBoom != nullptr)
	{
		CameraBoom->SetTargetFollowDistance(FlightFollowDistance);
	}

	if (FollowCamera)
	{
		FollowCamera->SetTargetFOV(FlightFOV);
	}	

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = false;
		uAnimInstance->FlightMoveSpeed = 0.0f;
	}

	if (uInputSubsystem)
	{
		uInputSubsystem->RemoveMappingContext(DefaultMappingContext);
		uInputSubsystem->AddMappingContext(FlightMappingContext, 0);
	}

	if (HeavyFlyingMusicComponent != nullptr)
	{
		HeavyFlyingMusicComponent->FadeOut(1.0f, 0.0f);
	}

	if (CalmFlyingMusicComponent != nullptr)
	{
		CalmFlyingMusicComponent->Stop();
		CalmFlyingMusicComponent->FadeIn(1.0f);
	}
}

void APlayableCharacter::OnSwitchToTravelPowerFlightForwardState()
{
	if (GetCharacterMovement() != nullptr)
	{
		GetCharacterMovement()->MaxFlySpeed = MaxFlightForwardSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	}

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = false;
	}

	if (uInputSubsystem)
	{
		uInputSubsystem->RemoveMappingContext(DefaultMappingContext);
		uInputSubsystem->AddMappingContext(FlightMappingContext, 0);
	}

	if (CalmFlyingMusicComponent != nullptr)
	{
		CalmFlyingMusicComponent->FadeOut(1.0f, 0.0f);
	}

	if (HeavyFlyingMusicComponent != nullptr)
	{
		HeavyFlyingMusicComponent->Stop();
		HeavyFlyingMusicComponent->FadeIn(1.0f);
	}
}

// Called to bind functionality to input
void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Global
		EnhancedInputComponent->BindAction(EnableTravelPowerAction, ETriggerEvent::Triggered, this, &APlayableCharacter::OnEnableTravelPowerInput);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayableCharacter::OnLookInput);

		//Default
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayableCharacter::OnMoveInput);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayableCharacter::OnSprintInput);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayableCharacter::OnSprintInput);

		//Flight
		EnhancedInputComponent->BindAction(FlightStrafeAction, ETriggerEvent::Triggered, this, &APlayableCharacter::OnFlightStrafeInput);
		EnhancedInputComponent->BindAction(FlightForwardAction, ETriggerEvent::Triggered, this, &APlayableCharacter::OnFlightForwardInput);
	}
	else if(GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, TEXT("APlayableCharacter requires an Enhanced Input Component!"));
	}
}

void APlayableCharacter::OnLookInput(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (eControlState == ControlState::TravelPower_Flight_Forward)
	{
		LookAxisVector.X /= 4.0f;
		LookAxisVector.Y /= 4.0f;
	}

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlayableCharacter::OnMoveInput(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayableCharacter::OnSprintInput(const FInputActionValue& Value)
{
	bool isSprinting = Value.Get<bool>();
	if (isSprinting && eControlState != ControlState::Sprinting)
	{
		SwitchState(ControlState::Sprinting);
	}
	else if (!isSprinting && eControlState == ControlState::Sprinting)
	{
		SwitchState(ControlState::Default);
	}
}

void APlayableCharacter::OnEnableTravelPowerInput(const FInputActionValue& Value)
{
	if (eControlState == ControlState::TravelPower_Flight_Strafe || eControlState == ControlState::TravelPower_Flight_Forward)
	{
		SwitchState(ControlState::Default);
	}
	else
	{
		SwitchState(ControlState::TravelPower_Flight_Strafe);
	}
}

void APlayableCharacter::OnFlightStrafeInput(const FInputActionValue& Value)
{
	// Can only strafe while already in the strafe state
	if (eControlState != ControlState::TravelPower_Flight_Strafe)
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		const FVector UpDirection = FVector(0.0f, 0.0f, 1.0f);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(UpDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);

		SetTargetRotation(YawRotation);
	}
}

void APlayableCharacter::OnFlightForwardInput(const FInputActionValue& Value)
{
	float moveValue = FMath::Clamp(Value.Get<float>(), 0.0f, 1.0f);

	if (moveValue > 0.1f && eControlState != ControlState::TravelPower_Flight_Forward)
	{
		SwitchState(ControlState::TravelPower_Flight_Forward);
	}
	else if (moveValue <= 0.1f)
	{
		SwitchState(ControlState::TravelPower_Flight_Strafe);
		return;
	}

	const FRotator Rotation = Controller->GetControlRotation();
	const FVector ForwardDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);

	AddMovementInput(ForwardDirection, moveValue);
	SetTargetRotation(Rotation);
}

void APlayableCharacter::SetTargetRotation(const FRotator& Target)
{
	if (bHasTargetRotation && fTargetRotation == Target)
	{
		return;
	}

	bHasTargetRotation = true;
	fStartRotation = GetActorRotation();
	fTargetRotation = Target;
	fRotationTimer = 0.0f;
}