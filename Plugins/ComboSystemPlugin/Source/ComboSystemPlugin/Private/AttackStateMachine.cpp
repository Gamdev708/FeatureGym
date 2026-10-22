#include "AttackStateMachine.h"

#include "Animation/AnimInstance.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "State/Attack/AttackActionState.h"
#include "State/Attack/AttackIdleState.h"

UAttackStateMachine::UAttackStateMachine()
{
    InputButton = 0;
    InputState = 0;
    ComboProgressType = 0;
    bHasAnimationStopped = false;
    bShouldTransitionToNextMove = false;
    CurrentDamage = 0.f;
    bIsChargedAttackLooping = false;
}

void UAttackStateMachine::BeginPlay()
{
    Super::BeginPlay();
    AActor* Owner = GetOwner();
    SkeletalMesh = Owner ? Owner->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
    if (!ensureMsgf(SkeletalMesh && DefaultState,
        TEXT("AttackStateMachine requires a skeletal mesh and a default idle state.")))
    {
        return;
    }
    DefaultState->Initialize(this);
    SwitchState(DefaultState);
}

void UAttackStateMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CurrentState)
    {
        CurrentState->Exit();
        CurrentState = nullptr;
    }
    HeldAttackButtons.Reset();
    ClearBufferedAttackInput();
    ComboProgressType = 0;
    ResetFlags();
    Super::EndPlay(EndPlayReason);
}

double UAttackStateMachine::GetInputTimeSeconds() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
}

void UAttackStateMachine::ClearBufferedAttackInput()
{
    InputButton = 0;
    InputState = 0;
    BufferedInputTime = -1.0;
}

bool UAttackStateMachine::PeekBufferedAttackInput(EInputButtons& Button, EButtonState& State) const
{
    Button = EInputButtons::None;
    State = EButtonState::None;
    const double Age = GetInputTimeSeconds() - BufferedInputTime;
    if (BufferedInputTime < 0.0 || !FMath::IsFinite(InputBufferDuration) ||
        Age < 0.0 || Age > FMath::Max(0.01f, InputBufferDuration) ||
        InputButton < static_cast<int32>(EInputButtons::LightAttack) ||
        InputButton > static_cast<int32>(EInputButtons::HeavyAttack) ||
        (InputState != static_cast<int32>(EButtonState::Pressed) &&
         InputState != static_cast<int32>(EButtonState::Hold)))
    {
        return false;
    }
    Button = static_cast<EInputButtons>(InputButton);
    State = static_cast<EButtonState>(InputState);
    return true;
}

bool UAttackStateMachine::TryConsumeAttackInput(EInputButtons& Button, EButtonState& State)
{
    const bool bValid = PeekBufferedAttackInput(Button, State);
    ClearBufferedAttackInput();
    return bValid;
}

int32 UAttackStateMachine::GetInputButton() const
{
    EInputButtons Button;
    EButtonState State;
    return PeekBufferedAttackInput(Button, State) ? static_cast<int32>(Button) : 0;
}

int32 UAttackStateMachine::GetInputState() const
{
    EInputButtons Button;
    EButtonState State;
    return PeekBufferedAttackInput(Button, State) ? static_cast<int32>(State) : 0;
}

void UAttackStateMachine::SetInputButton(int32 Value)
{
    if (Value < static_cast<int32>(EInputButtons::LightAttack) ||
        Value > static_cast<int32>(EInputButtons::HeavyAttack))
    {
        ClearBufferedAttackInput();
        return;
    }
    if (InputButton != Value)
    {
        InputButton = Value;
        BufferedInputTime = GetInputTimeSeconds();
    }
}

void UAttackStateMachine::SetInputState(int32 Value)
{
    if (Value != static_cast<int32>(EButtonState::Pressed) &&
        Value != static_cast<int32>(EButtonState::Hold))
    {
        ClearBufferedAttackInput();
        return;
    }
    if (InputState != Value)
    {
        InputState = Value;
        BufferedInputTime = GetInputTimeSeconds();
    }
}

void UAttackStateMachine::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    EInputButtons Button;
    EButtonState State;
    if (BufferedInputTime >= 0.0 && !PeekBufferedAttackInput(Button, State))
    {
        ClearBufferedAttackInput();
    }
    // Expire before the current state can inspect or consume a command.
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAttackStateMachine::AttackPressed(EInputButtons Button)
{
    if (Button != EInputButtons::LightAttack &&
        Button != EInputButtons::MediumAttack &&
        Button != EInputButtons::HeavyAttack)
    {
        return;
    }
    if (HeldAttackButtons.Contains(Button))
    {
        return;
    }
    HeldAttackButtons.Add(Button);
    InputButton = static_cast<int32>(Button);
    InputState = static_cast<int32>(EButtonState::Pressed);
    BufferedInputTime = GetInputTimeSeconds();
}

