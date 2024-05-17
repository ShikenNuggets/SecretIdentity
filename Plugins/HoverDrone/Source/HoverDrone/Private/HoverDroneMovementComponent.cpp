// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDroneMovementComponent.h"
#include "HoverDroneSpeedLimitBox.h"
#include "HoverDronePawn.h"
#include "HoverDroneUtils.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/HUD.h"
#include "Engine/Canvas.h"
#include "DisplayDebugHelpers.h"

#define LOCTEXT_NAMESPACE "HoverDroneMovementComponent"

UHoverDroneMovementComponent::UHoverDroneMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxSpeed = 30000.f;
	
	Acceleration = 5000.f;
	Deceleration = 10000.f;
	MaxPitchRotSpeed = 70.f;
	MaxYawRotSpeed = 110.f;
	RotAcceleration = 150.f;
	RotDeceleration = 150.f;

	MinSpeedHeight = 0.f;
	MaxSpeedHeight = 0.f;
	MaxSpeedHeightMultiplier = 1.f;

	FullAirFrictionVelocity = 3000.f;
	MinAirFriction = 0.01f;

	bSimulateRotation = true;

	bMaintainHoverHeight = false;
	MaintainHoverHeightTolerance = 150.f;		// 1.5m
	MaintainHoverHeightPredictionTime = 1.f;
	DesiredHoverHeight = 400.f;
	MinHoverHeight = 150.f;

	// tuned for Athena_Terrain. Much higher than this and you can get above the storm clouds
	DroneMaxAltitude = 50000.f;

	MinPitch = -88.f;
	MaxPitch = 88.f;

	ForceFacingInterpInTime = 1.5f;
	
	DirectRotationInputYawScale = 1.f;
	DirectRotationInputPitchScale = 1.f;
	DirectRotationInputInterpSpeed = 6.f;

	MaxSimulationTimestep = 1.f / 60.f;

	bIgnoreTimeDilation = true;

	DroneSpeedParamIndex = 3;

	PrimaryComponentTick.bTickEvenWhenPaused = true;

	ForceFacingPlayerLocalOffset = FVector(0.f, 0.f, 70.f);

	FollowedActorPositionInterpSpeed = 5.f;

	MovementAccelFactor = FVector(1.0f, 1.0f, 1.0f);

	DroneSpeedParameters = {
		//						LinAcc		LinDec		RotAcc		RotDec		MaxRotSpd	Thrust	
		FDroneSpeedParameters(	0.025f,		0.025f,		0.08f,		0.05f,		0.75f,		0.03f		),
		FDroneSpeedParameters(	0.05f,		0.05f,		0.15f,		0.1f,		0.1f,		0.05f		),
		FDroneSpeedParameters(	0.1f,		0.1f,		0.3f,		0.2f,		0.2f,		0.1f		),
		FDroneSpeedParameters(	0.25f,		0.25f,		0.6f,		0.35f,		0.35f,		0.25f		),
		FDroneSpeedParameters(	0.5f,		0.5f,		0.6f,		0.7f,		0.7f,		0.5f		),
		FDroneSpeedParameters(	1.f,		1.f,		1.f,		1.f,		1.f,		1.f			),
		FDroneSpeedParameters(	2.f,		2.f,		1.5f,		2.f,		1.3f,		2.f			),
		FDroneSpeedParameters(  3.5f,		4.f,		1.5f,		2.f,		1.3f,		4.f			),
	};

	MaxAllowedSpeedIndex = DroneSpeedParameters.Num() - 1;
}

void UHoverDroneMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UpdatedComponent)
	{
		DirectRotationInputGoalRotation = UpdatedComponent->GetComponentRotation();
	}
}

// internal
static void LimitControlAccelOnAxis(float& AxisAccel, float AxisPos, float LimitMin, float LimitMax)
{
	if ( (AxisAccel != 0.f) && (LimitMin < LimitMax) )
	{
		if (AxisAccel < 0.f)
		{
			// invalidate controls if oob on the min
			if (AxisPos < LimitMin)
			{
				AxisAccel = 0.f;
			}
		}
		else
		{
			// invalidate controls if oob on the max
			if (AxisPos > LimitMax)
			{
				AxisAccel = 0.f;
			}
		}
	}
}

