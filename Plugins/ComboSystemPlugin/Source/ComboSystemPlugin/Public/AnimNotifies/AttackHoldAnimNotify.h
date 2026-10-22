#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AttackHoldAnimNotify.generated.h"

UENUM(BlueprintType)
enum class EAttackNotifyEvent : uint8
{
    ChargeTier,
    Finished
};

UCLASS(meta = (DisplayName = "Attack Event"))
class COMBOSYSTEMPLUGIN_API UAttackHoldAnimNotify : public UAnimNotify
{
    GENERATED_BODY()

public:
    UAttackHoldAnimNotify();

    UPROPERTY(EditAnywhere, Category = "Attack")
    EAttackNotifyEvent Event = EAttackNotifyEvent::ChargeTier;

    UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0", EditCondition = "Event == EAttackNotifyEvent::ChargeTier"))
    int32 Tier = 1;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
    virtual FString GetNotifyName_Implementation() const override;
};
