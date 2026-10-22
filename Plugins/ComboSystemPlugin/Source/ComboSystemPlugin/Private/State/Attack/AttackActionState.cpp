#include "State/Attack/AttackActionState.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AttackStateMachine.h"
#include "State/Attack/AttackIdleState.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarComboDebug(
    TEXT("combo.Debug"), 0, TEXT("Enable attack debug output (0=off, 1=on); also requires Show Attack Debug on the attack asset."));

UAttackActionState::UAttackActionState()
{
    ChargeLoopSectionName = TEXT("FullChargeLoop");
    for (int32 Tier = 0; Tier < 3; ++Tier)
    {
        FAttackChargeRelease& Release = ChargeReleases.AddDefaulted_GetRef();
        Release.DamageMultiplier = 1.f + 0.5f * Tier;
    }
}

void UAttackActionState::DebugAttack(const FString& Message, const FColor& Color) const
{
#if !UE_BUILD_SHIPPING
    const AActor* Owner = AttackStateMachine ? AttackStateMachine->GetOwner() : nullptr;
    const UWorld* World = Owner ? Owner->GetWorld() : nullptr;
    if (!GEngine || !World || !World->IsGameWorld()) { return; }

    // Reuse one row for this actor, including across different attack-state objects.
    const uint64 DebugKey = (uint64(0x4154544B) << 32) | uint64(Owner->GetUniqueID());
    if (!bShowAttackDebug || CVarComboDebug.GetValueOnGameThread() == 0)
    {
        GEngine->RemoveOnScreenDebugMessage(DebugKey);
        return;
    }

    const FString Label = MoveName.IsNone() ? GetClass()->GetName() : MoveName.ToString();
    const EInputButtons Button = DebugInputButton != EInputButtons::None ? DebugInputButton :
        (bChargeEnabled ? ChargeButton : EInputButtons::None);
    const FString InputLabel = StaticEnum<EInputButtons>()->GetNameStringByValue(static_cast<int64>(Button));
    const FString StateLabel = bReleaseRequested ? TEXT("Released") :
        (bCharging ? TEXT("Hold") : StaticEnum<EButtonState>()->GetNameStringByValue(static_cast<int64>(DebugInputState)));
    const float DisplayDamage = bChargeEnabled && ChargeReleases.IsValidIndex(ChargeTier)
        ? Damage * ChargeReleases[ChargeTier].DamageMultiplier : Damage;

    GEngine->AddOnScreenDebugMessage(DebugKey, AttackDebugDuration, Color,
        FString::Printf(TEXT("%s | %s, %s | DMG %.1f | %s"),
            *Label, *InputLabel, *StateLabel, DisplayDamage, *GetNextAttackAvailability()));

    // Detailed event text stays available in the Output Log, never as extra screen rows.
    if (!Message.IsEmpty())
    {
        UE_LOG(LogTemp, Verbose, TEXT("[Attack %s | %s] %s"), *GetNameSafe(Owner), *Label, *Message);
    }
#endif
}

bool UAttackActionState::CanTransitionToNextAttack() const
{
    // Only an actual notify window may unlock chaining; the legacy flag alone cannot.
    return IsActiveAction() && !bCharging && !bIsFinal && !ComboWindows.IsEmpty() &&
        !AttackStateMachine->HasAnimationStopped() && AttackStateMachine->ShouldTransitionToNextMove();
}

FString UAttackActionState::GetNextAttackAvailability() const
{
    if (!CanTransitionToNextAttack())
    {
        return TEXT("Next Unavailable");
    }

    const bool bDelayed = AttackStateMachine->GetComboProgressType() ==
        static_cast<int32>(EMoveProgressType::Delayed);
    TArray<FString> Options;
    const auto AddOption = [this, &Options](EInputButtons Button, const TCHAR* Label,
        TSubclassOf<UAttackActionState> PressPath, TSubclassOf<UAttackActionState> HoldPath)
    {
        if (!RequiredInputs.Contains(Button)) { return; }
        const bool bPressPossible = RequiredStates.Contains(EButtonState::Pressed) && PressPath;
        const bool bHoldPossible = RequiredStates.Contains(EButtonState::Hold) && HoldPath;
        if (bPressPossible)
        {
            Options.Add(Label);
        }
        else if (bHoldPossible)
        {
            Options.Add(FString::Printf(TEXT("%s Hold"), Label));
        }
    };

    // Match the same normal/delayed press-path selection used by Update().
    AddOption(EInputButtons::LightAttack, TEXT("Light"),
        bDelayed ? LightAttackDelayed : LightAttackPress, LightAttackHold);
    AddOption(EInputButtons::MediumAttack, TEXT("Medium"),
        bDelayed ? MediumAttackDelayed : MediumAttackPress, MediumAttackHold);
    AddOption(EInputButtons::HeavyAttack, TEXT("Heavy"),
        bDelayed ? HeavyAttackDelayed : HeavyAttackPress, HeavyAttackHold);

    if (Options.IsEmpty()) { return TEXT("Next Unavailable"); }
    const FString AvailableInputs = FString::Join(Options, TEXT("/"));
    return bDelayed ? FString::Printf(TEXT("Delayed Available (%s)"), *AvailableInputs) :
        FString::Printf(TEXT("%s Available"), *AvailableInputs);
}