void UHoverDroneMovementComponent::ApplyControlInputToVelocity(float DeltaTime)
{
	check(DroneSpeedParameters.IsValidIndex(DroneSpeedParamIndex));

	FVector ControlAcceleration = GetPendingInputVector().GetClampedToMaxSize(1.f) * DroneSpeedParameters[DroneSpeedParamIndex].LinearAccelScale;
	//UE_LOG(LogHoverDrone, Display, TEXT("%s"), *MovementAccelFactor.ToCompactString());
	ControlAcceleration *= MovementAccelFactor;
	ControlAcceleration *= DroneSpeedScalar;
	
	bool bMaintainHeight = bMaintainHoverHeight;
	if (ControlAcceleration.Z != 0.f)
	{
		// user is adjusting height, let them and reset the hover height when they are done
		bMaintainHeight = false;
		DesiredHoverHeight = -1.f;
	}
	
	// make sure that if players are out of bounds, we don't let them push farther out of bounds but we do
	// let them push back in.
	// Also apply any speed limitation we want to place on the drone
	UpdatedMaxAllowedSpeed(UEHoverDrone::ApplyDroneLimiters(GetOwner(), ControlAcceleration));
	
	// basic Z thrust exactly counteracts gravity
	float ZThrust = -GetGravityZ();

	const float FixedTimeStep = .008f;
	for (float RemainingTime = DeltaTime; RemainingTime > 0.0f; RemainingTime -= FixedTimeStep)
	{
		float DT = FMath::Min(FixedTimeStep, RemainingTime);
		if (bMaintainHeight)
		{
			FVector AccumulatedThrust(0.f);
			float HoverThrust = 0.f;

			// look directly beneath us
			if (CurrentAltitude > 0.f)
			{
				float CurrentHeight = CurrentAltitude;
				if (DesiredHoverHeight <= 0)
				{
					DesiredHoverHeight = FMath::Max(CurrentHeight, MinHoverHeight);
				}
				else
				{
					// check height a few secs into the future so we don't have to overshoot to correct
					static float ProjectionTime = 0.6f;
					float ProjectedHeight = CurrentHeight + Velocity.Z * ProjectionTime;

					if (ProjectedHeight < (DesiredHoverHeight - MaintainHoverHeightTolerance))
					{
						// thrust up!
						static FVector2D ThrustMagRange(100.f, 4000.f);
						FVector2D ThrustDistRange((DesiredHoverHeight - MaintainHoverHeightTolerance), 0);
						float RealThrustMag = FMath::GetMappedRangeValueClamped(ThrustDistRange, ThrustMagRange, ProjectedHeight);
						HoverThrust += RealThrustMag;
					}
					else if (ProjectedHeight > (DesiredHoverHeight + MaintainHoverHeightTolerance))
					{
						// turn on some portion of gravity to come down
						static float MaxThrustDistance = 1000.f;
						static FVector2D ThrustMagRange(500.f, 3000.f);
						FVector2D ThrustDistRange((DesiredHoverHeight + MaintainHoverHeightTolerance), (DesiredHoverHeight + MaxThrustDistance));
						float RealThrustMag = FMath::GetMappedRangeValueClamped(ThrustDistRange, ThrustMagRange, ProjectedHeight);
						HoverThrust -= RealThrustMag;
					}
				}
			}

			// look ahead a certain time
			FVector XYVel = Velocity;
			XYVel.Z = 0.f;
			if (XYVel.IsNearlyZero() == false)
			{
				static float AltTestHeightAdjust = 10000.f;
				float const PredictedHeight = MeasureAltitude(PawnOwner->GetActorLocation() + (XYVel * MaintainHoverHeightPredictionTime) + FVector(0, 0, AltTestHeightAdjust)) - 10000.f;
				if (PredictedHeight > -AltTestHeightAdjust)
				{
					// else no ground below us
					if (PredictedHeight < (DesiredHoverHeight - MaintainHoverHeightTolerance))
					{
						// thrust up!
						static FVector2D ThrustMagRange(100.f, 4000.f);
						FVector2D ThrustDistRange((DesiredHoverHeight - MaintainHoverHeightTolerance), 0);
						float RealThrustMag = FMath::GetMappedRangeValueClamped(ThrustDistRange, ThrustMagRange, PredictedHeight);

						// favor going up, even if probe directly below us says we're too high
						HoverThrust = FMath::Max(HoverThrust, RealThrustMag);
					}
					else if (PredictedHeight > (DesiredHoverHeight + MaintainHoverHeightTolerance))
					{
						// don't override an "up" from the direct probe
						if (HoverThrust < 0.f)
						{
							// turn on some portion of gravity to come down
							static float MaxThrustDistance = 1000.f;
							static FVector2D ThrustMagRange(100.f, 980.f);
							FVector2D ThrustDistRange((DesiredHoverHeight + MaintainHoverHeightTolerance), (DesiredHoverHeight + MaxThrustDistance));
							float RealThrustMag = FMath::GetMappedRangeValueClamped(ThrustDistRange, ThrustMagRange, PredictedHeight);

							HoverThrust = FMath::Min(HoverThrust, -RealThrustMag);
						}
					}
				}
			}

			// thrust scaling
			HoverThrust *= DroneSpeedParameters[DroneSpeedParamIndex].HoverThrustScale * (1.f);

			ZThrust += HoverThrust;
		}

		float CurrentAccel = 0.f;
		float CurrentDecel = 0.f;
		CurrentAccel = Acceleration;
		CurrentDecel = Deceleration;

		// Apply various accelerations

		// friction deceleration
		float const CurVelMag = Velocity.Size();
		float AirFrictionScalar = (FullAirFrictionVelocity != 0.f) ? (CurVelMag / FullAirFrictionVelocity) : 1.f;
		AirFrictionScalar = FMath::Max(AirFrictionScalar, MinAirFriction);
		FVector const AntiVelocityDir = -(Velocity.GetSafeNormal());
		float AntiVelocityMag = CurrentDecel * DT * AirFrictionScalar;
		AntiVelocityMag = FMath::Clamp(AntiVelocityMag, 0.f, CurVelMag);		// don't let decel end up accelerating the other way
		Velocity += AntiVelocityDir * AntiVelocityMag;

		const float HeightInterpPercent = FMath::GetRangePct<>(CurrentAltitude, MinSpeedHeight, MaxSpeedHeight);
		const float HeightInterpPercentClamped = FMath::Clamp<>(HeightInterpPercent, 0.f, 1.f);
		const float HeightSpeedMultiplier = FMath::Max<>(1.f, FMath::Lerp<>(1.f, MaxSpeedHeightMultiplier, HeightInterpPercentClamped));
		
		// control acceleration
		Velocity += ControlAcceleration * FMath::Abs(CurrentAccel) * DT * HeightSpeedMultiplier;

		// gravity/thrust
		Velocity += FVector(0, 0, GetGravityZ() + ZThrust) * DT;
	}

	ConsumeInputVector();
}

