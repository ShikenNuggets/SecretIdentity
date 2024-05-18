// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "PlayableCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/WindDirectionalSource.h"
#include "Field/FieldSystemActor.h"
#include "Field/FieldSystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Actors/CombatFieldSystemActor.h"
#include "SecretIdentity/Components/PlayerCameraBoom.h"
#include "SecretIdentity/Components/PlayerCameraComponent.h"
#include "SecretIdentity/Components/PlayCharacterMovementComponent.h"
#include "SecretIdentity/SceneComponents/MusicPlayer.h"
#include "SecretIdentity/UObjects/PlayableAnimInstance.h"

// Sets default values
APlayableCharacter::APlayableCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	if (GetCapsuleComponent() != nullptr)
	{
		GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	}

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	uMovementComponent = Cast<UPlayCharacterMovementComponent>(GetCharacterMovement());
	if (uMovementComponent != nullptr)
	{
		FPlayCharacterMovementOptions options;
		options.JogSpeed = JogSpeed;
		options.DefaultSprintMultiplier = DefaultSprintMultiplier;
		options.DefaultJumpForce = DefaultJumpForce;
		options.MaxFlightForwardSpeed = MaxFlightForwardSpeed;
		options.FlightStrafeRotationTime = FlightStrafeRotationTime;
		options.FlightForwardRotationTime = FlightForwardRotationTime;

		uMovementComponent->SetOptions(options);
		OnPlayerStateChangedDelegate.AddUObject(uMovementComponent, &UPlayCharacterMovementComponent::OnPlayerStateChanged);
	}

	CameraBoom = CreateDefaultSubobject<UPlayerCameraBoom>(TEXT("CameraBoom"));
	if (CameraBoom != nullptr)
	{
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f)); //Camera should target the shoulders rather than the back
		OnPlayerStateChangedDelegate.AddUObject(CameraBoom, &UPlayerCameraBoom::OnPlayerStateChanged);

		FollowCamera = CreateDefaultSubobject<UPlayerCameraComponent>(TEXT("FollowCamera"));
		if (FollowCamera != nullptr)
		{
			FollowCamera->SetupAttachment(CameraBoom, UPlayerCameraBoom::SocketName);
			OnPlayerStateChangedDelegate.AddUObject(FollowCamera, &UPlayerCameraComponent::OnPlayerStateChanged);
		}
	}

	MusicPlayer = CreateDefaultSubobject<UMusicPlayer>(TEXT("MusicPlayer"));
	if (MusicPlayer != nullptr)
	{
		MusicPlayer->SetupAttachment(RootComponent);
		OnPlayerStateChangedDelegate.AddUObject(MusicPlayer, &UMusicPlayer::OnPlayerStateChanged);
	}

	if (GetMesh())
	{
		RightHandCollider = CreateDefaultSubobject<USphereComponent>(TEXT("RightHandCollider"));
		if (RightHandCollider != nullptr)
		{
			RightHandCollider->SetupAttachment(GetMesh(), TEXT("middle_03_r"));
			RightHandCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			RightHandCollider->SetSphereRadius(16.0f);
		}
	}
}

