#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AttackChargeWindowNotifyState.generated.h"

UCLASS(meta = (DisplayName = "Attack Charge Window new"))
class COMBOSYSTEMPLUGIN_API UAttackChargeWindowNotifyState : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAttackChargeWindowNotifyState();
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyTick(FBranchingPointNotifyPayload& Payload, float FrameDeltaTime) override;
    virtual FString GetNotifyName_Implementation() const override;
};
