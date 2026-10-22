// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateMachine.h"
#include "State/Attack/AttackBaseState.h"
#include "AttackStateMachine.generated.h"

class UAttackIdleState;
class UAttackActionState;
class USkeletalMeshComponent;
struct FAnimNotifyEventReference;

/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent),Blueprintable)
class COMBOSYSTEMPLUGIN_API UAttackStateMachine : public UStateMachine
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,Instanced, Category = "Combo StateMachine Attributes") TObjectPtr<UAttackIdleState> DefaultState;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	int32 InputButton;
	int32 InputState;
	
	int32 ComboProgressType;

	uint32 bHasAnimationStopped : 1;
	uint32 bShouldTransitionToNextMove : 1;

	UPROPERTY() float CurrentDamage;

public:
	UAttackStateMachine();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // One latest command, not a queue. Expiry is independent of physical held buttons.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack Input", meta = (ClampMin = "0.01", Units = "s"))
    float InputBufferDuration = 2.f;

    bool PeekBufferedAttackInput(EInputButtons& Button, EButtonState& State) const;
    bool TryConsumeAttackInput(EInputButtons& Button, EButtonState& State);

    UFUNCTION(BlueprintCallable, Category = "Attack Input")
    void ClearBufferedAttackInput();

    UFUNCTION(BlueprintCallable, Category = "Attack Input")
    void AttackPressed(EInputButtons Button);

    UFUNCTION(BlueprintCallable, Category = "Attack Input")
    void AttackReleased(EInputButtons Button);

    UFUNCTION(BlueprintPure, Category = "Attack Input")
    bool IsAttackButtonDown(EInputButtons Button) const;

    // All animation events are validated and routed to the current action here.
    void HandleChargeTierNotify(USkeletalMeshComponent* Mesh,
        const FAnimNotifyEventReference& Reference, int32 Tier);
    void HandleChargeReleaseNotify(USkeletalMeshComponent* Mesh,
        const FAnimNotifyEventReference& Reference);
    void HandleAttackFinishedNotify(USkeletalMeshComponent* Mesh,
        const FAnimNotifyEventReference& Reference);
    void HandleComboWindowBeginNotify(USkeletalMeshComponent* Mesh,
        const FAnimNotifyEventReference& Reference, EMoveProgressType Progress);
    void HandleComboWindowEndNotify(USkeletalMeshComponent* Mesh,
        const FAnimNotifyEventReference& Reference);
    bool IsCurrentState(const UState* State) const { return CurrentState == State; }
    USkeletalMeshComponent* GetAttackMesh() const { return SkeletalMesh; }


	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") void SetInputButton(int32 Value);
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") void SetInputState(int32 Value);
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") FORCEINLINE void SetComboProgressType(int32 Value) { ComboProgressType = Value; }

	int32 GetInputButton() const;
	int32 GetInputState() const;
	FORCEINLINE int32 GetComboProgressType() const { return ComboProgressType; }

	FORCEINLINE bool HasAnimationStopped() const { return bHasAnimationStopped; }
	FORCEINLINE bool ShouldTransitionToNextMove() const { return bShouldTransitionToNextMove; }

	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") void SetShouldTransitionToNextMove(bool Value) { bShouldTransitionToNextMove = Value; }
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") void SetHasAnimationStopped(bool Value) { bHasAnimationStopped = Value; }

	FORCEINLINE TObjectPtr<UAttackIdleState> GetDefaultState() const { return DefaultState; }
	FORCEINLINE float GetCurrentStateDamage() const { return CurrentDamage; }
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions")  void SetCurrentStateDamage(float NewDamage) { CurrentDamage = NewDamage; }

	void ResetFlags();
	
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") bool IsInAttackState();
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") bool IsInIdleState();
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") bool HasCompletedCombo();
	UFUNCTION(BlueprintCallable, Category = "Combo StateMachine Functions") bool IsKeyOnHold();
	
	void PlayMontage(UAnimMontage* Montage,float InPlayRate = 1.0f);
	void SetPlayRateForCurrentMontage(float InPlayRate);
	
private:
	UPROPERTY()
	USkeletalMeshComponent* SkeletalMesh;
	UPROPERTY()
	bool bIsChargedAttackLooping;

    UAttackActionState* GetCurrentAttackAction() const;
    UAttackActionState* ResolveNotifyAction(USkeletalMeshComponent* Mesh,
        const FAnimNotifyEventReference& Reference) const;

    double GetInputTimeSeconds() const;
    double BufferedInputTime = -1.0;

    // Physical input survives state transitions; InputButton/InputState are a single buffered command.
    TSet<EInputButtons> HeldAttackButtons;

#if WITH_DEV_AUTOMATION_TESTS
    friend struct FAttackChargeTestAccess;
#endif
};