// Called when the game starts or when spawned
void APlayableCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	WARN_IF_NULL(Controller);
	if (Controller != nullptr)
	{
		WARN_IF_NULL(Cast<APlayerController>(Controller));
		LOG_MSG(Controller->GetClass()->GetFName().ToString());
	}
	
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
		if (uAnimInstance != nullptr)
		{
			uAnimInstance->OnTriggerEnableHandCollision.AddUObject(this, &APlayableCharacter::OnAnimNotifyTriggerEnableHandCollision);
			uAnimInstance->OnTriggerDisableHandCollision.AddUObject(this, &APlayableCharacter::OnAnimNotifyTriggerDisableHandCollision);
			uAnimInstance->OnTriggerEndPunching.AddUObject(this, &APlayableCharacter::OnAnimNotifyTriggerEndPunching);
		}
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

	if (FieldSystemActorBP != nullptr)
	{
		RightHandFieldSystem = GetWorld()->SpawnActor<ACombatFieldSystemActor>(FieldSystemActorBP, FVector(0.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
		if (RightHandFieldSystem != nullptr)
		{
			RightHandFieldSystem->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("middle_03_r"));
			RightHandFieldSystem->SetFieldActive(true);
			RightHandFieldSystem->DrawDebugInfo(false);
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

	WARN_IF_NULL(PunchAction);

	WARN_IF_NULL(CameraBoom);
	WARN_IF_NULL(FollowCamera);

	WARN_IF_NULL(MusicPlayer);

	WARN_IF_NULL(WindSource);

	WARN_IF_NULL(FieldSystemActorBP);
	WARN_IF_NULL(RightHandCollider);
	WARN_IF_NULL(RightHandFieldSystem);

	WARN_IF_NULL(uMovementComponent);
	WARN_IF_NULL(uAnimInstance);
	WARN_IF_NULL(uInputSubsystem);

	SwitchState(EPlayerControlState::Default);
}

// Called every frame
void APlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (eControlState == EPlayerControlState::TravelPower_Flight_Forward)
	{
		float velocityMagnitude = GetCharacterMovement()->Velocity.Size();

		if (uAnimInstance)
		{
			uAnimInstance->FlightMoveSpeed = velocityMagnitude;
		}

		if (FMath::IsNearlyZero(velocityMagnitude, 1.0f))
		{
			SwitchState(EPlayerControlState::TravelPower_Flight_Strafe);
		}
	}

	if (bHasTargetRotation)
	{
		fRotationTimer += DeltaTime;

		float fRotationChangeTime = FlightStrafeRotationTime;
		if (eControlState == EPlayerControlState::TravelPower_Flight_Forward)
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

void APlayableCharacter::SwitchState(EPlayerControlState NewState)
{
	if (!IsStateSwitchValid(eControlState, NewState))
	{
		return;
	}

	eControlState = NewState;

	switch (eControlState)
	{
		case EPlayerControlState::Default:
			OnSwitchToDefaultState();
			break;

		case EPlayerControlState::Sprinting:
			OnSwitchToSprintingState();
			break;

		case EPlayerControlState::Punching:
			OnSwitchToPunchState();
			break;

		case EPlayerControlState::TravelPower_Flight_Strafe:
			OnSwitchToTravelPowerFlightStrafeState();
			break;

		case EPlayerControlState::TravelPower_Flight_Forward:
			OnSwitchToTravelPowerFlightForwardState();
			break;

		default:
			WARN_IF_MSG(true, "Unhandled eControlState case in APlayableCharacter::SwitchState!");
			break;
	}

	OnPlayerStateChangedDelegate.Broadcast(NewState);
}

bool APlayableCharacter::IsStateSwitchValid(EPlayerControlState OldState, EPlayerControlState NewState) const
{
	if (NewState >= EPlayerControlState::Count)
	{
		WARN_IF_MSG(true, "Tried to switch player to invalid state!");
		return false;
	}

	//Cannot start sprinting while flying
	if ((OldState == EPlayerControlState::TravelPower_Flight_Strafe || OldState == EPlayerControlState::TravelPower_Flight_Forward) && NewState == EPlayerControlState::Sprinting)
	{
		return false;
	}
	
	//Cannot punch in mid-air
	if (NewState == EPlayerControlState::Punching && (GetCharacterMovement()->IsFlying() || GetCharacterMovement()->IsFalling()))
	{
		return false;
	}

	//Cannot start punching while flying
	if ((OldState == EPlayerControlState::TravelPower_Flight_Strafe || OldState == EPlayerControlState::TravelPower_Flight_Forward) && NewState == EPlayerControlState::Punching)
	{
		return false;
	}

	//Cannot fly while punching
	if (OldState == EPlayerControlState::Punching && (NewState == EPlayerControlState::TravelPower_Flight_Strafe || NewState == EPlayerControlState::TravelPower_Flight_Forward))
	{
		return false;
	}

	//Must move from flight strafe to flight forward, cannot move from other states directly to flying forward
	if (OldState != EPlayerControlState::TravelPower_Flight_Strafe && NewState == EPlayerControlState::TravelPower_Flight_Forward)
	{
		return false;
	}

	return true;
}

void APlayableCharacter::OnSwitchToDefaultState()
{
	bHasTargetRotation = false;

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = false;
		uAnimInstance->IsPunching = false;
	}

	if (uInputSubsystem)
	{
		uInputSubsystem->RemoveMappingContext(FlightMappingContext);
		uInputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	OnAnimNotifyTriggerDisableHandCollision(); //This is redundant - Just in case the punch animation gets interrupted or something
}

void APlayableCharacter::OnSwitchToSprintingState()
{
	bHasTargetRotation = false;

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = true;
	}
}

void APlayableCharacter::OnSwitchToPunchState()
{
	bHasTargetRotation = false;

	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = false;
		uAnimInstance->IsPunching = true;
	}
}

void APlayableCharacter::OnSwitchToTravelPowerFlightStrafeState()
{
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
}

void APlayableCharacter::OnSwitchToTravelPowerFlightForwardState()
{
	if (uAnimInstance)
	{
		uAnimInstance->IsSprinting = false;
	}

	if (uInputSubsystem)
	{
		uInputSubsystem->RemoveMappingContext(DefaultMappingContext);
		uInputSubsystem->AddMappingContext(FlightMappingContext, 0);
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

		//Combat
		EnhancedInputComponent->BindAction(PunchAction, ETriggerEvent::Triggered, this, &APlayableCharacter::OnPunchInput);
	}
	else if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, TEXT("APlayableCharacter requires an Enhanced Input Component!"));
	}
}

