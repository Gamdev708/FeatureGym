#include "AnimNotifies/ComboWindowNotifyState.h"
#include "AttackNotifyContext.h"

UComboWindowNotifyState::UComboWindowNotifyState()
{
    bIsNativeBranchingPoint = true;
}

void UComboWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
    {
        Machine->HandleComboWindowBeginNotify(MeshComp, EventReference, ProgressType);
    }
}

void UComboWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
    {
        Machine->HandleComboWindowEndNotify(MeshComp, EventReference);
    }
}

void UComboWindowNotifyState::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
    NotifyBegin(Payload.SkelMeshComponent, Payload.SequenceAsset, 0.f, AttackNotifyContext::MakeReference(Payload));
}

void UComboWindowNotifyState::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload)
{
    NotifyEnd(Payload.SkelMeshComponent, Payload.SequenceAsset, AttackNotifyContext::MakeReference(Payload));
}