bool UAttackActionState::IsActiveAction() const
{
    return bActiveAttack && AttackStateMachine && AttackStateMachine->IsCurrentState(this);
}

bool UAttackActionState::ValidateChargeSettings() const
{
    if (!bChargeEnabled)
    {
        return true;
    }
    if (!Montage || ChargeReleases.IsEmpty() ||
        ChargeButton == EInputButtons::None || ChargeButton == EInputButtons::Invalid ||
        ChargeLoopSectionName.IsNone() || RecoverySectionName.IsNone() ||
        ChargeLoopSectionName == RecoverySectionName ||
        Montage->GetSectionIndex(ChargeLoopSectionName) == INDEX_NONE ||
        Montage->GetSectionIndex(RecoverySectionName) == INDEX_NONE)
    {
        return false;
    }
    for (const FAttackChargeRelease& Release : ChargeReleases)
    {
        if (!FMath::IsFinite(Release.DamageMultiplier) || Release.DamageMultiplier < 0.f)
        {
            return false;
        }
    }
    return true;
}

void UAttackActionState::Enter()
{
    ChargeTier = 0;
    bDebugChargeWindowReported = false;
    bActiveAttack = true;
    bCharging = bChargeEnabled;
    bReleaseCommitted = false;
    bReleaseRequested = bCharging &&
        !AttackStateMachine->IsAttackButtonDown(ChargeButton);
    ComboWindows.Reset();
    AttackStateMachine->ResetFlags();

    Super::Enter();
    if (!IsActiveAction())
    {
        return; // A Blueprint Enter event may have changed the state.
    }

    DebugAttack(FString::Printf(TEXT("Enter | Charging=%s | Release pending=%s"),
        bCharging ? TEXT("Yes") : TEXT("No"), bReleaseRequested ? TEXT("Yes") : TEXT("No")));
    PlayingMesh = AttackStateMachine->GetAttackMesh();
    UAnimInstance* Anim = PlayingMesh.IsValid() ? PlayingMesh->GetAnimInstance() : nullptr;
    if (!Anim || !Montage || !FMath::IsFinite(Speed) || Speed <= 0.f ||
        !ValidateChargeSettings())
    {
        DebugAttack(TEXT("ERROR: Invalid animation instance, montage, speed, or charge sections -> Idle"), FColor::Red);
        UE_LOG(LogTemp, Warning, TEXT("Attack %s has invalid montage, speed, or charge sections."), *GetName());
        Reset();
        return;
    }

    PlayingAnimInstance = Anim;
    if (Anim->Montage_Play(Montage, Speed) <= 0.f)
    {
        DebugAttack(TEXT("ERROR: Montage failed to play -> Idle"), FColor::Red);
        Reset();
        return;
    }
    FAnimMontageInstance* Instance = Anim->GetActiveInstanceForMontage(Montage);
    if (!Instance || !IsActiveAction())
    {
        Reset();
        return;
    }
    MontageInstanceID = Instance->GetInstanceID();
    DebugAttack(FString::Printf(TEXT("Montage started: %s | Speed %.2f"), *GetNameSafe(Montage), Speed), FColor::Green);
    Instance->OnMontageEnded.BindUObject(this, &UAttackActionState::OnAttackMontageEnded, MontageInstanceID);
    Instance->OnMontageBlendingOutStarted.BindUObject(this, &UAttackActionState::OnAttackMontageBlendingOut, MontageInstanceID);

    if (bCharging)
    {
        Anim->Montage_SetNextSection(ChargeLoopSectionName, ChargeLoopSectionName, Montage);
        Anim->Montage_SetNextSection(RecoverySectionName, NAME_None, Montage);
    }
    AttackStateMachine->SetCurrentStateDamage(Damage);
}

