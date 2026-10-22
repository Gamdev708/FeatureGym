#pragma once

#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AttackStateMachine.h"
#include "Components/SkeletalMeshComponent.h"

namespace AttackNotifyContext
{
    inline FAnimNotifyEventReference MakeReference(const FBranchingPointNotifyPayload& Payload)
    {
        FAnimNotifyEventReference Reference(Payload.NotifyEvent, Payload.SequenceAsset);
        Reference.AddContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(Payload.MontageInstanceID);
        return Reference;
    }

    // Notify code finds only the component. The machine owns validation and state routing.
    inline UAttackStateMachine* FindStateMachine(USkeletalMeshComponent* Mesh)
    {
        AActor* Owner = Mesh ? Mesh->GetOwner() : nullptr;
        return Owner ? Owner->FindComponentByClass<UAttackStateMachine>() : nullptr;
    }
}
