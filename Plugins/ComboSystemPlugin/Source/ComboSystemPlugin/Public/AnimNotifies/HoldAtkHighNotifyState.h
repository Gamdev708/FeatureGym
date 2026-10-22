#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "HoldAtkHighNotifyState.generated.h"

// Retain the original reflected class name for existing montage references.
UCLASS(meta = (DisplayName = "Attack Charge Window"))
class COMBOSYSTEMPLUGIN_API UHoldAtkHighNotifyState : public UAnimNotifyState
{
    GENERATED_BODY()
    
public:
    UHoldAtkHighNotifyState();
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyTick(FBranchingPointNotifyPayload& Payload, float FrameDeltaTime) override;
    virtual FString GetNotifyName_Implementation() const override;
};
