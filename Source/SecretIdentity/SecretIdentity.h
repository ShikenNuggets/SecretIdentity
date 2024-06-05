// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EPlayerControlState : uint8
{
	None = 0, //Used when the player is not being controlleed
	Default,
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
	constexpr auto IgnoreOnlyPawn			= TEXT("IgnoreOnlyPawn");
	constexpr auto OverlapOnlyPawn			= TEXT("OverlapOnlyPawn");
	constexpr auto Pawn						= TEXT("Pawn");
	constexpr auto Spectator				= TEXT("Spectator");
	constexpr auto CharacterMesh			= TEXT("CharacterMesh");
	constexpr auto PhysicsActor				= TEXT("PhysicsActor");
	constexpr auto Destructible				= TEXT("Destructible");
	constexpr auto InvisibleWall			= TEXT("InvisibleWall");
	constexpr auto InvisibleWallDynamic		= TEXT("InvisibleWallDynamic");
	constexpr auto Trigger					= TEXT("Trigger");
	constexpr auto Ragdoll					= TEXT("Ragdoll");
	constexpr auto Vehicle					= TEXT("Vehicle");
	constexpr auto UI						= TEXT("UI");

	//constexpr auto CombatOnlyPhysicsBody	= TEXT("CombatOnlyPhysicsBody");
	//constexpr auto CombatCollider			= TEXT("CombatCollider");
}