void UAttackActionState::Update(float DeltaTime)
{
    if (!IsActiveAction()) { return; }
    Super::Update(DeltaTime);
    if (!IsActiveAction()) { return; }
    DebugAttack(FString()); // Refresh the existing status row, without adding log entries.

    if (AttackStateMachine->HasAnimationStopped())
    {
        DebugAttack(TEXT("Animation-stopped flag -> Idle"), FColor::Yellow);
        Reset();
        return;
    }
    if (!CanTransitionToNextAttack())
    {
        return;
    }

    EInputButtons EnteredInput;
    EButtonState EnteredState;
    if (!AttackStateMachine->PeekBufferedAttackInput(EnteredInput, EnteredState)) { return; }
    EMoveProgressType EnteredProgress = static_cast<EMoveProgressType>(AttackStateMachine->GetComboProgressType());
    if (EnteredInput == EInputButtons::None || EnteredState == EButtonState::None) { return; }
    if (!HandleInput(EnteredInput, EnteredState))
    {
        DebugAttack(TEXT("Discarded combo input that does not meet requirements"), FColor::Yellow);
        AttackStateMachine->ClearBufferedAttackInput();
        return;
    }
	UAttackActionState* NextAction = nullptr;
	switch (EnteredInput)
	{
		
	case EInputButtons::LightAttack:
		switch (EnteredState)
		{
		case EButtonState::Pressed:
			if (EnteredProgress == EMoveProgressType::Delayed)
			{
				if (!ensureAlwaysMsgf(LightAttackDelayed, TEXT("LightAttackDelayed is not configured.")))
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
				SwitchToIdleState();
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
		case EButtonState::Pressed:
			if (EnteredProgress == EMoveProgressType::Delayed)
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
				SwitchToIdleState();
				return;
			}
			NextAction = NewObject<UAttackActionState>(AttackStateMachine, HeavyAttackHold);
			break;
		}
	}
	if (NextAction)
	{
        if (!AttackStateMachine->TryConsumeAttackInput(EnteredInput, EnteredState)) { return; }
        NextAction->CaptureDebugInput(EnteredInput, EnteredState);
		SwitchToNextActionState(NextAction);
		return;
	}
}


void UAttackActionState::RecordRelease(EInputButtons Button)
{
    if (IsActiveAction() && bCharging && Button == ChargeButton)
    {
        if (!bReleaseRequested)
        {
            DebugAttack(FString::Printf(TEXT("Release requested | Tier %d | Waiting for release window"), ChargeTier), FColor::Yellow);
        }
        bReleaseRequested = true;
    }
}

void UAttackActionState::ReachChargeTier(int32 NewTier)
{
    if (!IsActiveAction() || !bCharging || bReleaseCommitted || bReleaseRequested ||
        !AttackStateMachine->IsAttackButtonDown(ChargeButton) || ChargeReleases.IsEmpty())
    {
        return;
    }
    const int32 PreviousTier = ChargeTier;
    ChargeTier = FMath::Max(ChargeTier, FMath::Clamp(NewTier, 0, ChargeReleases.Num() - 1));
    if (ChargeTier != PreviousTier)
    {
        DebugAttack(FString::Printf(TEXT("Charge tier %d -> %d"), PreviousTier, ChargeTier), FColor::Green);
    }
}

void UAttackActionState::ProcessChargeRelease()
{
    if (!IsActiveAction() || !bCharging || bReleaseCommitted) { return; }
    if (!bDebugChargeWindowReported)
    {
        bDebugChargeWindowReported = true;
        DebugAttack(TEXT("Charge release window reached"));
    }
    bReleaseRequested |= !AttackStateMachine->IsAttackButtonDown(ChargeButton);
    if (!bReleaseRequested) { return; }

    UAnimInstance* Anim = PlayingAnimInstance.Get();
    FAnimMontageInstance* Instance = Anim ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
    if (!Instance || Instance->GetInstanceID() != MontageInstanceID ||
        !ChargeReleases.IsValidIndex(ChargeTier))
    {
        DebugAttack(TEXT("ERROR: Charge release has invalid playback or tier -> Idle"), FColor::Red);
        Reset();
        return;
    }

    const FAttackChargeRelease& Release = ChargeReleases[ChargeTier];
    bReleaseCommitted = true;
    bCharging = false;
    AttackStateMachine->SetCurrentStateDamage(Damage * Release.DamageMultiplier);
    DebugAttack(FString::Printf(TEXT("Release committed | Tier %d | Section %s | Damage %.2f"),
        ChargeTier, *RecoverySectionName.ToString(), Damage * Release.DamageMultiplier), FColor::Green);
    Anim->Montage_JumpToSection(RecoverySectionName, Montage);
}

bool UAttackActionState::AcceptsNotify(const USkeletalMeshComponent* Mesh, int32 InstanceID) const
{
    return IsActiveAction() && Mesh == PlayingMesh.Get() &&
        InstanceID != INDEX_NONE && InstanceID == MontageInstanceID;
}

