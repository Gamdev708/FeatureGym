#include "ADSAnimInstance.h"

#include "APSCharacter.h"
#include "APSCharacterMovementComponent.h"

void UADSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ResetMovementData();
	CacheOwningCharacter();
}

void UADSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Preview instances may have no pawn; reacquire references after an owner change.
	if (!IsValid(OwningPlayerCharacter) || !IsValid(OwningMovementComponent)
		|| OwningPlayerCharacter != TryGetPawnOwner())
	{
		CacheOwningCharacter();
	}

	if (!IsValid(OwningPlayerCharacter) || !IsValid(OwningMovementComponent))
	{
		ResetMovementData();
		return;
	}

	const FVector Velocity = OwningMovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	AirSpeed = Velocity.Z;
	bShouldMove = GroundSpeed > 3.f
		&& !OwningMovementComponent->GetCurrentAcceleration().IsNearlyZero();
	bIsFalling = OwningMovementComponent->IsFalling();
	bIsClimbing = OwningMovementComponent->IsClimbing();
	ClimbVelocity = bIsClimbing
		? OwningMovementComponent->GetUnrotatedClimbingVelocity()
		: FVector::ZeroVector;
}

void UADSAnimInstance::CacheOwningCharacter()
{
	OwningPlayerCharacter = Cast<AAPSCharacter>(TryGetPawnOwner());
	OwningMovementComponent = IsValid(OwningPlayerCharacter)
		? Cast<UAPSCharacterMovementComponent>(OwningPlayerCharacter->GetCharacterMovement())
		: nullptr;
}

void UADSAnimInstance::ResetMovementData()
{
	GroundSpeed = 0.f;
	AirSpeed = 0.f;
	bShouldMove = false;
	bIsFalling = false;
	bIsClimbing = false;
	ClimbVelocity = FVector::ZeroVector;
}
