// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "CombatSkeletalMeshComponent.h"

#include "SecretIdentity/SecretIdentity.h"
#include "SecretIdentity/UE_Helpers.h"

UCombatSkeletalMeshComponent::UCombatSkeletalMeshComponent()
{
	SetSimulatePhysics(false);
	SetEnableGravity(true);
	SetCollisionProfileName(CollisionProfiles::Ragdoll);
	SetAllUseCCD(true);
	SetGenerateOverlapEvents(true);
}

void UCombatSkeletalMeshComponent::EnableRagdoll()
{
	SetSimulatePhysics(true);
}