// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "CombatColliderComponent.h"

#include "SecretIdentity/SecretIdentity.h"

UCombatColliderComponent::UCombatColliderComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UCombatColliderComponent::Enable()
{
	SetCollisionProfileName(CollisionProfiles::OverlapAllDynamic);
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void UCombatColliderComponent::Disable()
{
	SetCollisionProfileName(CollisionProfiles::NoCollision);
}