void APlayableCharacter::OnLookInput(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (eControlState == EPlayerControlState::TravelPower_Flight_Forward)
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
	if (isSprinting && eControlState != EPlayerControlState::Sprinting)
	{
		SwitchState(EPlayerControlState::Sprinting);
	}
	else if (!isSprinting && eControlState == EPlayerControlState::Sprinting)
	{
		SwitchState(EPlayerControlState::Default);
	}
}

void APlayableCharacter::OnEnableTravelPowerInput(const FInputActionValue& Value)
{
	if (eControlState == EPlayerControlState::TravelPower_Flight_Strafe || eControlState == EPlayerControlState::TravelPower_Flight_Forward)
	{
		SwitchState(EPlayerControlState::Default);
	}
	else
	{
		SwitchState(EPlayerControlState::TravelPower_Flight_Strafe);
	}
}

void APlayableCharacter::OnFlightStrafeInput(const FInputActionValue& Value)
{
	// Can only strafe while already in the strafe state
	if (eControlState != EPlayerControlState::TravelPower_Flight_Strafe)
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

	if (moveValue > 0.1f && eControlState != EPlayerControlState::TravelPower_Flight_Forward)
	{
		SwitchState(EPlayerControlState::TravelPower_Flight_Forward);
	}
	else if (moveValue <= 0.1f)
	{
		SwitchState(EPlayerControlState::TravelPower_Flight_Strafe);
		return;
	}

	const FRotator Rotation = Controller->GetControlRotation();
	const FVector ForwardDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);

	AddMovementInput(ForwardDirection, moveValue);
	SetTargetRotation(Rotation);
}

void APlayableCharacter::OnPunchInput(const FInputActionValue& Value)
{
	bool punchButtonDown = Value.Get<bool>();

	if (punchButtonDown)
	{
		SwitchState(EPlayerControlState::Punching);
	}
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

void APlayableCharacter::OnAnimNotifyTriggerEnableHandCollision()
{
	if (RightHandCollider != nullptr)
	{
		RightHandCollider->SetCollisionProfileName(CollisionProfiles::OverlapAllDynamic);
		RightHandCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (RightHandFieldSystem != nullptr)
	{
		RightHandFieldSystem->SetFieldActive(true);
		RightHandFieldSystem->DrawDebugInfo(true);
	}
}

void APlayableCharacter::OnAnimNotifyTriggerDisableHandCollision()
{
	if (RightHandCollider)
	{
		RightHandCollider->SetCollisionProfileName(CollisionProfiles::NoCollision);
	}

	if (RightHandFieldSystem != nullptr)
	{
		RightHandFieldSystem->SetFieldActive(false);
		RightHandFieldSystem->DrawDebugInfo(false);
	}
}

void APlayableCharacter::OnAnimNotifyTriggerEndPunching()
{
	SwitchState(EPlayerControlState::Default);
}