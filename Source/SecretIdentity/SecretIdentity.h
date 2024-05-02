// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EPlayerControlState : uint8
{
	Default = 0,
	Sprinting,
	Punching,
	TravelPower_Flight_Strafe,
	TravelPower_Flight_Forward,

	Count UMETA(Hidden)
};

DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerControlStateDelegate, EPlayerControlState);

namespace CollisionChannels
{
	constexpr ECollisionChannel Combat = ECollisionChannel::ECC_EngineTraceChannel1;
}

namespace CollisionProfiles
{
	constexpr auto NoCollision				= TEXT("NoCollision");
	constexpr auto BlockAll					= TEXT("BlockAll");
	constexpr auto OverlapAll				= TEXT("OverlapAll");
	constexpr auto BlockAllDynamic			= TEXT("OverlapAll");
	constexpr auto OverlapAllDynamic		= TEXT("OverlapAll");
	constexpr auto Ragdoll					= TEXT("Ragdoll");

	constexpr auto CombatOnlyPhysicsBody	= TEXT("CombatOnlyPhysicsBody");
	constexpr auto CombatCollider			= TEXT("CombatCollider");
}