// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "HoverDronePawnBase.h"
#include "HoverDronePawn.generated.h"

class UInputMappingContext;

HOVERDRONE_API extern float DroneSpeedScalar;

UCLASS()
class HOVERDRONE_API AHoverDronePawn : public AHoverDronePawnBase
{
	GENERATED_BODY()

public:
	/** ctor */
	AHoverDronePawn(const FObjectInitializer& ObjectInitializer);

	// APawn interface
	virtual FRotator GetViewRotation() const override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;

	/** Returns drone's current height above the ground. */
	UFUNCTION(BlueprintCallable, Category=HoverDrone)
	float GetAltitude() const;
	
	/** Returns true if this drone has auto-altitude on. */
	UFUNCTION(BlueprintCallable, Category=HoverDrone)
	bool IsMaintainingConstantAltitude() const;

	/** Movement input handlers */
	virtual void MoveForward(float Val) override;
	virtual void MoveRight(float Val) override;
	void MoveUp(float Val);

	/** Turn by accelerating (i.e. drone's thrusters) */
	void TurnAccel(float Val);
	/** Look up/down by accelerating (i.e. drone's thrusters) */
	void LookUpAccel(float Val);

	UFUNCTION(BlueprintCallable, Category = HoverDrone)
	int32 GetDroneSpeedIndex() const;
	void SetDroneSpeedIndex(int32 SpeedIndex);

	void ResetInterpolation();

protected:
	/** When true, speed can be changed by calls to IncreaseHoverDroneSpeed and DecreaseHoverDroneSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Drone Pawn|Input")
	bool bAllowSpeedChange;

private:

	// override to ignore base class bindings
	virtual void MoveUp_World(float Val) override {};

	/** Input handler for lookat functionality */
	void BeginLookat();
	void EndLookat();

	/** Input handler for turbo auto-altitude */
	void ToggleFixedHeight();

	/** Returns drone's current height above the ground. */
	UFUNCTION(BlueprintCallable, Category = HoverDrone)
	void SetAllowSpeedChange(bool bOnOff) { bAllowSpeedChange = bOnOff; };

	/** For interpolating the tilt. */
	FRotator LastTiltedDroneRot;

	/** Input handler for speed adjusting */
	void IncreaseHoverDroneSpeed();
	void DecreaseHoverDroneSpeed();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Hover Drone Pawn|Input")
	class UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Hover Drone Pawn|Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Hover Drone Pawn|Input")
	class UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Hover Drone Pawn|Input")
	class UInputAction* ChangeAltitudeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Hover Drone Pawn|Input")
	class UInputAction* ChangeSpeedAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hover Drone Pawn|Input")
	int32 InputMappingPriority = 1;

	

	void MoveActionBinding(const struct FInputActionValue& ActionValue);
	void LookActionBinding(const struct FInputActionValue& ActionValue);
	void ChangeAltitudeActionBinding(const struct FInputActionValue& ActionValue);
	void ChangeSpeedActionBinding(const struct FInputActionValue& ActionValue);
};



