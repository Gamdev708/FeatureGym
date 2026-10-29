#include "Showcase/FGShowcase.h"
#include "APSCharacterMovementComponent.h"
#include "State/Attack/AttackActionState.h"
#include "State/State.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

void UFGShowcaseCombatAnim::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    ACharacter* Character=Cast<ACharacter>(TryGetPawnOwner());
    if(!Character)return;
    // Preserve the supplied animation graph while removing its hard dependency
    // on BP_TestChar and the Prototypes C++ module.
    const float Speed=Character->GetVelocity().Size2D();
    if (FNumericProperty* P=FindFProperty<FNumericProperty>(GetClass(),TEXT("GroundSpeed")))
        P->SetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(this),Speed);
    if(FBoolProperty* P=FindFProperty<FBoolProperty>(GetClass(),TEXT("ShouldMove")))
        P->SetPropertyValue_InContainer(this,Speed>3.f);
    if(FBoolProperty* P=FindFProperty<FBoolProperty>(GetClass(),TEXT("IsFalling")))
        P->SetPropertyValue_InContainer(this,Character->GetCharacterMovement()->IsFalling());
}

AFGShowcaseTarget::AFGShowcaseTarget()
{
    PrimaryActorTick.bCanEverTick = true;
    SetCanBeDamaged(true);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetBody"));
    SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder"));
    Body->SetStaticMesh(Mesh.Object);
    Body->SetRelativeScale3D(FVector(.65,.65,1.8));
    Body->SetCollisionProfileName(TEXT("BlockAll"));
    Readout = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TargetReadout"));
    Readout->SetupAttachment(Body);
    Readout->SetAbsolute(false,false,true);
    Readout->SetRelativeLocation(FVector(0,0,100));
    Readout->SetRelativeRotation(FRotator(0,180,0));
    Readout->SetHorizontalAlignment(EHTA_Center);
    Readout->SetWorldSize(24);
    Readout->SetTextRenderColor(FColor::White);
    Readout->SetText(FText::FromString(TEXT("100 HP")));
}
float AFGShowcaseTarget::TakeDamage(float Damage, const FDamageEvent&, AController*, AActor*)
{
    if (Health <= 0 || Damage <= 0) return 0;
    LastDamage = Damage;
    Health = FMath::Max(0.f, Health-Damage);
    Readout->SetText(FText::FromString(FString::Printf(TEXT("%.0f HP\n-%.0f"),Health,Damage)));
    Readout->SetTextRenderColor(Health > 0 ? FColor::Orange : FColor::Green);
    ResetAt = GetWorld()->GetTimeSeconds()+4.f;
    return Damage;
}
void AFGShowcaseTarget::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (ResetAt > 0 && GetWorld()->GetTimeSeconds() >= ResetAt) ResetTarget();
}
void AFGShowcaseTarget::ResetTarget()
{
    Health=100; LastDamage=0; ResetAt=0;
    Readout->SetText(FText::FromString(TEXT("100 HP")));
    Readout->SetTextRenderColor(FColor::White);
}
AFGShowcaseCharacter::AFGShowcaseCharacter(const FObjectInitializer& Initializer) : Super(Initializer)
{
    PrimaryActorTick.bCanEverTick = true;
    Combo = CreateDefaultSubobject<UFGShowcaseCombo>(TEXT("ShowcaseCombo"));
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->SetCrouchedHalfHeight(45.f);
    GetCharacterMovement()->MaxWalkSpeedCrouched = 300.f;
    GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
}
void AFGShowcaseCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    // Movement, look, traversal and climb contexts use the same bindings as Prototypes.
    Super::SetupPlayerInputComponent(Input);
    // Only the Gym's demonstration controls are added here.
    Input->BindKey(EKeys::F,IE_Pressed,this,&AFGShowcaseCharacter::Hop);
    Input->BindKey(EKeys::One,IE_Pressed,this,&AFGShowcaseCharacter::TraverseMode);
    Input->BindKey(EKeys::Two,IE_Pressed,this,&AFGShowcaseCharacter::CombatMode);
    Input->BindKey(EKeys::R,IE_Pressed,this,&AFGShowcaseCharacter::ReturnToCheckpoint);
    Input->BindKey(EKeys::Home,IE_Pressed,this,&AFGShowcaseCharacter::ResetRun);
    Input->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&AFGShowcaseCharacter::LightPressed);
    Input->BindKey(EKeys::LeftMouseButton,IE_Released,this,&AFGShowcaseCharacter::LightReleased);
    Input->BindKey(EKeys::MiddleMouseButton,IE_Pressed,this,&AFGShowcaseCharacter::MediumPressed);
    Input->BindKey(EKeys::MiddleMouseButton,IE_Released,this,&AFGShowcaseCharacter::MediumReleased);
    Input->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&AFGShowcaseCharacter::HeavyPressed);
    Input->BindKey(EKeys::RightMouseButton,IE_Released,this,&AFGShowcaseCharacter::HeavyReleased);
    if (APlayerController* PC=Cast<APlayerController>(GetController()))
    { PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false; }
}
void AFGShowcaseCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    APlayerController* PC=Cast<APlayerController>(GetController());
    if (!PC || !IsLocallyControlled()) return;
    auto* Movement=CastChecked<UAPSCharacterMovementComponent>(GetCharacterMovement());
    Movement->MaxWalkSpeed=PC->IsInputKeyDown(EKeys::LeftShift)?800.f:500.f;
    const FVector P=GetActorLocation();
    const int32 Index=P.X < -5400 ? 0 : P.X < -2200 ? 1 : P.X < 1000 ? 2 : P.X < 4200 ? 3 : P.X < 7500 ? 4 : 5;
    static const TCHAR* Names[]={TEXT("00 / ARRIVAL"),TEXT("01 / VAULT + SLIDE"),TEXT("02 / WALL RUN + WALL JUMP"),TEXT("03 / CLIMB + LANDING"),TEXT("04 / COMBO LAB"),TEXT("05 / FINAL ARENA")};
    Station=Names[Index];
    if (Index!=StationIndex)
    {
        StationIndex=Index;
        static const float X[]={-7200,-5100,-1900,1300,4500,8000};
        Checkpoint=FVector(X[Index],0,110);
    }
    if (P.Z < -700) ReturnToCheckpoint();
    UAttackActionState* Action=Cast<UAttackActionState>(Combo->GetActiveState());
    if (Action != HitState.Get()) { HitState=Action;StateAge=DeltaSeconds; }
    else StateAge+=DeltaSeconds;
    // Application-side hit feedback. Combo selection, buffering and charge damage come from the plugin.
    if (bCombatMode && Action && StateAge<100.f && !Action->IsCharging())
    {
        StateAge=1000.f;
        const FVector Center=P+GetActorForwardVector()*140.f;
        const float Damage=Combo->GetCurrentStateDamage();
        for (TActorIterator<AFGShowcaseTarget> It(GetWorld());It;++It)
        {
            if (FVector::DistSquared(It->GetActorLocation(),Center)<FMath::Square(210.f))
            {
                const float Applied=UGameplayStatics::ApplyDamage(*It,Damage,PC,this,nullptr);
                if (Applied>0) {++Hits;LastHitDamage=Applied;}
            }
        }
    }
}
void AFGShowcaseCharacter::Climb()
{
    auto* Movement=CastChecked<UAPSCharacterMovementComponent>(GetCharacterMovement());
    if(!bCombatMode && !Movement->IsClimbing())Movement->ToggleClimbing(true);
}
void AFGShowcaseCharacter::StopClimb(){ CastChecked<UAPSCharacterMovementComponent>(GetCharacterMovement())->ToggleClimbing(false); }
void AFGShowcaseCharacter::Slide(){ if(!bCombatMode) CastChecked<UAPSCharacterMovementComponent>(GetCharacterMovement())->ToggleCrouch_Sliding(); }
void AFGShowcaseCharacter::Hop(){ if(!bCombatMode) CastChecked<UAPSCharacterMovementComponent>(GetCharacterMovement())->RequestHopping(); }
void AFGShowcaseCharacter::TraverseMode(){SwitchMode(false);}
void AFGShowcaseCharacter::CombatMode(){SwitchMode(true);}
void AFGShowcaseCharacter::SwitchMode(bool bCombat)
{
    if (bCombat==bCombatMode || GetCharacterMovement()->IsFalling() || CastChecked<UAPSCharacterMovementComponent>(GetCharacterMovement())->IsClimbing()) return;
    const TCHAR* Path=bCombat?TEXT("/Game/Showcase/Blueprints/BP_ShowcaseCombat.BP_ShowcaseCombat_C"):TEXT("/Game/Showcase/Blueprints/BP_ShowcaseTraversal.BP_ShowcaseTraversal_C");
    UClass* Class=LoadClass<AFGShowcaseCharacter>(nullptr,Path);
    if(!Class)return;
    FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AFGShowcaseCharacter* Next=GetWorld()->SpawnActor<AFGShowcaseCharacter>(Class,GetActorTransform(),Params);
    if(!Next)return;
    Next->Checkpoint=Checkpoint;Next->Elapsed=Elapsed;Next->Hits=Hits;Next->LastHitDamage=LastHitDamage;
    AController* C=GetController(); C->Possess(Next);Destroy();
}
void AFGShowcaseCharacter::ReturnToCheckpoint()
{
    Combo->ClearBufferedAttackInput();
    GetCharacterMovement()->StopMovementImmediately();
    if(auto* M=Cast<UAPSCharacterMovementComponent>(GetCharacterMovement()))M->ToggleClimbing(false);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    SetActorLocation(Checkpoint,false,nullptr,ETeleportType::TeleportPhysics);
    if(Controller)Controller->SetControlRotation(FRotator(0,0,0));
}
void AFGShowcaseCharacter::ResetRun()
{
    Checkpoint=FVector(-7200,0,110);Elapsed=0;Hits=0;LastHitDamage=0;ReturnToCheckpoint();
    for(TActorIterator<AFGShowcaseTarget> It(GetWorld());It;++It)It->ResetTarget();
}
void AFGShowcaseCharacter::LightPressed(){if(bCombatMode)Combo->AttackPressed(EInputButtons::LightAttack);}
void AFGShowcaseCharacter::LightReleased(){Combo->AttackReleased(EInputButtons::LightAttack);}
void AFGShowcaseCharacter::MediumPressed(){if(bCombatMode)Combo->AttackPressed(EInputButtons::MediumAttack);}
void AFGShowcaseCharacter::MediumReleased(){Combo->AttackReleased(EInputButtons::MediumAttack);}
void AFGShowcaseCharacter::HeavyPressed(){if(bCombatMode)Combo->AttackPressed(EInputButtons::HeavyAttack);}
void AFGShowcaseCharacter::HeavyReleased(){Combo->AttackReleased(EInputButtons::HeavyAttack);}
FString AFGShowcaseCharacter::GetMovementLabel() const
{
    if(GetCharacterMovement()->IsFalling())return TEXT("AIRBORNE");
    if(GetCharacterMovement()->MovementMode==MOVE_Custom)return FString::Printf(TEXT("CUSTOM MOVEMENT / %d"),GetCharacterMovement()->CustomMovementMode);
    return TEXT("GROUNDED");
}
void AFGShowcaseHUD::DrawHUD()
{
    Super::DrawHUD();
    AFGShowcaseCharacter* P=Cast<AFGShowcaseCharacter>(GetOwningPawn());if(!Canvas || !P)return;
    const float W=FMath::Min(760.f,Canvas->SizeX-40.f);
    DrawRect(FLinearColor(.015f,.025f,.04f,.88f),20,20,W,148);
    DrawText(TEXT("FEATURE GYM  /  SYSTEMS IN MOTION"),FLinearColor(.25f,.85f,1),36,30,nullptr,1.3f);
    DrawText(P->Station,FLinearColor::White,36,59,nullptr,1.2f);
    const FString Mode=P->bCombatMode?TEXT("COMBO SYSTEM"):TEXT("ADVANCED PARKOUR");
    DrawText(FString::Printf(TEXT("%s  |  %s  |  %.0f cm/s  |  %.1fs"),*Mode,*P->GetMovementLabel(),P->GetVelocity().Size(),P->Elapsed),FLinearColor(.7f,.8f,.85f),36,84);
    DrawText(TEXT("WASD move / Mouse look / Shift sprint / Space jump / 1 parkour / 2 combat"),FLinearColor::White,36,108);
    DrawText(P->bCombatMode?TEXT("LMB light + charge follow-up / R checkpoint / Home restart"):TEXT("E vault or climb / Q detach / C slide or crouch / F hop / R checkpoint / Home restart"),FLinearColor::White,36,132);
    if(P->bCombatMode)
    {
        DrawText(FString::Printf(TEXT("PLUGIN STATE: %s   DAMAGE: %.0f   TARGET HITS: %d   LAST HIT: %.0f"),P->Combo->IsInAttackState()?TEXT("ATTACK"):TEXT("IDLE"),P->Combo->GetCurrentStateDamage(),P->Hits,P->LastHitDamage),FLinearColor(1,.65f,.2f),36,178);
    }
}
AFGShowcaseGameMode::AFGShowcaseGameMode()
{
    HUDClass=AFGShowcaseHUD::StaticClass();
    DefaultPawnClass=AFGShowcaseCharacter::StaticClass();
}