void UAttackActionState::OpenComboWindow(const FAnimNotifyEvent* Window, EMoveProgressType Progress)
{
    if (!IsActiveAction() || bCharging || !Window) { return; }
    ComboWindows.RemoveAll([Window](const auto& Entry) { return Entry.Key == Window; });
    ComboWindows.Emplace(Window, Progress);
    ApplyComboWindow();
    DebugAttack(FString::Printf(TEXT("Combo window opened | %s | Active windows %d"),
        *UEnum::GetValueAsString(Progress), ComboWindows.Num()));
}

void UAttackActionState::CloseComboWindow(const FAnimNotifyEvent* Window)
{
    if (!IsActiveAction() || !Window) { return; }
    const int32 Removed = ComboWindows.RemoveAll([Window](const auto& Entry) { return Entry.Key == Window; });
    ApplyComboWindow();
    if (Removed > 0)
    {
        DebugAttack(FString::Printf(TEXT("Combo window closed | Active windows %d"), ComboWindows.Num()));
    }
}

void UAttackActionState::ApplyComboWindow()
{
    AttackStateMachine->SetComboProgressType(ComboWindows.IsEmpty() ? 0 :
        static_cast<int32>(ComboWindows.Last().Value));
    AttackStateMachine->SetShouldTransitionToNextMove(!bCharging && !ComboWindows.IsEmpty());
}

void UAttackActionState::FinishAttack()
{
    if (IsActiveAction() && !bCharging)
    {
        DebugAttack(TEXT("Finished notify -> Idle"), FColor::Green);
        Reset();
    }
}

void UAttackActionState::OnAttackMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceID)
{
    if (IsActiveAction() && EndedMontage == Montage && InstanceID == MontageInstanceID)
    {
        DebugAttack(bInterrupted ? TEXT("Montage ended: interrupted -> Idle") : TEXT("Montage ended normally -> Idle"),
            bInterrupted ? FColor::Yellow : FColor::Green);
        Reset();
    }
}

void UAttackActionState::OnAttackMontageBlendingOut(UAnimMontage* EndedMontage, bool bInterrupted, int32 InstanceID)
{
    if (bInterrupted && IsActiveAction() && EndedMontage == Montage && InstanceID == MontageInstanceID)
    {
        DebugAttack(TEXT("Montage interrupted during blend-out -> Idle"), FColor::Yellow);
        Reset();
    }
}

void UAttackActionState::Exit()
{
    bActiveAttack = false;
    DebugAttack(TEXT("Exit | Clearing charge and combo data"));
    bDebugChargeWindowReported = false;
    bCharging = bReleaseRequested = bReleaseCommitted = false;
    ChargeTier = 0;
    ComboWindows.Reset();

    UAnimInstance* Anim = PlayingAnimInstance.Get();
    const int32 OldInstanceID = MontageInstanceID;
    MontageInstanceID = INDEX_NONE;
    PlayingAnimInstance.Reset();
    PlayingMesh.Reset();
    if (Anim)
    {
        if (FAnimMontageInstance* Instance = Anim->GetMontageInstanceForID(OldInstanceID))
        {
            if (Instance->OnMontageEnded.IsBoundToObject(this)) { Instance->OnMontageEnded.Unbind(); }
            if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(this)) { Instance->OnMontageBlendingOutStarted.Unbind(); }
            // Never stop another playback of the same montage asset.
            if (Anim->GetActiveInstanceForMontage(Montage) == Instance)
            {
                Anim->Montage_Stop(0.1f, Montage);
            }
        }
    }

    AttackStateMachine->ClearBufferedAttackInput();
    AttackStateMachine->SetComboProgressType(0);
    AttackStateMachine->ResetFlags();
    Super::Exit();
}

void UAttackActionState::SwitchToNextActionState(UAttackActionState* NextAction) const
{
    if (NextAction && IsActiveAction())
    {
        DebugAttack(FString::Printf(TEXT("Combo transition -> %s"), *NextAction->GetClass()->GetName()), FColor::Green);
        NextAction->Initialize(AttackStateMachine);
        AttackStateMachine->SwitchState(NextAction);
    }
}

void UAttackActionState::SwitchToIdleState() const
{
    if (!IsActiveAction()) { return; }
    UAttackIdleState* IdleState = AttackStateMachine->GetDefaultState();
    if (IdleState) { IdleState->Initialize(AttackStateMachine); }
    AttackStateMachine->SwitchState(IdleState);
}

void UAttackActionState::Reset()
{
    SwitchToIdleState();
}