void UAttackStateMachine::AttackReleased(EInputButtons Button)
{
    HeldAttackButtons.Remove(Button);
    if (UAttackActionState* Action = GetCurrentAttackAction())
    {
        Action->RecordRelease(Button);
    }
}

bool UAttackStateMachine::IsAttackButtonDown(EInputButtons Button) const
{
    return HeldAttackButtons.Contains(Button);
}

UAttackActionState* UAttackStateMachine::GetCurrentAttackAction() const
{
    return Cast<UAttackActionState>(CurrentState);
}

UAttackActionState* UAttackStateMachine::ResolveNotifyAction(USkeletalMeshComponent* Mesh,
    const FAnimNotifyEventReference& Reference) const
{
    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Mesh || Mesh->GetOwner() != Owner)
    {
        return nullptr;
    }
    UAttackActionState* Action = GetCurrentAttackAction();
    const auto* Context = Reference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
    return Action && Context && Action->AcceptsNotify(Mesh, Context->MontageInstanceID)
        ? Action : nullptr;
}

void UAttackStateMachine::HandleChargeTierNotify(USkeletalMeshComponent* Mesh,
    const FAnimNotifyEventReference& Reference, int32 Tier)
{
    if (UAttackActionState* Action = ResolveNotifyAction(Mesh, Reference))
    {
        Action->ReachChargeTier(Tier);
    }
}

void UAttackStateMachine::HandleChargeReleaseNotify(USkeletalMeshComponent* Mesh,
    const FAnimNotifyEventReference& Reference)
{
    if (UAttackActionState* Action = ResolveNotifyAction(Mesh, Reference))
    {
        Action->ProcessChargeRelease();
    }
}

void UAttackStateMachine::HandleAttackFinishedNotify(USkeletalMeshComponent* Mesh,
    const FAnimNotifyEventReference& Reference)
{
    if (UAttackActionState* Action = ResolveNotifyAction(Mesh, Reference))
    {
        Action->FinishAttack();
    }
}

void UAttackStateMachine::HandleComboWindowBeginNotify(USkeletalMeshComponent* Mesh,
    const FAnimNotifyEventReference& Reference, EMoveProgressType Progress)
{
    if (UAttackActionState* Action = ResolveNotifyAction(Mesh, Reference))
    {
        Action->OpenComboWindow(Reference.GetNotify(), Progress);
    }
}

void UAttackStateMachine::HandleComboWindowEndNotify(USkeletalMeshComponent* Mesh,
    const FAnimNotifyEventReference& Reference)
{
    if (UAttackActionState* Action = ResolveNotifyAction(Mesh, Reference))
    {
        Action->CloseComboWindow(Reference.GetNotify());
    }
}

void UAttackStateMachine::ResetFlags()
{
    bHasAnimationStopped = false;
    bShouldTransitionToNextMove = false;
    CurrentDamage = 0.f;
}

bool UAttackStateMachine::IsInAttackState()
{
    return GetCurrentAttackAction() != nullptr;
}

bool UAttackStateMachine::IsInIdleState()
{
    return CurrentState && CurrentState->IsA<UAttackIdleState>();
}

bool UAttackStateMachine::HasCompletedCombo()
{
    UAttackActionState* Action = GetCurrentAttackAction();
    return Action && Action->IsFinal();
}

bool UAttackStateMachine::IsKeyOnHold()
{
    return !HeldAttackButtons.IsEmpty();
}

void UAttackStateMachine::PlayMontage(UAnimMontage* Montage, float InPlayRate)
{
    UAnimInstance* Anim = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
    if (Anim && Montage && InPlayRate > 0.f)
    {
        Anim->Montage_Play(Montage, InPlayRate);
    }
}

void UAttackStateMachine::SetPlayRateForCurrentMontage(float InPlayRate)
{
    UAnimInstance* Anim = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
    if (Anim && InPlayRate > 0.f)
    {
        if (UAnimMontage* ActiveMontage = Anim->GetCurrentActiveMontage())
        {
            Anim->Montage_SetPlayRate(ActiveMontage, InPlayRate);
        }
    }
}
