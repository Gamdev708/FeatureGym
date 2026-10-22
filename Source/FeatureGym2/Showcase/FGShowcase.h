#pragma once

#include "CoreMinimal.h"
#include "APSCharacter.h"
#include "AttackStateMachine.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "FGShowcase.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;

UCLASS(Transient, Blueprintable)
class FEATUREGYM2_API UFGShowcaseCombatAnim : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};

// Integration for the portfolio level; the reusable plugin code remains in Plugins/.
UCLASS()
class FEATUREGYM2_API UFGShowcaseCombo : public UAttackStateMachine
{
    GENERATED_BODY()
public:
    UState* GetActiveState() const { return CurrentState; }
};

UCLASS()
class FEATUREGYM2_API AFGShowcaseTarget : public AActor
{
    GENERATED_BODY()
public:
    AFGShowcaseTarget();
    virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    virtual void Tick(float DeltaSeconds) override;
    void ResetTarget();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Readout;
    float Health = 100.f;
    float LastDamage = 0.f;
private:
    float ResetAt = 0.f;
};

UCLASS()
class FEATUREGYM2_API AFGShowcaseCharacter : public AAPSCharacter
{
    GENERATED_BODY()
public:
    AFGShowcaseCharacter(const FObjectInitializer& ObjectInitializer);
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UFGShowcaseCombo> Combo;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bCombatMode = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Showcase|Climbing") TObjectPtr<UAnimMontage> ClimbUpLoop;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Showcase|Climbing") TObjectPtr<UAnimMontage> ClimbDownLoop;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Showcase|Climbing") TObjectPtr<UAnimMontage> ClimbIdleLoop;
    UFUNCTION(BlueprintCallable, Category="Showcase|Climbing") void Climb();
    UFUNCTION(BlueprintCallable, Category="Showcase|Climbing") void StopClimb();
    FVector Checkpoint = FVector(-7200, 0, 110);
    FString Station = TEXT("ARRIVAL / Choose your demonstration");
    int32 Hits = 0;
    float LastHitDamage = 0.f;
    float Elapsed = 0.f;
    FString GetMovementLabel() const;
private:
    void TraverseMode();
    void CombatMode();
    void SwitchMode(bool bCombat);
    void ResetRun();
    void ReturnToCheckpoint();
    void UpdateClimbingAnimation();
    void Slide();
    void Hop();
    void LightPressed();
    void LightReleased();
    void MediumPressed();
    void MediumReleased();
    void HeavyPressed();
    void HeavyReleased();
    TWeakObjectPtr<UState> HitState;
    float StateAge = 0.f;
    int32 StationIndex = -1;
};

UCLASS()
class FEATUREGYM2_API AFGShowcaseHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};

UCLASS()
class FEATUREGYM2_API AFGShowcaseGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AFGShowcaseGameMode();
};