void UHoverDroneMovementComponent::ApplyControlInputToRotation(float DeltaTime)
{
	// adjust rot accel and clamps for zoom
	float const FOVAdjScalar = GetInputFOVScale();
	float const AdjustedMaxYawRotSpeed = (MaxYawRotSpeed) * FOVAdjScalar * DroneSpeedParameters[DroneSpeedParamIndex].MaxRotSpeedScale;
	float const AdjustedMaxPitchRotSpeed = (MaxPitchRotSpeed) * FOVAdjScalar * DroneSpeedParameters[DroneSpeedParamIndex].MaxRotSpeedScale;
	float const AdjustedRotAccel = (RotAcceleration) * FOVAdjScalar * DroneSpeedParameters[DroneSpeedParamIndex].RotAccelScale;
	float const AdjustedRotDecel = (RotDeceleration) * FOVAdjScalar * DroneSpeedParameters[DroneSpeedParamIndex].RotDecelScale;

	const float FixedTimeStep = .008f;
	for (float RemainingTime = DeltaTime; RemainingTime > 0.0f; RemainingTime -= FixedTimeStep)
	{
		float DT = FMath::Min(FixedTimeStep, RemainingTime);

		FRotator const Input = RotationInput;
		if (Input.IsZero())
		{
			// Decelerate towards zero!
			if (RotVelocity.Yaw != 0.f)
			{
				float const YawVelDelta = AdjustedRotDecel * DT;
				if (RotVelocity.Yaw > 0.f)
				{
					// don't overshoot past zero to the neg
					RotVelocity.Yaw = (RotVelocity.Yaw > YawVelDelta) ? (RotVelocity.Yaw - YawVelDelta) : 0.f;
				}
				else
				{
					// yaw velocity is neg
					// don't overshoot past zero to the pos
					RotVelocity.Yaw = (RotVelocity.Yaw < -YawVelDelta) ? (RotVelocity.Yaw + YawVelDelta) : 0.f;
				}
			}

			if (RotVelocity.Pitch != 0.f)
			{
				float const PitchVelDelta = AdjustedRotDecel * DT;
				if (RotVelocity.Pitch > 0.f)
				{
					RotVelocity.Pitch = (RotVelocity.Pitch > PitchVelDelta) ? (RotVelocity.Pitch - PitchVelDelta) : 0.f;
				}
				else
				{
					// Pitch velocity is neg
					RotVelocity.Pitch = (RotVelocity.Pitch < -PitchVelDelta) ? (RotVelocity.Pitch + PitchVelDelta) : 0.f;
				}
			}
		}
		else
		{
			// updating rotation to avoid clamping and avoid overshooting badly on long frames!
			// don't let the delta take us out of bounds.
			float const MaxYawVelMag = FMath::Min(1.f, FMath::Abs(Input.Yaw)) * AdjustedMaxYawRotSpeed;
			if (RotVelocity.Yaw > MaxYawVelMag)
			{
				// going too fast in the positive dir, need to decelerate toward zero
				// but not past MaxYawVelMag
				float const YawVelDelta = FMath::Min((AdjustedRotDecel * DT), (RotVelocity.Yaw - MaxYawVelMag));
				RotVelocity.Yaw -= YawVelDelta;
			}
			else if (RotVelocity.Yaw < -MaxYawVelMag)
			{
				// going too fast in the negative dir, need to decelerate toward zero
				// but not past -MaxYawVelMag
				float const YawVelDelta = FMath::Min((AdjustedRotDecel * DT), FMath::Abs(RotVelocity.Yaw - MaxYawVelMag));
				RotVelocity.Yaw += YawVelDelta;
			}
			else
			{
				float const MaxDeltaYawVel = FMath::Max(0.f, MaxYawVelMag - RotVelocity.Yaw);
				float const MinDeltaYawVel = FMath::Min(0.f, -(RotVelocity.Yaw + MaxYawVelMag));
				float const DeltaYaw = FMath::Clamp(Input.Yaw * AdjustedRotAccel * DT, MinDeltaYawVel, MaxDeltaYawVel);
				RotVelocity.Yaw += DeltaYaw;
			}

			// now do pitch
			float const MaxPitchVelMag = FMath::Min(1.f, FMath::Abs(Input.Pitch)) * AdjustedMaxPitchRotSpeed;
			if (RotVelocity.Pitch > MaxPitchVelMag)
			{
				// going too fast in the positive dir, need to decelerate toward zero
				// but not past MaxPitchVelMag
				float const PitchVelDelta = FMath::Min((AdjustedRotDecel * DT), (RotVelocity.Pitch - MaxPitchVelMag));
				RotVelocity.Pitch -= PitchVelDelta;
			}
			else if (RotVelocity.Pitch < -MaxPitchVelMag)
			{
				// going too fast in the negative dir, need to decelerate toward zero
				// but not past -MaxPitchVelMag
				float const PitchVelDelta = FMath::Min((AdjustedRotDecel * DT), FMath::Abs(RotVelocity.Pitch - MaxPitchVelMag));
				RotVelocity.Pitch += PitchVelDelta;
			}
			else
			{
				float const MaxDeltaPitchVel = FMath::Max(0.f, MaxPitchVelMag - RotVelocity.Pitch);
				float const MinDeltaPitchVel = FMath::Min(0.f, -(RotVelocity.Pitch + MaxPitchVelMag));
				float const DeltaPitch = FMath::Clamp(Input.Pitch * AdjustedRotAccel * DT, MinDeltaPitchVel, MaxDeltaPitchVel);
				RotVelocity.Pitch += DeltaPitch;
			}
		}
	}
}

