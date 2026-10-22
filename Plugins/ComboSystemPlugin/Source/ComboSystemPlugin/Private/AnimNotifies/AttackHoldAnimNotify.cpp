#include "AnimNotifies/AttackHoldAnimNotify.h"
#include "AttackNotifyContext.h"

UAttackHoldAnimNotify::UAttackHoldAnimNotify()
{
    bIsNativeBranchingPoint = true;
}

void UAttackHoldAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
    {
        if (Event == EAttackNotifyEvent::ChargeTier) { Machine->HandleChargeTierNotify(MeshComp, EventReference, Tier); }
        else if (Event == EAttackNotifyEvent::Finished) { Machine->HandleAttackFinishedNotify(MeshComp, EventReference); }
    }
}

void UAttackHoldAnimNotify::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    Notify(Payload.SkelMeshComponent, Payload.SequenceAsset, AttackNotifyContext::MakeReference(Payload));
}

FString UAttackHoldAnimNotify::GetNotifyName_Implementation() const
{
    return Event == EAttackNotifyEvent::Finished ? TEXT("Attack Finished") :
        FString::Printf(TEXT("Charge Tier %d"), Tier);
}
