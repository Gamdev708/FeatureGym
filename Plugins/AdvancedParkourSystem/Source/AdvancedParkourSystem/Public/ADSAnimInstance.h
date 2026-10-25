#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ADSAnimInstance.generated.h"

class AAPSCharacter;
class UAPSCharacterMovementComponent;

/** Movement data for Animation Blueprints used by the parkour character. */
UCLASS(Blueprintable)
class ADVANCEDPARKOURSYSTEM_API UADSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|References")
	TObjectPtr<AAPSCharacter> OwningPlayerCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|References")
	TObjectPtr<UAPSCharacterMovementComponent> OwningMovementComponent = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|Locomotion")
	float GroundSpeed = 0.f;

	/** Signed world vertical velocity in cm/s. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|Locomotion")
	float AirSpeed = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|Locomotion")
	bool bShouldMove = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|Locomotion")
	bool bIsFalling = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|Climbing")
	bool bIsClimbing = false;

	/** Surface-relative velocity in cm/s: Z = up/down, Y = right/left. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ADS|Climbing")
	FVector ClimbVelocity = FVector::ZeroVector;

private:
	void CacheOwningCharacter();
	void ResetMovementData();
};