void UHoverDroneMovementComponent::AddRotationInput(FRotator NewRotInput)
{
	RotationInput += NewRotInput;
}

void UHoverDroneMovementComponent::AddDirectRotationInput(FRotator NewRotInput)
{
	DirectRotationInput += NewRotInput;
}

void UHoverDroneMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (!PawnOwner || !UpdatedComponent || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	// subclasses don't account for dilation, so do this adjustment before calling the super
	if (bIgnoreTimeDilation)
	{
		DeltaTime = FMath::Clamp(DeltaTime / PawnOwner->GetActorTimeDilation(), KINDA_SMALL_NUMBER, 0.05f);
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	// Note: we intentionally skip over SpectatorPawnMovement::Tick because its bIgnoreTimeDilation implementation is problematic for us
	// This call will translate the drone. rotation will happen below.
	UFloatingPawnMovement::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update measured velocity (used by camera for tilt)
	// This is distinct from the simulation velocity becuase of the potential tethering adjustment above
	const FVector NewLocation = UpdatedComponent->GetComponentLocation();
	MeasuredVelocity = ((DeltaTime != 0.f) && !bResetInterpolation) ? ((NewLocation - OldLocation) / DeltaTime) : FVector::ZeroVector;

	// For now, we're using drone to playback replay camera sequencer
	// Long term, we should probably make a new component for this
// 	const AWorldSettings* const WorldSettings = PawnOwner->GetWorldSettings();
// 	if (SpectatorController && SpectatorController->CameraSequencerMode == EReplayCameraSequencerMode::Playback && !WorldSettings->Pauser)
// 	{
// 		return;
// 	}

	if (bSimulateRotation)
	{
		bool bSkipAccel = false;
		bool bSkipDirect = false;

		// resolve input conflicts
		if (DirectRotationInput.IsNearlyZero() == false)
		{
			// direct input takes precedence
			RotVelocity = FRotator::ZeroRotator;
			RotationInput = FRotator::ZeroRotator;
			bSkipAccel = true;
		}
		else if (RotationInput.IsNearlyZero() == false)
		{
			// we have accel input and no direct input, clear any direct stuff
			bSkipDirect = true;
		}

		if (!bSkipAccel)
		{
			float UnsimulatedTime = DeltaTime;
			while (UnsimulatedTime > KINDA_SMALL_NUMBER)
			{
				float const SimTime = ((MaxSimulationTimestep > 0.f) && (UnsimulatedTime > MaxSimulationTimestep))
					? FMath::Min(MaxSimulationTimestep, UnsimulatedTime * 0.5f)
					: UnsimulatedTime;			// all of it

				// simulate!

				AController const* Controller = PawnOwner->GetController();
				if (Controller && Controller->IsLocalPlayerController())
				{
					ApplyControlInputToRotation(SimTime);
				}

				FRotator RotDelta = RotVelocity * SimTime;

				// enforce pitch limits
				float const CurrentPitch = UpdatedComponent->GetComponentRotation().Pitch;
				float const MinDeltaPitch = MinPitch - CurrentPitch;
				float const MaxDeltaPitch = MaxPitch - CurrentPitch;
				float const OldPitch = RotDelta.Pitch;
				RotDelta.Pitch = FMath::Clamp(RotDelta.Pitch, MinDeltaPitch, MaxDeltaPitch);
				if (!FMath::IsNearlyEqual(OldPitch, RotDelta.Pitch, KINDA_SMALL_NUMBER))
				{
					// if we got clamped, zero the pitch velocity
					RotVelocity.Pitch = 0.f;
				}

				if (!RotDelta.IsNearlyZero())
				{
					FRotator const NewRot = UpdatedComponent->GetComponentRotation() + RotDelta;

					FHitResult Hit(1.f);
					SafeMoveUpdatedComponent(FVector::ZeroVector, NewRot, false, Hit);

					DirectRotationInputGoalRotation = DirectRotationInputGoalRotation + RotDelta;
				}

				UnsimulatedTime -= SimTime;
			}
		}

		// now handle direct input
		if (!bSkipDirect)
		{
			FRotator DirectRotDelta = DirectRotationInput * GetInputFOVScale();
			DirectRotDelta.Yaw *= DirectRotationInputYawScale;
			DirectRotDelta.Pitch *= DirectRotationInputPitchScale;

			// enforce pitch limits
			// we enforce on the delta so a big delta doesn't result in a flip
			float const CurrentPitch = DirectRotationInputGoalRotation.Pitch;
			float const MinDeltaPitch = MinPitch - CurrentPitch;
			float const MaxDeltaPitch = MaxPitch - CurrentPitch;
			DirectRotDelta.Pitch = FMath::Clamp(DirectRotDelta.Pitch, MinDeltaPitch, MaxDeltaPitch);

			DirectRotationInputGoalRotation += DirectRotDelta;

			FRotator const NewRot = FMath::RInterpTo(UpdatedComponent->GetComponentRotation(), DirectRotationInputGoalRotation, DeltaTime, DirectRotationInputInterpSpeed);

			FHitResult Hit(1.f);
			SafeMoveUpdatedComponent(FVector::ZeroVector, NewRot, false, Hit);
		}
		else
		{
			// do this late so it stays tight to the accel rotation
			DirectRotationInputGoalRotation = UpdatedComponent->GetComponentRotation();
		}

		// clear out any input, we've handled it (or ignored it)
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		LastDirectRotationInput = DirectRotationInput;
		LastRotationInput = RotationInput;
#endif
		
		RotationInput = FRotator::ZeroRotator;
		DirectRotationInput = FRotator::ZeroRotator;

		AController* const Controller = PawnOwner->GetController();
		if (Controller && Controller->IsLocalPlayerController())
		{
			Controller->SetControlRotation(UpdatedComponent->GetComponentRotation());
		}
	}


	// cache altitude
	CurrentAltitude = MeasureAltitude(PawnOwner->GetActorLocation());

	bResetInterpolation = false;
}

void UHoverDroneMovementComponent::OnRegister()
{
	Super::OnRegister();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	AHUD::OnShowDebugInfo.AddUObject(this, &ThisClass::ShowDebugInfo);
#endif
}

void UHoverDroneMovementComponent::OnUnregister()
{
	Super::OnUnregister();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	AHUD::OnShowDebugInfo.RemoveAll(this);
#endif
}

void UHoverDroneMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	Init();
}

