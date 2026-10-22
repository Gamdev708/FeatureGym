#include "AnimNotifies/AttackChargeWindowNotifyState.h"
#include "AttackNotifyContext.h"

UAttackChargeWindowNotifyState::UAttackChargeWindowNotifyState()
{
    bIsNativeBranchingPoint = true;
}

void UAttackChargeWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
    {
        Machine->HandleChargeReleaseNotify(MeshComp, EventReference);
    }
}

void UAttackChargeWindowNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    // No accumulated time: animation ticks only check a latched release request.
    if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
    {
        Machine->HandleChargeReleaseNotify(MeshComp, EventReference);
    }
}

void UAttackChargeWindowNotifyState::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
    NotifyBegin(Payload.SkelMeshComponent, Payload.SequenceAsset, 0.f, AttackNotifyContext::MakeReference(Payload));
}

void UAttackChargeWindowNotifyState::BranchingPointNotifyTick(FBranchingPointNotifyPayload& Payload, float FrameDeltaTime)
{
    NotifyTick(Payload.SkelMeshComponent, Payload.SequenceAsset, FrameDeltaTime, AttackNotifyContext::MakeReference(Payload));
}

FString UAttackChargeWindowNotifyState::GetNotifyName_Implementation() const
{
    return TEXT("Charge Release Window");
}
