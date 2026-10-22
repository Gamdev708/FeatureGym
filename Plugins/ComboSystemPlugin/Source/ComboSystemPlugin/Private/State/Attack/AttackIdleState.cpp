// Fill out your copyright notice in the Description page of Project Settings.


#include "State/Attack/AttackIdleState.h"

#include "AttackStateMachine.h"
#include "State/Attack/AttackActionState.h"

void UAttackIdleState::Enter()
{
	Super::Enter();
}

void UAttackIdleState::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
	if (!AttackStateMachine->IsCurrentState(this)) { return; }

    EInputButtons EnteredInput;
    EButtonState EnteredState;
    if (!AttackStateMachine->PeekBufferedAttackInput(EnteredInput, EnteredState)) { return; }
	EMoveProgressType EnteredProgress = static_cast<EMoveProgressType>(AttackStateMachine->GetComboProgressType());
	if (EnteredInput == EInputButtons::None || EnteredState == EButtonState::None) { return; }

	UAttackActionState* NextAction = nullptr;

	if (HandleInput(EnteredInput, EnteredState))
	{
		switch (EnteredInput)
		{
		case EInputButtons::LightAttack:
			switch (EnteredState)
			{
			case EButtonState::Pressed:
				if (EnteredProgress == EMoveProgressType::Delayed)
				{
					if (!ensureAlwaysMsgf(LightAttackDelayed, TEXT("The Next Move is LightAttackTap Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
					{
						return;
					}
					NextAction = NewObject<UAttackActionState>(AttackStateMachine, LightAttackDelayed);					
				}
				else
				{
					if (!ensureAlwaysMsgf(LightAttackPress, TEXT("The Next Move LightAttackPress is Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
					{
						return;
					}
					NextAction = NewObject<UAttackActionState>(AttackStateMachine, LightAttackPress);
				}
				break;
			case EButtonState::Hold:
				if (!ensureAlwaysMsgf(LightAttackHold, TEXT("The Next Move is LightAttackHold Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
				{
					return;
				}
				NextAction = NewObject<UAttackActionState>(AttackStateMachine, LightAttackHold);
				break;
			}
			break;

		case EInputButtons::MediumAttack:
			switch (EnteredState)
			{
			case EButtonState::Pressed:
				if (EnteredProgress == EMoveProgressType::Delayed)
				{
					if (!ensureAlwaysMsgf(MediumAttackDelayed, TEXT("The Next Move is LightAttackTap Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
					{
						return;
					}
					NextAction = NewObject<UAttackActionState>(AttackStateMachine, MediumAttackDelayed);					
				}
				else
				{
					if (!ensureAlwaysMsgf(MediumAttackPress, TEXT("The Next Move MediumAttackPress is Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
					{
						return;
					}
					NextAction = NewObject<UAttackActionState>(AttackStateMachine, MediumAttackPress);
				}
				break;
			case EButtonState::Hold:
				if (!ensureAlwaysMsgf(MediumAttackHold, TEXT("The Next Move MediumAttackHold is Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
				{
					return;
				}
				NextAction = NewObject<UAttackActionState>(AttackStateMachine, MediumAttackHold);
				break;
			}
			break;

		case EInputButtons::HeavyAttack:
			switch (EnteredState)
			{
			case EButtonState::Pressed:if (EnteredProgress == EMoveProgressType::Delayed)
                                       				{
                                       					if (!ensureAlwaysMsgf(HeavyAttackDelayed, TEXT("The Next Move is LightAttackTap Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
                                       					{
                                       						return;
                                       					}
                                       					NextAction = NewObject<UAttackActionState>(AttackStateMachine, HeavyAttackDelayed);					
                                       				}
                                       				else
                                       				{
                                       					if (!ensureAlwaysMsgf(HeavyAttackPress, TEXT("The Next Move HeavyAttackPress is Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
                                       					{
                                       						return;
                                       					}
                                       					NextAction = NewObject<UAttackActionState>(AttackStateMachine, HeavyAttackPress);
                                       				}
				
				break;
			case EButtonState::Hold:
				if (!ensureAlwaysMsgf(HeavyAttackHold, TEXT("The Next Move HeavyAttackHold is Not SET!! Please make sure you have attached the related move to the given input params. AUTO switching to Idle Now!")))
				{
					return;
				}
				NextAction = NewObject<UAttackActionState>(AttackStateMachine, HeavyAttackHold);
				break;
			}
			break;
		}

		if (!NextAction) { return; }
        if (!AttackStateMachine->TryConsumeAttackInput(EnteredInput, EnteredState)) { return; }
        NextAction->CaptureDebugInput(EnteredInput, EnteredState);
		NextAction->Initialize(AttackStateMachine);
		//AttackStateMachine->SetCurrentStateDamage(NextAction->GetDamage());
		AttackStateMachine->SwitchState(NextAction);
	}

}

void UAttackIdleState::Exit()
{
	Super::Exit();
	AttackStateMachine->ClearBufferedAttackInput();
	AttackStateMachine->SetComboProgressType(0);
}
