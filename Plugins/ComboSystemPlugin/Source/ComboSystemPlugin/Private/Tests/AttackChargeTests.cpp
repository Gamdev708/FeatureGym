#include "AttackStateMachine.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimTypes.h"
#include "Animation/AnimMontage.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "State/Attack/AttackActionState.h"

// Exercise state decisions without an editor world or animation assets.
struct FAttackChargeTestAccess
{
    static UAttackActionState* Activate(UAttackStateMachine* Machine)
    {
        UAttackActionState* Action = NewObject<UAttackActionState>(Machine);
        Action->Initialize(Machine);
        Machine->CurrentState = Action;
        Action->bActiveAttack = true;
        Action->bChargeEnabled = true;
        Action->bCharging = true;
        return Action;
    }

    static void SetChargeMontage(UAttackActionState* Action, UAnimMontage* Montage)
    {
        Action->bChargeEnabled = true;
        Action->Montage = Montage;
    }

    static bool HasValidChargeSettings(const UAttackActionState* Action)
    {
        return Action->ValidateChargeSettings();
    }

    static TArray<FAttackChargeRelease>& DamageTiers(UAttackActionState* Action)
    {
        return Action->ChargeReleases;
    }

    static void AgeInput(UAttackStateMachine* Machine, double Seconds)
    {
        Machine->BufferedInputTime = Machine->GetInputTimeSeconds() - Seconds;
    }

    static bool CanChain(const UAttackActionState* Action)
    {
        return Action->CanTransitionToNextAttack();
    }

    static bool ReleaseRequested(const UAttackActionState* Action)
    {
        return Action->bReleaseRequested;
    }

    static void MarkReleased(UAttackActionState* Action)
    {
        Action->bCharging = false;
        Action->bReleaseCommitted = true;
    }

