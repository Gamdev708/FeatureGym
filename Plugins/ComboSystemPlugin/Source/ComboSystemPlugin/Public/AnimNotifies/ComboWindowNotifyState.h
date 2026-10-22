// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "State/Attack/AttackBaseState.h"
#include "ComboWindowNotifyState.generated.h"


/**
 * 
 */
UCLASS()
class COMBOSYSTEMPLUGIN_API UComboWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
    UComboWindowNotifyState();
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;

protected:	
	UPROPERTY(EditAnywhere , Category = "Combo Window")
	EMoveProgressType ProgressType = EMoveProgressType::Normal;
	
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