void UHoverDroneMovementComponent::OnTeleported()
{
	Super::OnTeleported();

	Init();
}

void UHoverDroneMovementComponent::ForceFacing(FVector Location)
{
	bForceFacingLocation = true;
	bForceFacingFollowedPlayer = false;
	ForcedFacingLocation = Location;
	ForceFacingInterpInPct = 0.f;
}

void UHoverDroneMovementComponent::ForceFacingFollowedPlayer()
{
	bForceFacingLocation = false;
	bForceFacingFollowedPlayer = true;
}

void UHoverDroneMovementComponent::StopForceFacing()
{
	bForceFacingLocation = false;
	bForceFacingFollowedPlayer = false;
}

void UHoverDroneMovementComponent::TetherToFollowedPlayer()
{
	bTetherToFollowedPlayer = true;
}

void UHoverDroneMovementComponent::StopTether()
{
	bTetherToFollowedPlayer = false;
}


void UHoverDroneMovementComponent::SetMaintainHoverHeight(bool bShouldMaintainHeight)
{
	if (bShouldMaintainHeight != bMaintainHoverHeight)
	{
		bMaintainHoverHeight = bShouldMaintainHeight;
		DesiredHoverHeight = -1.f;		// signal this needs to be reestablished
	}
}
bool UHoverDroneMovementComponent::GetMaintainHoverHeight() const
{
	return bMaintainHoverHeight;
}