    static void SetPlayback(UAttackActionState* Action, USkeletalMeshComponent* Mesh, int32 ID)
    {
        Action->PlayingMesh = Mesh;
        Action->MontageInstanceID = ID;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackInputTrackingTest,
    "ComboSystem.Charge.InputTracking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackInputTrackingTest::RunTest(const FString& Parameters)
{
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>();
    Machine->AttackPressed(EInputButtons::Invalid);
    TestFalse(TEXT("Invalid buttons are ignored"), Machine->IsKeyOnHold());

    Machine->AttackPressed(EInputButtons::HeavyAttack);
    Machine->AttackPressed(EInputButtons::LightAttack);
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    TestEqual(TEXT("Duplicate Started does not overwrite the newest buffer"),
        Machine->GetInputButton(), static_cast<int32>(EInputButtons::LightAttack));
    Machine->SetInputButton(0);
    Machine->SetInputState(0);
    TestTrue(TEXT("Consuming combo input preserves physical hold"),
        Machine->IsAttackButtonDown(EInputButtons::HeavyAttack));
    Machine->AttackReleased(EInputButtons::LightAttack);
    TestTrue(TEXT("Releasing light preserves heavy"), Machine->IsKeyOnHold());
    Machine->AttackReleased(EInputButtons::HeavyAttack);
    TestFalse(TEXT("No buttons remain held"), Machine->IsKeyOnHold());
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    Machine->AttackReleased(EInputButtons::HeavyAttack);
    TestEqual(TEXT("Quick tap remains a buffered press"),
        Machine->GetInputState(), static_cast<int32>(EButtonState::Pressed));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackChargeTierTest,
    "ComboSystem.Charge.TierAndReleaseLatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackChargeTierTest::RunTest(const FString& Parameters)
{
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>();
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    UAttackActionState* Action = FAttackChargeTestAccess::Activate(Machine);
    Action->ReachChargeTier(1);
    Action->ReachChargeTier(1);
    TestEqual(TEXT("Duplicate markers do not add tiers"), Action->GetChargeTier(), 1);
    Action->ReachChargeTier(0);
    TestEqual(TEXT("Tier cannot decrease"), Action->GetChargeTier(), 1);
    Machine->AttackReleased(EInputButtons::LightAttack);
    TestFalse(TEXT("Another button cannot request release"),
        FAttackChargeTestAccess::ReleaseRequested(Action));
    Machine->AttackReleased(EInputButtons::HeavyAttack);
    TestTrue(TEXT("Matching release is latched"), FAttackChargeTestAccess::ReleaseRequested(Action));
    TestTrue(TEXT("Input release alone does not execute the attack"), Action->IsCharging());
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    Action->ReachChargeTier(2);
    TestEqual(TEXT("Repressing cannot cancel an already requested release"), Action->GetChargeTier(), 1);
    Action->FinishAttack();
    TestTrue(TEXT("Finished marker cannot complete a charge before release"), Machine->IsCurrentState(Action));

    UAttackActionState* Next = FAttackChargeTestAccess::Activate(Machine);
    Next->ReachChargeTier(100);
    TestEqual(TEXT("Tier clamps to configured releases"), Next->GetChargeTier(), 2);
    Next->ReachChargeTier(-10);
    TestEqual(TEXT("Negative marker cannot reduce tier"), Next->GetChargeTier(), 2);
    Action->ReachChargeTier(2);
    TestEqual(TEXT("Inactive action ignores tier markers"), Action->GetChargeTier(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackChargePlaybackTest,
    "ComboSystem.Charge.PlaybackOwnershipAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackChargePlaybackTest::RunTest(const FString& Parameters)
{
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>();
    UAttackActionState* Action = FAttackChargeTestAccess::Activate(Machine);
    USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>();
    USkeletalMeshComponent* OtherMesh = NewObject<USkeletalMeshComponent>();
    FAttackChargeTestAccess::SetPlayback(Action, Mesh, 42);
    TestTrue(TEXT("Current playback accepted"), Action->AcceptsNotify(Mesh, 42));
    TestFalse(TEXT("Previous playback rejected"), Action->AcceptsNotify(Mesh, 41));
    TestFalse(TEXT("Unknown playback rejected"), Action->AcceptsNotify(Mesh, INDEX_NONE));
    TestFalse(TEXT("Another mesh rejected"), Action->AcceptsNotify(OtherMesh, 42));

    Machine->AttackPressed(EInputButtons::HeavyAttack);
    Action->SetIsFinal(true);
    FAttackChargeTestAccess::MarkReleased(Action);
    Action->Update(0.016f);
    TestTrue(TEXT("Final attacks do not reset on the next update"), Machine->IsCurrentState(Action));
    Action->FinishAttack();
    TestTrue(TEXT("Finished returns to no state when no idle is configured"), Machine->IsCurrentState(nullptr));
    TestFalse(TEXT("Exited action rejects callbacks"), Action->AcceptsNotify(Mesh, 42));
    TestFalse(TEXT("Exited action is not charging"), Action->IsCharging());
    TestEqual(TEXT("Charge tier clears on exit"), Action->GetChargeTier(), 0);
    TestTrue(TEXT("Held input survives exit"), Machine->IsAttackButtonDown(EInputButtons::HeavyAttack));
    Action->FinishAttack();
    TestTrue(TEXT("Repeated finish is harmless"), Machine->IsCurrentState(nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackComboWindowsTest,
    "ComboSystem.Charge.ComboWindows",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackComboWindowsTest::RunTest(const FString& Parameters)
{
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>();
    UAttackActionState* Action = FAttackChargeTestAccess::Activate(Machine);
    FAnimNotifyEvent First;
    FAnimNotifyEvent Second;
    Action->OpenComboWindow(&First, EMoveProgressType::Normal);
    TestFalse(TEXT("Combos cannot open during charging"), Machine->ShouldTransitionToNextMove());
    FAttackChargeTestAccess::MarkReleased(Action);
    Action->OpenComboWindow(&First, EMoveProgressType::Normal);
    Action->OpenComboWindow(&Second, EMoveProgressType::Delayed);
    Action->CloseComboWindow(&First);
    TestTrue(TEXT("Older window end cannot close a newer window"), Machine->ShouldTransitionToNextMove());
    TestEqual(TEXT("Latest active window supplies combo timing"),
        Machine->GetComboProgressType(), static_cast<int32>(EMoveProgressType::Delayed));
    Action->CloseComboWindow(&Second);
    TestFalse(TEXT("Last window end closes transitions"), Machine->ShouldTransitionToNextMove());
    Action->CloseComboWindow(&Second);
    TestFalse(TEXT("Duplicate end remains closed"), Machine->ShouldTransitionToNextMove());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackNotifyRoutingTest,
    "ComboSystem.Charge.StateMachineNotifyRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackNotifyRoutingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world created"), World)) { return false; }
    AActor* Owner = World->SpawnActor<AActor>();
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>(Owner);
    USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Owner);
    USkeletalMeshComponent* OtherMesh = NewObject<USkeletalMeshComponent>(Owner);
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    UAttackActionState* Action = FAttackChargeTestAccess::Activate(Machine);
    FAttackChargeTestAccess::SetPlayback(Action, Mesh, 42);

    FAnimNotifyEvent Window;
    UAnimMontage* Montage = NewObject<UAnimMontage>();
    FAnimNotifyEventReference MissingContext(&Window, Montage);
    FAnimNotifyEventReference Current(&Window, Montage);
    Current.AddContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(42);
    FAnimNotifyEventReference Old(&Window, Montage);
    Old.AddContextData<UE::Anim::FAnimNotifyMontageInstanceContext>(41);

    Machine->HandleChargeTierNotify(Mesh, MissingContext, 2);
    Machine->HandleChargeTierNotify(Mesh, Old, 2);
    Machine->HandleChargeTierNotify(OtherMesh, Current, 2);
    TestEqual(TEXT("Machine rejects missing context, old playback, and wrong mesh"), Action->GetChargeTier(), 0);
    Machine->HandleChargeTierNotify(Mesh, Current, 1);
    TestEqual(TEXT("Machine forwards a valid tier event"), Action->GetChargeTier(), 1);
    Machine->HandleChargeReleaseNotify(Mesh, Current);
    TestTrue(TEXT("A release-window event does not release while held"), Action->IsCharging());

    FAttackChargeTestAccess::MarkReleased(Action);
    Machine->HandleComboWindowBeginNotify(Mesh, Current, EMoveProgressType::Normal);
    TestTrue(TEXT("Machine forwards combo-window begin"), Machine->ShouldTransitionToNextMove());
    Machine->HandleComboWindowEndNotify(Mesh, Old);
    TestTrue(TEXT("Old playback cannot close the current window"), Machine->ShouldTransitionToNextMove());
    Machine->HandleComboWindowEndNotify(Mesh, Current);
    TestFalse(TEXT("Machine forwards combo-window end"), Machine->ShouldTransitionToNextMove());
    Machine->HandleAttackFinishedNotify(Mesh, Old);
    TestTrue(TEXT("Old finish cannot reset current action"), Machine->IsCurrentState(Action));
    Machine->HandleAttackFinishedNotify(Mesh, Current);
    TestTrue(TEXT("Valid finish reaches current action"), Machine->IsCurrentState(nullptr));
    Machine->HandleChargeTierNotify(Mesh, Current, 2);
    TestEqual(TEXT("Events with no current action are harmless"), Action->GetChargeTier(), 0);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackThreeSectionConfigurationTest,
    "ComboSystem.Charge.ThreeSectionConfiguration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackThreeSectionConfigurationTest::RunTest(const FString& Parameters)
{
    UAttackActionState* Action = NewObject<UAttackActionState>();
    UAnimMontage* Montage = NewObject<UAnimMontage>();
    for (const FName Name : {FName(TEXT("Default")), FName(TEXT("FullChargeLoop")), FName(TEXT("Recovery"))})
    {
        Montage->CompositeSections.AddDefaulted_GetRef().SectionName = Name;
    }
    FAttackChargeTestAccess::SetChargeMontage(Action, Montage);
    TestTrue(TEXT("Only Default, FullChargeLoop and Recovery are required"),
        FAttackChargeTestAccess::HasValidChargeSettings(Action));
    auto& Tiers = FAttackChargeTestAccess::DamageTiers(Action);
    TestEqual(TEXT("Default damage tiers are retained"), Tiers.Num(), 3);
    TestEqual(TEXT("Normal damage multiplier"), Tiers[0].DamageMultiplier, 1.f);
    TestEqual(TEXT("Charged damage multiplier"), Tiers[1].DamageMultiplier, 1.5f);
    TestEqual(TEXT("Full-charge damage multiplier"), Tiers[2].DamageMultiplier, 2.f);
    Tiers[1].DamageMultiplier = 1.75f;
    TestTrue(TEXT("Custom damage does not require additional sections"),
        FAttackChargeTestAccess::HasValidChargeSettings(Action));
    Tiers[1].DamageMultiplier = -1.f;
    TestFalse(TEXT("Negative damage multiplier remains invalid"),
        FAttackChargeTestAccess::HasValidChargeSettings(Action));
    Tiers[1].DamageMultiplier = 1.75f;
    Montage->CompositeSections.Pop();
    TestFalse(TEXT("Recovery section is still required"),
        FAttackChargeTestAccess::HasValidChargeSettings(Action));
    Montage->CompositeSections.AddDefaulted_GetRef().SectionName = TEXT("Recovery");
    Montage->CompositeSections.RemoveAt(1);
    TestFalse(TEXT("Charge loop section is still required"),
        FAttackChargeTestAccess::HasValidChargeSettings(Action));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackInputExpiryTest,
    "ComboSystem.Charge.InputExpiryAndConsumption",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackInputExpiryTest::RunTest(const FString& Parameters)
{
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>();
    EInputButtons Button;
    EButtonState State;
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    FAttackChargeTestAccess::AgeInput(Machine, Machine->InputBufferDuration + 1.0);
    TestFalse(TEXT("Old input is hidden immediately, even before a tick"),
        Machine->PeekBufferedAttackInput(Button, State));
    TestEqual(TEXT("Legacy getter cannot expose stale input"), Machine->GetInputButton(), 0);
    TestTrue(TEXT("Expiry does not release the physically held button"),
        Machine->IsAttackButtonDown(EInputButtons::HeavyAttack));
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    TestFalse(TEXT("Repeated callbacks while held cannot refresh old input"),
        Machine->PeekBufferedAttackInput(Button, State));
    Machine->AttackReleased(EInputButtons::HeavyAttack);
    Machine->AttackPressed(EInputButtons::HeavyAttack);
    TestTrue(TEXT("A fresh physical press starts a new buffer lifetime"),
        Machine->PeekBufferedAttackInput(Button, State));
    Machine->AttackReleased(EInputButtons::HeavyAttack);
    Machine->AttackPressed(EInputButtons::LightAttack);
    TestTrue(TEXT("Only the latest command is consumed"), Machine->TryConsumeAttackInput(Button, State));
    TestTrue(TEXT("Latest command is light"), Button == EInputButtons::LightAttack);
    TestFalse(TEXT("The same command cannot carry into another attack"),
        Machine->TryConsumeAttackInput(Button, State));
    TestTrue(TEXT("Consumption does not clear held buttons"), Machine->IsAttackButtonDown(EInputButtons::LightAttack));
    Machine->AttackReleased(EInputButtons::LightAttack);
    Machine->SetInputButton(static_cast<int32>(EInputButtons::LightAttack));
    Machine->SetInputState(static_cast<int32>(EButtonState::Pressed));
    FAttackChargeTestAccess::AgeInput(Machine, Machine->InputBufferDuration + 1.0);
    Machine->SetInputButton(static_cast<int32>(EInputButtons::LightAttack));
    Machine->SetInputState(static_cast<int32>(EButtonState::Pressed));
    TestFalse(TEXT("Identical legacy setter values do not refresh an old command"),
        Machine->PeekBufferedAttackInput(Button, State));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackNotifyTransitionGateTest,
    "ComboSystem.Charge.NotifyTransitionGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackNotifyTransitionGateTest::RunTest(const FString& Parameters)
{
    UAttackStateMachine* Machine = NewObject<UAttackStateMachine>();
    UAttackActionState* Action = FAttackChargeTestAccess::Activate(Machine);
    FAttackChargeTestAccess::MarkReleased(Action);
    Machine->AttackPressed(EInputButtons::LightAttack);
    Machine->SetShouldTransitionToNextMove(true);
    TestFalse(TEXT("A flag alone cannot skip the animation before its notify window"),
        FAttackChargeTestAccess::CanChain(Action));
    Action->Update(0.016f);
    TestTrue(TEXT("Spamming before the window leaves the current attack running"), Machine->IsCurrentState(Action));
    EInputButtons Button;
    EButtonState State;
    TestTrue(TEXT("Early input waits for the notify instead of being consumed"),
        Machine->PeekBufferedAttackInput(Button, State));
    FAnimNotifyEvent Window;
    Action->OpenComboWindow(&Window, EMoveProgressType::Normal);
    TestTrue(TEXT("Notify window permits chaining"), FAttackChargeTestAccess::CanChain(Action));
    Action->SetIsFinal(true);
    TestFalse(TEXT("Final attacks still cannot chain"), FAttackChargeTestAccess::CanChain(Action));
    Action->SetIsFinal(false);
    Action->CloseComboWindow(&Window);
    TestFalse(TEXT("Window end prevents further transitions"), FAttackChargeTestAccess::CanChain(Action));
    return true;
}

#endif
