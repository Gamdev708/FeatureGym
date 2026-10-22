// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/HoldAtkHighNotifyState.h"

#include "AttackNotifyContext.h"

UHoldAtkHighNotifyState::UHoldAtkHighNotifyState()
{
    bIsNativeBranchingPoint = true;
}

void UHoldAtkHighNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
	{
		Machine->HandleChargeReleaseNotify(MeshComp, EventReference);
	}
}

void UHoldAtkHighNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (UAttackStateMachine* Machine = AttackNotifyContext::FindStateMachine(MeshComp))
	{
		Machine->HandleChargeReleaseNotify(MeshComp, EventReference);
	}
}

void UHoldAtkHighNotifyState::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
	NotifyBegin(Payload.SkelMeshComponent, Payload.SequenceAsset, 0.f, AttackNotifyContext::MakeReference(Payload));
}

void UHoldAtkHighNotifyState::BranchingPointNotifyTick(FBranchingPointNotifyPayload& Payload, float FrameDeltaTime)
{
	NotifyTick(Payload.SkelMeshComponent, Payload.SequenceAsset, FrameDeltaTime, AttackNotifyContext::MakeReference(Payload));
}

FString UHoldAtkHighNotifyState::GetNotifyName_Implementation() const
{
	return Super::GetNotifyName_Implementation();
}