void UHoverDroneMovementComponent::ResetDesiredAltitude()
{
	DesiredHoverHeight = -1.f;
}

void UHoverDroneMovementComponent::Init()
{
	if (PawnOwner)
	{
		CurrentAltitude = MeasureAltitude(PawnOwner->GetActorLocation());
		DirectRotationInputGoalRotation = UpdatedComponent->GetComponentRotation();
		MaxAllowedSpeedIndex = FMath::Clamp<>(MaxAllowedSpeedIndex, 0, DroneSpeedParameters.Num() - 1);
		DroneSpeedParamIndex = FMath::Clamp<>(DroneSpeedParamIndex, 0, DroneSpeedParameters.Num() - 1);
	}
}

void UHoverDroneMovementComponent::SetDroneSpeedIndex(int32 SpeedIndex)
{
	if (DroneSpeedParameters.IsValidIndex(SpeedIndex))
	{
		if (SpeedIndex > MaxAllowedSpeedIndex)
		{
			SpeedIndex = MaxAllowedSpeedIndex;
		}
		DroneSpeedParamIndex = SpeedIndex;
	}
}

void UHoverDroneMovementComponent::UpdatedMaxAllowedSpeed(int32 NewMaxAllowedSpeed)
{
	if (!DroneSpeedParameters.IsValidIndex(NewMaxAllowedSpeed))
	{
		return;
	}

	if (NewMaxAllowedSpeed != MaxAllowedSpeedIndex)
	{
		MaxAllowedSpeedIndex = NewMaxAllowedSpeed;
		SetDroneSpeedIndex(MaxAllowedSpeedIndex);		
		MaxAllowedSpeedUpdated.Broadcast();
	}
}

//#include "DrawDebugHelpers.h"
float UHoverDroneMovementComponent::MeasureAltitude(FVector Location) const
{
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(Reverb_HoverDrone_MeasureAltitude), true, PawnOwner);
	FHitResult Hit;

	FVector const TraceStart = Location;
	FVector const TraceEnd = TraceStart - FVector::UpVector * 100000.f;
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams);
	if (bHit)
	{
//		DrawDebugSphere(PawnOwner->GetWorld(), Hit.ImpactPoint, 30.f, 10, FColor::Yellow, false, 0.5f);
		return (Hit.ImpactPoint - TraceStart).Size();
	}

	return 0.f;
}

