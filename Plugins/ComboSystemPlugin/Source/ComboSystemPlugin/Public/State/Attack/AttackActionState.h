// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "State/Attack/AttackBaseState.h"
#include "AttackActionState.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;
struct FAnimNotifyEvent;

USTRUCT(BlueprintType)
struct COMBOSYSTEMPLUGIN_API FAttackChargeRelease
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charge", meta = (ClampMin = "0.0"))
    float DamageMultiplier = 1.f;
};

/**
 * 
 */
UCLASS(Blueprintable,BlueprintType,DefaultToInstanced,EditInlineNew)
class COMBOSYSTEMPLUGIN_API UAttackActionState : public UAttackBaseState
{
	GENERATED_BODY()

public:
	UAttackActionState();
    virtual void Enter() override;
	UFUNCTION(BlueprintCallable, Category = "Combo State Functions") void Reset();
	virtual void Update(float DeltaTime) override;
	virtual void Exit() override;

    // Capture the command before the previous state consumes its input buffer.
    void CaptureDebugInput(EInputButtons Button, EButtonState State)
    {
        DebugInputButton = Button;
        DebugInputState = State;
    }

    void RecordRelease(EInputButtons Button);
    void ProcessChargeRelease();
    void ReachChargeTier(int32 NewTier);
    void FinishAttack();
    void OpenComboWindow(const FAnimNotifyEvent* Window, EMoveProgressType Progress);
    void CloseComboWindow(const FAnimNotifyEvent* Window);
    bool AcceptsNotify(const USkeletalMeshComponent* Mesh, int32 InstanceID) const;

    UFUNCTION(BlueprintPure, Category = "Charge")
    int32 GetChargeTier() const { return ChargeTier; }

    UFUNCTION(BlueprintPure, Category = "Charge")
    bool IsCharging() const { return bCharging; }


	/*FORCEINLINE EInputButtons GetInputButton() { return RequiredInput; }
	FORCEINLINE EButtonState GetButtonState() { return Buttonstate; }*/
	// UFUNCTION(BlueprintCallable,Category="Action State Settings")  void AddDuration(float time) { WaitDuration += time; }
	UFUNCTION(BlueprintCallable,Category="Action State Settings") FORCEINLINE bool IsFinal() { return bIsFinal; }
	UFUNCTION(BlueprintCallable,Category="Action State Settings") void SetIsFinal(bool Value) { bIsFinal = Value; }
private:
	void SwitchToIdleState() const;
	void SwitchToNextActionState(UAttackActionState* NextAction) const;

    void DebugAttack(const FString& Message, const FColor& Color = FColor::Cyan) const;
    bool bDebugChargeWindowReported = false;
    EInputButtons DebugInputButton = EInputButtons::None;
    EButtonState DebugInputState = EButtonState::None;

    bool CanTransitionToNextAttack() const;
    FString GetNextAttackAvailability() const;
    bool IsActiveAction() const;
    bool ValidateChargeSettings() const;
    void ApplyComboWindow();
    void OnAttackMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceID);
    void OnAttackMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceID);

    TWeakObjectPtr<UAnimInstance> PlayingAnimInstance;
    TWeakObjectPtr<USkeletalMeshComponent> PlayingMesh;
    int32 MontageInstanceID = INDEX_NONE;
    int32 ChargeTier = 0;
    bool bActiveAttack = false;
    bool bCharging = false;
    bool bReleaseRequested = false;
    bool bReleaseCommitted = false;
    TArray<TPair<const FAnimNotifyEvent*, EMoveProgressType>> ComboWindows;

#if WITH_DEV_AUTOMATION_TESTS
    friend struct FAttackChargeTestAccess;
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo Settings") FName MoveName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug")
    bool bShowAttackDebug = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "0.1", EditCondition = "bShowAttackDebug"))
    float AttackDebugDuration = 4.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge")
    bool bChargeEnabled = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge", meta = (EditCondition = "bChargeEnabled"))
    EInputButtons ChargeButton = EInputButtons::HeavyAttack;

    // Tier zero is ordinary damage. Notifies select the tier; all tiers release into Recovery.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge", meta = (EditCondition = "bChargeEnabled"))
    TArray<FAttackChargeRelease> ChargeReleases; // Preserve the property name for existing Blueprint damage values.

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Charge", meta = (EditCondition = "bChargeEnabled"))
    FName RecoverySectionName = TEXT("Recovery");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Light", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> LightAttackPress;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Light", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> LightAttackHold;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Light", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> LightAttackDelayed;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Medium", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> MediumAttackPress;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Medium", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> MediumAttackHold;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Medium", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> MediumAttackDelayed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Heavy", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> HeavyAttackPress;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Heavy", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> HeavyAttackHold;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Path|Heavy", meta = (EditCondition = "!bIsFinal", EditConditionHides)) TSubclassOf<UAttackActionState> HeavyAttackDelayed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo Settings") bool bIsFinal;
	/* The duration to wait before executing the next move. This property is always editable */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo Settings", meta = (EditCondition = "bWaitforInput", EditConditionHides)) float WaitDuration;

};