float UHoverDroneMovementComponent::GetInputFOVScale() const
{
	if (bUseFOVScaling)
	{
		return FMath::GetMappedRangeValueClamped(CameraFovRange, InputFovScaleRange, CurrentFOV);
	}
	
	return 1.f;
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

// Debug
void UHoverDroneMovementComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	const UFont* RenderFont = GEngine->GetMediumFont();
	Canvas->SetDrawColor(FColor::White);

	// This is likely an unrealistic amount of info to actually be that useful.
	// But with things laid out the way they are it should be simple enough to
	// add code that can toggle certain things on / off as problems arise.
	
	TStringBuilder<1024> DebugString;

	if (EnumHasAnyFlags(DebugFlags, EHoverDroneDebug::Position))
	{
		DebugString.Reset();

		DebugString.Appendf(TEXT("MinPitch: %f\n"), MinPitch);
		DebugString.Appendf(TEXT("MaxPitch: %f\n"), MaxPitch);

		YPos += 16;
		YPos += Canvas->DrawText(RenderFont, DebugString.ToString(), 4, YPos);
	}

	if (EnumHasAnyFlags(DebugFlags, EHoverDroneDebug::Velocity))
	{
		DebugString.Reset();

		const FVector InputVector = GetLastInputVector();
		DebugString.Appendf(TEXT("Turbo: %d\n"), !!bTurbo);
		DebugString.Appendf(TEXT("FullAirFrictionVelocity: %f\n"), FullAirFrictionVelocity);
		DebugString.Appendf(TEXT("MinAirFriction: %f\n"), MinAirFriction);
		DebugString.Appendf(TEXT("MaxAllowedSpeedIndex: %d\n"), MaxAllowedSpeedIndex);
		DebugString.Appendf(TEXT("DroneSpeedParamIndex: %d\n"), DroneSpeedParamIndex);
		DebugString.Appendf(TEXT("MinSpeedHeight: %f\n"), MinSpeedHeight);
		DebugString.Appendf(TEXT("MaxSpeedHeight: %f\n"), MaxSpeedHeight);
		DebugString.Appendf(TEXT("MaxSpeedHeightMultiplier: %f\n"), MaxSpeedHeightMultiplier);
		DebugString.Appendf(TEXT("DroneSpeedScalar: %f\n"), DroneSpeedScalar);
		
		DebugString.Appendf(TEXT("MovementInput: X=%.2f Y=%.2f Z=%.2f\n"), InputVector.X, InputVector.Y, InputVector.Z);
		DebugString.Appendf(TEXT("MeasuredVelocity: X=%.2f Y=%.2f Z=%.2f\n"), MeasuredVelocity.X, MeasuredVelocity.Y, MeasuredVelocity.Z);
		DebugString.Appendf(TEXT("MovementAccelFactor: X=%.2f Y=%.2f Z=%.2f"), MovementAccelFactor.X, MovementAccelFactor.Y, MovementAccelFactor.Z);

		YPos += 16;
		YPos += Canvas->DrawText(RenderFont, DebugString.ToString(), 4, YPos);
	}

	if (EnumHasAnyFlags(DebugFlags, EHoverDroneDebug::RotationalVelocity))
	{
		DebugString.Reset();

		DebugString.Appendf(TEXT("bSimulateRotation: %d\n"), !!bSimulateRotation);
		DebugString.Appendf(TEXT("DirectRotationInput: P=%.2f Y=%.2f R=%.2f\n"), LastDirectRotationInput.Pitch, LastDirectRotationInput.Yaw, LastDirectRotationInput.Roll);
		DebugString.Appendf(TEXT("DirectRotationInputYawScale: %f\n"), DirectRotationInputYawScale);
		DebugString.Appendf(TEXT("DirectRotationInputPitchScale: %f\n"), DirectRotationInputPitchScale);
		DebugString.Appendf(TEXT("DirectRotationInputInterpSpeed: %f\n"), DirectRotationInputInterpSpeed);
		DebugString.Appendf(TEXT("DirectRotationInputGoalRotation: P=%.2f Y=%.2f R=%.2f\n"), DirectRotationInputGoalRotation.Pitch, DirectRotationInputGoalRotation.Yaw, DirectRotationInputGoalRotation.Roll);

		DebugString.Appendf(TEXT("RotationInput: P=%.2f Y=%.2f R=%.2f\n"), LastRotationInput.Pitch, LastRotationInput.Yaw, LastRotationInput.Roll);

		DebugString.Appendf(TEXT("RotAcceleration: %f\n"), RotAcceleration);
		DebugString.Appendf(TEXT("RotDeceleration: %f\n"), RotDeceleration);
		DebugString.Appendf(TEXT("MaxPitchRotSpeed: %f\n"), MaxPitchRotSpeed);
		DebugString.Appendf(TEXT("MaxYawRotSpeed: %f\n"), MaxYawRotSpeed);
		DebugString.Appendf(TEXT("RotVelocity: P=%.2f Y=%.2f R=%.2f\n"), RotVelocity.Pitch, RotVelocity.Yaw, RotVelocity.Roll);

		YPos += 16;
		YPos += Canvas->DrawText(RenderFont, DebugString.ToString(), 4, YPos);
	}

	if (EnumHasAnyFlags(DebugFlags, EHoverDroneDebug::Altitude))
	{
		DebugString.Reset();
		
		DebugString.Appendf(TEXT("DroneMaxAltitude: %f\n"), DroneMaxAltitude);
		DebugString.Appendf(TEXT("MinHoverHeight: %f\n"), MinHoverHeight);
		DebugString.Appendf(TEXT("bMaintainHoverHeight: %d\n"), !!bMaintainHoverHeight);
		DebugString.Appendf(TEXT("MaintainHoverHeightTolerance: %f\n"), MaintainHoverHeightTolerance);
		DebugString.Appendf(TEXT("MaintainHoverHeightPredictionTime: %f\n"), MaintainHoverHeightPredictionTime);
		DebugString.Appendf(TEXT("DesiredHoverHeight: %f\n"), DesiredHoverHeight);
		DebugString.Appendf(TEXT("CurrentAltitude: %f\n"), CurrentAltitude);

		YPos += 16;
		YPos += Canvas->DrawText(RenderFont, DebugString.ToString(), 4, YPos);
	}

	if (EnumHasAnyFlags(DebugFlags, EHoverDroneDebug::ForceFacing))
	{
		DebugString.Reset();

		DebugString.Appendf(TEXT("bForceFacingLocation: %d\n"), !!bForceFacingLocation);
		DebugString.Appendf(TEXT("bForceFacingFollowedPlayer: %d\n"), !!bForceFacingFollowedPlayer);
		DebugString.Appendf(TEXT("bTetherToFollowedPlayer: %d\n"), !!bTetherToFollowedPlayer);
		DebugString.Appendf(TEXT("ForceFacingInterpInPct: %f\n"), ForceFacingInterpInPct);
		DebugString.Appendf(TEXT("FollowedActorPositionInterpSpeed: %f\n"), FollowedActorPositionInterpSpeed);
		DebugString.Appendf(TEXT("ForceFacingInterpInTime: %f\n"), ForceFacingInterpInTime);
		DebugString.Appendf(TEXT("ForcedFacingLocation: X=%.2f Y=%.2f Z=%.2f"), ForcedFacingLocation.X, ForcedFacingLocation.Y, ForcedFacingLocation.Z);
		DebugString.Appendf(TEXT("LastFollowedPlayerFacingLoc: X=%.2f Y=%.2f Z=%.2f"), LastFollowedPlayerFacingLoc.X, LastFollowedPlayerFacingLoc.Y, LastFollowedPlayerFacingLoc.Z);
		DebugString.Appendf(TEXT("FollowedActorLastPosition: X=%.2f Y=%.2f Z=%.2f"), FollowedActorLastPosition.X, FollowedActorLastPosition.Y, FollowedActorLastPosition.Z);
		DebugString.Appendf(TEXT("FollowedActorSmoothedPosition: X=%.2f Y=%.2f Z=%.2f"), FollowedActorSmoothedPosition.X, FollowedActorSmoothedPosition.Y, FollowedActorSmoothedPosition.Z);
		DebugString.Appendf(TEXT("ForceFacingPlayerLocalOffset: X=%.2f Y=%.2f Z=%.2f"), ForceFacingPlayerLocalOffset.X, ForceFacingPlayerLocalOffset.Y, ForceFacingPlayerLocalOffset.Z);
	
		YPos += 16;
		YPos += Canvas->DrawText(RenderFont, DebugString.ToString(), 4, YPos);
	}

	if (EnumHasAnyFlags(DebugFlags, EHoverDroneDebug::FOV))
	{
		DebugString.Reset();
	
		DebugString.Appendf(TEXT("bUseFOVScaling: %d\n"), !!bUseFOVScaling);
		DebugString.Appendf(TEXT("CameraFOVRange: X=%.2f Y=%.2f"), CameraFovRange.X, CameraFovRange.Y);
		DebugString.Appendf(TEXT("InputFovScaleRange: X=%.2f Y=%.2f"), InputFovScaleRange.X, InputFovScaleRange.Y);

		YPos += 16;
		YPos += Canvas->DrawText(RenderFont, DebugString.ToString(), 4, YPos);
	}
}

void UHoverDroneMovementComponent::ShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
	static FName NAME_HoverDrone = FName(TEXT("HoverDrone"));

	if (Canvas && HUD->ShouldDisplayDebug(NAME_HoverDrone))
	{
		if (const APawn* Pawn = GetOwner<APawn>())
		{
			if (const AController* Controller = Pawn->GetController())
			{
				if (Controller->IsLocalPlayerController())
				{
					DrawDebug(Canvas, YL, YPos);
				}
			}
		}
	}
}

#endif

#undef LOCTEXT_NAMESPACE
