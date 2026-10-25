// REFERENCE COPY: earlier climbing, crouch and slide changes, since reverted.
// Stored outside Source/ so this file is not compiled into the project.
// This is the full movement-component implementation, not an additional translation unit.
// Do not compile alongside APSCharacterMovementComponent.cpp: it defines the same methods.
// The later aps.Debug switch is not included in this historical snapshot.
// Optional companion header change: add UFUNCTION(BlueprintCallable, Category =
// "Parkour|Crouch") immediately before void ToggleCrouch_Sliding();

// Fill out your copyright notice in the Description page of Project Settings.


#include "APSCharacterMovementComponent.h"

#include "APSCharacter.h"
#include "Animation/AnimMontage.h"
#include "APSWallRunnableInterface.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


// Called when the game starts
#pragma region Lifecycle
void UAPSCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
// #if !UE_BUILD_SHIPPING
// 	GetWorld()->GetTimerManager().SetTimerForNextTick(
// 		[this]()
// 		{
// 			if (UWorld* World = GetWorld())
// 			{
// 				if (APlayerController* PC = World->GetFirstPlayerController())
// 				{
// 					PC->ConsoleCommand(TEXT("show Collision"));
// 				}
// 			}
// 		}
// 	);
// #endif
	bWantsToWallRun = false;
	PrevNonWallrunnableActor = nullptr;
	CharacterOwner->GetCapsuleComponent()->OnComponentHit.AddDynamic(
		this, &UAPSCharacterMovementComponent::OnCapsuleHit);
	WallSearchTraceDistance = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.f;
	OwningPlayerCharacter = Cast<AAPSCharacter>(CharacterOwner);
	OwningPlayerAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
	
	if (OwningPlayerAnimInstance)
	{
		OwningPlayerAnimInstance->OnMontageEnded.AddDynamic(this, &UAPSCharacterMovementComponent::OnClimbMontageEnded);
		// Transition only when the montage has finished, rather than also firing
		// at blend-out and entering the movement state twice.
	}
	
	bNotifyApex = true;
}

void UAPSCharacterMovementComponent::AddInputVector(FVector WorldVector, bool bForce)
{
	if (IsWallRunning())
	{
		WallRunInputVector = WorldVector;
	}
	else
	{
		Super::AddInputVector(WorldVector, bForce);
	}
}

void UAPSCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	WallRunInputVector = {};

	// if (bWantsToVault)
	// {
	// 	TryStartVaulting();
	// }
}

void UAPSCharacterMovementComponent::ToggleWallRun(bool bEnableWallrun)
{
	if (bAutoWallRun) { return; }
	bWantsToWallRun = bEnableWallrun;

	if (!bEnableWallrun && IsWallRunning())
	{
		SetMovementMode(MOVE_Falling);
	}
}

bool UAPSCharacterMovementComponent::CanAttemptJump() const
{
	if (IsWallRunning())
	{
		return bWallRunInitialized;
	}
	return Super::CanAttemptJump();
}

void UAPSCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case CMOVE_WallRunning:
		PhysWallRunning(deltaTime, Iterations);
		break;
	case CMOVE_Climb:
		PhysClimbing(deltaTime, Iterations);
		break;
	case CMOVE_Slide:
		PhysSliding(deltaTime, Iterations);
		break;
	default:
		Super::PhysCustom(deltaTime, Iterations);
		break;
	}
}
void UAPSCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (MovementMode == MOVE_Falling && PreviousMovementMode !=MOVE_Falling)
	{
		FallApexHeight = UpdatedComponent->GetComponentLocation().Z;
		bNotifyApex =true;
	}
	
	if (IsClimbing())
	{
		bOrientRotationToMovement = false;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(48.f);

		OnEnterClimbStateDelegate.ExecuteIfBound();
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Climb)
	{
		bOrientRotationToMovement = true;
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);

		const FRotator DirtyRotation = UpdatedComponent->GetComponentRotation();
		const FRotator CleanStandRotation = FRotator(0.f, DirtyRotation.Yaw, 0.f);
		UpdatedComponent->SetRelativeRotation(CleanStandRotation);

		StopMovementImmediately();

		OnExitClimbStateDelegate.ExecuteIfBound();
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_WallRunning)
	{
		GetWorld()->GetTimerManager().SetTimer(WallRunCooldownTimerHandle, WallRunCooldownDuration, false);
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Slide)
	{
		CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(CapsuleDefaultHeight);
		CharacterOwner->GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

#pragma endregion


bool UAPSCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
	
	FHitResult WallHit;
	if (CanWallJump(WallHit))
	{
		DoWallJump(WallHit);
		return true;
	}
	const bool bIsMidJump = IsFalling() && OwningPlayerCharacter->JumpCurrentCount > 0;
	if (bIsMidJump)
	{
		Velocity.Z = MidJumpVelocity;
		PlayClimbMontage(MidJumpMontage);
	}
	
	
	return Super::DoJump(bReplayingMoves);
}


#pragma region WallRunTrace

void UAPSCharacterMovementComponent::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                                  UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                                                  const FHitResult& Hit)
{
	if (OtherActor == PrevNonWallrunnableActor && !Cast<IAPSWallRunnableInterface>(OtherActor) ) { return; }

	if (CanWallRunUp(Hit.ImpactNormal)|| CanWallRun())
	{
		WallRunHitResult = Hit;
		InitWallRun();
	}
	else
	{
		PrevNonWallrunnableActor = OtherActor;
	}
}

#pragma endregion

#pragma region WallRunCore

bool UAPSCharacterMovementComponent::IsWallRunning() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_WallRunning;
}

bool UAPSCharacterMovementComponent::CanWallRun() const
{
	return (bAutoWallRun || bWantsToWallRun) && IsFalling() && !IsWallRunCooldownActive();
}

bool UAPSCharacterMovementComponent::CanWallRunUp(FVector WallNormal) const
{
	const double ForwardProjWallNormal = FVector::DotProduct(CharacterOwner->GetActorForwardVector(),
															WallRunHitResult.ImpactNormal);
	
	return (ForwardProjWallNormal <= -WallForwardFacingThreshold);
}

void UAPSCharacterMovementComponent::InitWallRun()
{
	WallRunInputVector = {};
	bWallRunInitialized = false;
	bIsTurningAroundCorner = false;
	CurrentWallRunUpTime = 0.f;
	SetMovementMode(MOVE_Custom, CMOVE_WallRunning);
	if (CanWallRunUp(WallRunHitResult.ImpactNormal))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("WallRunUp"));
		WallRunSide = EWRS_Front;
	}
	else
	{
		const double RightProjWallNormal = FVector::DotProduct(CharacterOwner->GetActorRightVector(),
															   WallRunHitResult.ImpactNormal);
		WallRunSide = RightProjWallNormal > 0.0 ? EWRS_LeftSide : EWRS_RightSide;		
	}
	FRotator TargetRotation{};

	CalcWallRunRotation(TargetRotation);
	const FLatentActionInfo LatentActionInfo{0, INDEX_NONE,TEXT("OnWallRunInitComplete"), this};
	UKismetSystemLibrary::MoveComponentTo(CharacterOwner->GetRootComponent(), CharacterOwner->GetActorLocation(),
	                                      TargetRotation, true, true, 0.2f, true, EMoveComponentAction::Move,
	                                      LatentActionInfo);
}

void UAPSCharacterMovementComponent::CalcWallRunRotation(FRotator& OutWallRotation)
{
	FVector Y{};
	switch (WallRunSide)
	{
	case EWRS_LeftSide: Y = WallRunHitResult.ImpactNormal;
		break;
	case EWRS_RightSide: Y = -WallRunHitResult.ImpactNormal;
		break;
	case EWRS_Front: 
		OutWallRotation = (-WallRunHitResult.ImpactNormal).GetSafeNormal().Rotation();
		return;
	default: ;
	}
	// if (WallRunSide == EWRS_LeftSide)
	// {
	// 	Y = WallRunHitResult.ImpactNormal;
	// }
	// else if (WallRunSide == EWRS_RightSide)
	// {
	// 	Y = -WallRunHitResult.ImpactNormal;
	// }
	// else if (Wa)
	// {
	// 	
	// }

	const FVector X = FVector::CrossProduct(Y, CharacterOwner->GetActorUpVector()).GetSafeNormal();
	OutWallRotation = FRotationMatrix::MakeFromXY(X, Y).Rotator();
}

void UAPSCharacterMovementComponent::OnWallRunInitComplete()
{
	bWallRunInitialized = true;
}

void UAPSCharacterMovementComponent::PhysWallRunning(float deltaTime, int32 Iterations)
{
	if (!bWallRunInitialized || bIsTurningAroundCorner || deltaTime < MIN_TICK_TIME) { return; }

	if (WallRunSide == EWRS_Front)
	{
		PhysWallRunningUp(deltaTime, Iterations);
		return;
	}
	
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FVector TraceDirection = CharacterOwner->GetActorForwardVector();
	FVector TraceEnd = TraceStart + TraceDirection * WallSearchTraceDistance; // Wall Trace
	GetWorld()->LineTraceSingleByChannel(WallRunHitResult, TraceStart, TraceEnd, ECC_Visibility);
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false);
	if (WallRunHitResult.bBlockingHit)
	{
		HandleWallRunCorner();
		return;
	}

	if (WallRunSide == EWRS_LeftSide)
	{
		TraceDirection = -CharacterOwner->GetActorRightVector();
	}
	else if (WallRunSide == EWRS_RightSide)
	{
		TraceDirection = CharacterOwner->GetActorRightVector();
	}
	TraceEnd = TraceStart + TraceDirection * WallSearchTraceDistance; // Wall Trace Distance

	GetWorld()->LineTraceSingleByChannel(WallRunHitResult, TraceStart, TraceEnd, ECC_Visibility);
	if (WallRunHitResult.bBlockingHit)
	{
		//To hug the wall
		const FVector CharacterToWall = WallRunHitResult.ImpactPoint - CharacterOwner->GetActorLocation();
		SafeMoveUpdatedComponent(CharacterToWall, CharacterOwner->GetActorRotation(), true, WallRunHitResult);

		FRotator TargetRotation{};
		CalcWallRunRotation(TargetRotation);
		const FRotator InterpedTargetRotation = FMath::RInterpTo(CharacterOwner->GetActorRotation(), TargetRotation,
		                                                         deltaTime, WallRunRotationInterpSpeed);
		CharacterOwner->SetActorRotation(InterpedTargetRotation);

		Velocity = CharacterOwner->GetActorForwardVector() * WallRunSpeed;
		const FVector AdjustedVelocity = Velocity * deltaTime;
		SafeMoveUpdatedComponent(AdjustedVelocity, InterpedTargetRotation, true, WallRunHitResult);

		return;
	}
	/*Check for outer Coners*/
	if (WallRunSide == EWRS_LeftSide)
	{
		TraceStart = CharacterOwner->GetActorLocation() + -CharacterOwner->GetActorRightVector() *
			WallSearchTraceDistance;
	}
	else if (WallRunSide == EWRS_RightSide)
	{
		TraceStart = CharacterOwner->GetActorLocation() + CharacterOwner->GetActorRightVector() *
			WallSearchTraceDistance;
	}
	TraceDirection = -CharacterOwner->GetActorForwardVector();
	TraceEnd = TraceStart + TraceDirection * WallSearchTraceDistance;
	GetWorld()->LineTraceSingleByChannel(WallRunHitResult, TraceStart, TraceEnd, ECC_Visibility);
	if (WallRunHitResult.bBlockingHit)
	{
		HandleWallRunCorner();
		return;
	}
	SetMovementMode(MOVE_Falling);
}

void UAPSCharacterMovementComponent::PhysWallRunningUp(float deltaTime, int32 Iterations)
{
	CurrentWallRunUpTime += deltaTime;
	if (CurrentWallRunUpTime > WallRunUpDuration)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("WallRunUp Times Up"));
		SetMovementMode(MOVE_Falling);
		return;
	}
	const FVector Start = CharacterOwner->GetActorLocation();
	const FVector End = Start + CharacterOwner->GetActorForwardVector() * WallSearchTraceDistance;
	GetWorld()->LineTraceSingleByChannel(WallRunHitResult,Start,End,ECC_Visibility);

	if (!WallRunHitResult.bBlockingHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("WallRunUp Invalid"));
		SetMovementMode(MOVE_Falling);
		return;
	}

	// if (CanStartClimb())
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Starting Climb"));
	// 	SetMovementMode(MOVE_Falling);
	// 	PlayClimbMontage(IdleToClimbMontage);
	// 	return;
	// }
	
	FRotator TargetRotation{};
	CalcWallRunRotation(TargetRotation);
	const FRotator InterpedTargetRotation = FMath::RInterpTo(CharacterOwner->GetActorRotation(), TargetRotation,
	                                                         deltaTime, WallRunRotationInterpSpeed);
	CharacterOwner->SetActorRotation(InterpedTargetRotation);
	const FVector CharacterToWall = WallRunHitResult.ImpactPoint - CharacterOwner->GetActorLocation();
	
	SafeMoveUpdatedComponent(CharacterToWall, InterpedTargetRotation, true, WallRunHitResult);
	Velocity = CharacterOwner->GetActorUpVector() * WallRunSpeed;
	const FVector AdjustedVelocity = Velocity * deltaTime;
	SafeMoveUpdatedComponent(AdjustedVelocity, InterpedTargetRotation, true, WallRunHitResult);
	
}

void UAPSCharacterMovementComponent::HandleWallRunCorner()
{
	bIsTurningAroundCorner = true;
	FRotator TargetRotation{};
	CalcWallRunRotation(TargetRotation);

	const FVector CornerTurnDirection = FRotationMatrix(TargetRotation).GetUnitAxis(EAxis::X);
	const bool bPlayerWantsToTurn = FVector::DotProduct(CornerTurnDirection, WallRunInputVector) > 0.f;
	if (bPlayerWantsToTurn)
	{
		const FVector TargetLocation = WallRunHitResult.ImpactPoint + WallRunHitResult.ImpactNormal * CharacterOwner->
			GetCapsuleComponent()->GetScaledCapsuleRadius();

		const FLatentActionInfo LatentActionInfo{0, INDEX_NONE, TEXT("OnWallRunCornerTurnComplete"), this};
		UKismetSystemLibrary::MoveComponentTo(CharacterOwner->GetRootComponent(), TargetLocation, TargetRotation, true,
		                                      true, WallRunCornerTurnDuration, true,
		                                      EMoveComponentAction::Move, LatentActionInfo);
	}
	else
	{
		SetMovementMode(MOVE_Falling);
	}
}

void UAPSCharacterMovementComponent::OnWallRunCornerTurnComplete()
{
	bIsTurningAroundCorner = false;
}

bool UAPSCharacterMovementComponent::IsWallRunCooldownActive() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(WallRunCooldownTimerHandle);
}

#pragma endregion

#pragma region MovementOverrides

float UAPSCharacterMovementComponent::GetMaxSpeed() const
{
	if (IsClimbing())
	{
		return MaxClimbSpeed;
	}
	else
	{
		return Super::GetMaxSpeed();
	}
}

float UAPSCharacterMovementComponent::GetMaxAcceleration() const
{
	if (IsClimbing())
	{
		return MaxClimbAcceleration;
	}
	else
	{
		return Super::GetMaxAcceleration();
	}
}

FVector UAPSCharacterMovementComponent::ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity,
                                                                        const FVector& CurrentVelocity) const
{
	const bool bIsPlayingRMMontage =
		IsFalling() && OwningPlayerAnimInstance && OwningPlayerAnimInstance->IsAnyMontagePlaying();

	if (bIsPlayingRMMontage)
	{
		return RootMotionVelocity;
	}
	else
	{
		return Super::ConstrainAnimRootMotionVelocity(RootMotionVelocity, CurrentVelocity);
	}
}

#pragma endregion

#pragma region ClimbTraces

TArray<FHitResult> UAPSCharacterMovementComponent::DoCapusleTraceMultipleByObject(
	const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	TArray<FHitResult> OutCapsuleTraceHitResults;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;

		if (bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::CapsuleTraceMultiForObjects(
		this,
		Start,
		End,
		ClimbCapusleTraceRadius,
		ClimbCapusleHalfHeight,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutCapsuleTraceHitResults,
		false
	);

	return OutCapsuleTraceHitResults;
}

FHitResult UAPSCharacterMovementComponent::DoLineCapsuleTraceSingleByObject(
	const FVector& Start, const FVector& End, bool bShowDebugShape, bool bDrawPersistantShapes)
{
	FHitResult OutHit;

	EDrawDebugTrace::Type DebugTraceType = EDrawDebugTrace::None;

	if (bShowDebugShape)
	{
		DebugTraceType = EDrawDebugTrace::ForOneFrame;

		if (bDrawPersistantShapes)
		{
			DebugTraceType = EDrawDebugTrace::Persistent;
		}
	}

	UKismetSystemLibrary::LineTraceSingleForObjects(
		this,
		Start,
		End,
		ClimbableSurfaceTraceTypes,
		false,
		TArray<AActor*>(),
		DebugTraceType,
		OutHit,
		false
	);

	return OutHit;
}

#pragma endregion

#pragma region ClimbCore

void UAPSCharacterMovementComponent::ToggleClimbing(bool bEnableClimb)
{
	if (bEnableClimb)
	{
		if (CanStartClimb())
		{
			if (!IdleToClimbMontage || !OwningPlayerAnimInstance) return;
			// Motion Warping targets the character's feet, rather than its capsule center.
			const FVector EntryTarget = UpdatedComponent->GetComponentLocation()
				+ FVector::UpVector * (60.f - CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			SetMotionWarpTarget(TEXT("ClimbEntryPoint"), EntryTarget);
			PlayClimbMontage(IdleToClimbMontage);
			if (OwningPlayerAnimInstance->Montage_IsPlaying(IdleToClimbMontage))
			{
				// Entry root motion can travel vertically; walking would discard Z.
				SetMovementMode(MOVE_Flying);
			}
		}
		else if (CanClimbDown())
		{
			PlayClimbMontage(ClimbDownLedgeMontage);
		}
		else
		{
			TryStartVaulting();
		}
	}

	if (!bEnableClimb)
	{
		if (OwningPlayerAnimInstance)
		{
			UAnimMontage* Active = OwningPlayerAnimInstance->GetCurrentActiveMontage();
			if (Active && (Active == IdleToClimbMontage || Active == ClimbToTopMontage || Active == ClimbDownLedgeMontage))
			{
				OwningPlayerAnimInstance->Montage_Stop(0.1f, Active);
			}
		}
		//Stop climbing
		StopClimbing();
	}
}


bool UAPSCharacterMovementComponent::CanStartClimb()
{
	if (IsFalling()) return false;
	if (!TraceClimbableSurfaces()) return false;
	if (!TraceFromEyeHeight(100.f).bBlockingHit) return false;

	return true;
}

bool UAPSCharacterMovementComponent::CanClimbDown()
{
	if (IsFalling()) return false;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	const FVector WalkableSurfaceTraceStart = ComponentLocation + ComponentForward *
		ClimbDownWalkableSurfaceTraceOffset;
	const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

	FHitResult WalkableSurfaceHit =
		DoLineCapsuleTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);

	const FVector LedgeTraceStart = WalkableSurfaceHit.TraceStart + ComponentForward * ClimbDownLedgeTraceOffset;
	const FVector LedgeTraceEnd = LedgeTraceStart + DownVector * 200.f;

	FHitResult LedgeTraceHit = DoLineCapsuleTraceSingleByObject(LedgeTraceStart, LedgeTraceEnd);

	if (WalkableSurfaceHit.bBlockingHit && !LedgeTraceHit.bBlockingHit)
	{
		return true;
	}

	return false;
}

void UAPSCharacterMovementComponent::StartClimbing()
{
	SetMovementMode(MOVE_Custom, CMOVE_Climb);
}

void UAPSCharacterMovementComponent::StopClimbing()
{
	SetMovementMode(MOVE_Falling);
}

void UAPSCharacterMovementComponent::PhysClimbing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	// A ledge exit owns its root motion. Wall snapping during this montage
	// would pull the capsule back into the wall as it moves over the lip.
	if (ClimbToTopMontage && OwningPlayerAnimInstance && OwningPlayerAnimInstance->Montage_IsPlaying(ClimbToTopMontage))
	{
		RestorePreAdditiveRootMotionVelocity();
		ApplyRootMotionToVelocity(deltaTime);
		FHitResult Hit;
		const FVector Delta = Velocity * deltaTime;
		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
		if (Hit.IsValidBlockingHit())
		{
			SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}
		return;
	}

	/*Process all the climbable surfaces info*/
	TraceClimbableSurfaces();
	ProcessClimbableInfo();

	/*Check if we should stop climbing*/
	if (CheckShouldStopClimbing() || CheckHasReachedFloor())
	{
		StopClimbing();
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		//Define the max climb speed and acceleration
		CalcVelocity(deltaTime, 0.f, true, MaxBreakClimbDeceleration);
	}

	ApplyRootMotionToVelocity(deltaTime);

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	//Handle climb rotation
	SafeMoveUpdatedComponent(Adjusted, GetClimbRotation(deltaTime), true, Hit);

	if (Hit.Time < 1.f)
	{
		//adjust and try again
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	/*Snap movement to climbable surfaces*/
	SnapMovementToClimableSurface(deltaTime);

	if (CheckHasReachedLedge())
	{
		PlayClimbMontage(ClimbToTopMontage);
	}
}

void UAPSCharacterMovementComponent::ProcessClimbableInfo()
{
	CurrentClimbableSurfaceLocation = FVector::ZeroVector;
	CurrentClimbableSurfaceNormal = FVector::ZeroVector;

	if (ClimbableSurfacesTracedResults.IsEmpty()) return;

	for (const FHitResult& TracedHitResult : ClimbableSurfacesTracedResults)
	{
		CurrentClimbableSurfaceLocation += TracedHitResult.ImpactPoint;
		CurrentClimbableSurfaceNormal += TracedHitResult.ImpactNormal;
	}

	CurrentClimbableSurfaceLocation /= ClimbableSurfacesTracedResults.Num();
	CurrentClimbableSurfaceNormal = CurrentClimbableSurfaceNormal.GetSafeNormal();
}

bool UAPSCharacterMovementComponent::CheckShouldStopClimbing()
{
	if (ClimbableSurfacesTracedResults.IsEmpty()) return true;

	const float DotResult = FVector::DotProduct(CurrentClimbableSurfaceNormal, FVector::UpVector);
	const float DegreeDiff = FMath::RadiansToDegrees(FMath::Acos(DotResult));

	if (DegreeDiff <= 60.f)
	{
		return true;
	}

	return false;
}

bool UAPSCharacterMovementComponent::CheckHasReachedFloor()
{
	const FVector DownVector = -UpdatedComponent->GetUpVector();
	const FVector StartOffset = DownVector * 50.f;

	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + DownVector;

	TArray<FHitResult> PossibleFloorHits = DoCapusleTraceMultipleByObject(Start, End);

	if (PossibleFloorHits.IsEmpty()) return false;

	for (const FHitResult& PossibleFloorHit : PossibleFloorHits)
	{
		const bool bFloorReached =
			FVector::Parallel(-PossibleFloorHit.ImpactNormal, FVector::UpVector) &&
			GetUnrotatedClimbingVelocity().Z < -10.f;

		if (bFloorReached)
		{
			return true;
		}
	}

	return false;
}

FQuat UAPSCharacterMovementComponent::GetClimbRotation(float DeltaTime)
{
	const FQuat CurrentQuat = UpdatedComponent->GetComponentQuat();

	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		return CurrentQuat;
	}

	const FQuat TargetQuat = FRotationMatrix::MakeFromX(-CurrentClimbableSurfaceNormal).ToQuat();

	return FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 5.f);
}

void UAPSCharacterMovementComponent::SnapMovementToClimableSurface(float DeltaTime)
{
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();

	const FVector ProjectedCharacterToSurface =
		(CurrentClimbableSurfaceLocation - ComponentLocation).ProjectOnTo(ComponentForward);

	const FVector SnapVector = -CurrentClimbableSurfaceNormal * ProjectedCharacterToSurface.Length();

	UpdatedComponent->MoveComponent(
		SnapVector * DeltaTime * MaxClimbSpeed,
		UpdatedComponent->GetComponentQuat(),
		true);
}

bool UAPSCharacterMovementComponent::CheckHasReachedLedge()
{
	FHitResult LedgetHitResult = TraceFromEyeHeight(100.f, 50.f);

	if (!LedgetHitResult.bBlockingHit)
	{
		const FVector WalkableSurfaceTraceStart = LedgetHitResult.TraceEnd;

		const FVector DownVector = -UpdatedComponent->GetUpVector();
		const FVector WalkableSurfaceTraceEnd = WalkableSurfaceTraceStart + DownVector * 100.f;

		FHitResult WalkabkeSurfaceHitResult =
			DoLineCapsuleTraceSingleByObject(WalkableSurfaceTraceStart, WalkableSurfaceTraceEnd);

		if (WalkabkeSurfaceHitResult.bBlockingHit && GetUnrotatedClimbingVelocity().Z > 10.f)
		{
			const FVector StandLocation = WalkabkeSurfaceHitResult.ImpactPoint
				+ UpdatedComponent->GetForwardVector() * 55.f
				+ FVector::UpVector * 2.f;
			SetMotionWarpTarget(TEXT("ClimbTopPoint"), StandLocation);
			return true;
		}
	}

	return false;
}

void UAPSCharacterMovementComponent::TryStartVaulting()
{
	FVector VaultStartPosition;
	FVector VaultLandPosition;
	
	float VaultDistance = 0.f;

	if (CanStartVaulting(VaultStartPosition, VaultLandPosition, VaultDistance))
	{
		SetMotionWarpTarget(FName("VaultStartPoint"), VaultStartPosition);
		SetMotionWarpTarget(FName("VaultEndPoint"), VaultLandPosition);
		
		UAnimMontage* SelectedMontage = nullptr;
		for (const FVaultMontageData MontageData : VaultMontageData)
		{
			if (VaultDistance >= MontageData.MinVaultDistance && VaultDistance <= MontageData.MaxVaultDistance)
			{
				SelectedMontage = MontageData.Montage;
				// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("Selected Montage: %s"), *SelectedMontage->GetName()));
				break;
			}
			else if (VaultDistance > VaultMontageData[VaultMontageData.Num()-1].MaxVaultDistance)
			{
				SelectedMontage = VaultMontageData[VaultMontageData.Num()-1].Montage;
				// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("MAX Selected Montage: %s"), *SelectedMontage->GetName()));
				break;
			}
		}
		if (SelectedMontage == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("No Montage Found for Vault Distance"));
			return;
		}
		StartClimbing();
		PlayClimbMontage(SelectedMontage);
		bWantsToVault = false;
	}
}

bool UAPSCharacterMovementComponent::CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition,float& OutDistance)
{
	if (IsFalling()) return false;

	OutVaultStartPosition = FVector::ZeroVector;
	OutVaultLandPosition = FVector::ZeroVector;

	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector ComponentForward = UpdatedComponent->GetForwardVector();
	const FVector UpVector = UpdatedComponent->GetUpVector();
	const FVector DownVector = -UpdatedComponent->GetUpVector();

	float RunningTopHitZ = 0.f;
	bool bHasTopHit = false;
	
	for (int32 i = 0; i < 10; i++)
	{
		const FVector Start = ComponentLocation + UpVector * 100.f +
			ComponentForward * 50.f * (i + 1);

		const FVector End = Start + DownVector * 100.f * (i + 1);

		FHitResult VaultTraceHit = DoLineCapsuleTraceSingleByObject(Start, End,true,false);

		if (i == 0 && VaultTraceHit.bBlockingHit)
		{
			OutVaultStartPosition = VaultTraceHit.ImpactPoint;
			RunningTopHitZ = VaultTraceHit.ImpactPoint.Z;
			bHasTopHit = true;
		}
		
		if (i !=0 && VaultTraceHit.bBlockingHit)
		{
			const float HitZ = VaultTraceHit.ImpactPoint.Z;
			const float DropFromObstacleTop = bHasTopHit ? (RunningTopHitZ - HitZ) : 0.f;
			if (DropFromObstacleTop > 15.f || i == 9)
			{
				// We've dropped well below the obstacle's top surface — this is the far edge's
				// landing point. Take it and stop, instead of letting later, further-out rays
				// (which may land even further away, e.g. down a slope) overwrite it.
				
				OutVaultLandPosition = VaultTraceHit.ImpactPoint;
				break;
			}
			// Still within tolerance of the obstacle's top surface — treat it as more obstacle,
			// not the landing point yet, and keep refining the running "top" height.
			RunningTopHitZ = HitZ;
			bHasTopHit = true;
			// OutVaultLandPosition = VaultTraceHit.ImpactPoint;
		}
	}
	OutDistance = FVector::Dist2D(OutVaultStartPosition, OutVaultLandPosition);
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("Distance: %f"), OutDistance));
	if (OutVaultStartPosition != FVector::ZeroVector && OutVaultLandPosition != FVector::ZeroVector)
	{
		return true;
	}
	return false;
}

bool UAPSCharacterMovementComponent::IsVaultMontage(const UAnimMontage* Montage) const
{
	return VaultMontageData.ContainsByPredicate([Montage](const FVaultMontageData& VaultData)
	{
		return VaultData.Montage == Montage;
	});
}

bool UAPSCharacterMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climb;
}
#pragma region WallJumpCore
bool UAPSCharacterMovementComponent::CanWallJump(FHitResult& OutWallHit) const
{
	if (!IsWallRunning() && IsFalling()) { return false; }
	
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + UpdatedComponent->GetForwardVector() * WallJumpTraceDistance;
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(CharacterOwner);
	
	FHitResult OppositeWallHitResult;
	if (!GetWorld()->LineTraceSingleByChannel(OppositeWallHitResult,Start,End,ECC_Visibility,CollisionQueryParams))
	{
		return false;
	}
	
	if (FMath::Abs(OppositeWallHitResult.ImpactNormal.Z)>MaxWallJumpSurfaceNormalZ)
	{
		return false;
	}
	
	OutWallHit = OppositeWallHitResult;
	return true;
	
}

void UAPSCharacterMovementComponent::DoWallJump(const FHitResult& WallHit)
{
	const FVector PushDirection =WallHit.ImpactNormal.GetSafeNormal();
	
	CharacterOwner->SetActorRotation(PushDirection.Rotation());
	Velocity = PushDirection * WallJumpOffPower + FVector::UpVector * WallJumpUpPower;
	
	SetMovementMode(MOVE_Falling);
}
#pragma endregion

//Trace for climbable surfaces, return true if there are indeed valid surfaces, false otherwise
bool UAPSCharacterMovementComponent::TraceClimbableSurfaces()
{
	const FVector StartOffset = UpdatedComponent->GetForwardVector() * 30.f;
	const FVector Start = UpdatedComponent->GetComponentLocation() + StartOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector();

	ClimbableSurfacesTracedResults = DoCapusleTraceMultipleByObject(Start, End);

	return !ClimbableSurfacesTracedResults.IsEmpty();
}

FHitResult UAPSCharacterMovementComponent::TraceFromEyeHeight(float TraceDistance, float TraceStartOffset,
                                                              bool bShowDebugShape, bool bDrawPersistantShapes)
{
	const FVector ComponentLocation = UpdatedComponent->GetComponentLocation();
	const FVector EyeHeightOffset = UpdatedComponent->GetUpVector() * (CharacterOwner->BaseEyeHeight +
		TraceStartOffset);

	const FVector Start = ComponentLocation + EyeHeightOffset;
	const FVector End = Start + UpdatedComponent->GetForwardVector() * TraceDistance;

	return DoLineCapsuleTraceSingleByObject(Start, End, bShowDebugShape, bDrawPersistantShapes);
}

void UAPSCharacterMovementComponent::PlayClimbMontage(UAnimMontage* MontageToPlay)
{
	if (!MontageToPlay) return;
	if (!OwningPlayerAnimInstance) return;
	if (OwningPlayerAnimInstance->IsAnyMontagePlaying())
	{
		UAnimMontage* Active = OwningPlayerAnimInstance->GetCurrentActiveMontage();
		// In-place climb loops may be interrupted by the ledge-exit montage.
		if (!IsClimbing() || (Active && Active->HasRootMotion())) return;
	}

	OwningPlayerAnimInstance->Montage_Play(MontageToPlay);
}

void UAPSCharacterMovementComponent::OnClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted && (Montage == IdleToClimbMontage || Montage == ClimbToTopMontage || Montage == ClimbDownLedgeMontage))
	{
		SetMovementMode(MOVE_Falling);
		return;
	}
	if (Montage == IdleToClimbMontage || Montage == ClimbDownLedgeMontage)
	{
		StartClimbing();
		StopMovementImmediately();
	}

	if (Montage == ClimbToTopMontage || IsVaultMontage(Montage))
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UAPSCharacterMovementComponent::RequestHopping()
{
	const FVector UnrotatedLastInputVector =
		UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), GetLastInputVector());

	const float DotResult =
		FVector::DotProduct(UnrotatedLastInputVector.GetSafeNormal(), FVector::UpVector);

	if (DotResult >= 0.9f)
	{
		HandleHopUp();
	}
	else if (DotResult <= -0.9f)
	{
		HandleHopDown();
	}
}

void UAPSCharacterMovementComponent::SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition)
{
	if (!CharacterOwner) return;

	OwningPlayerCharacter->GetMotionWarpingComponent()->AddOrUpdateWarpTargetFromLocationAndRotation(
		InWarpTargetName,
		InTargetPosition,
		CharacterOwner->GetActorRotation()
	);
}

void UAPSCharacterMovementComponent::HandleHopUp()
{
	FVector HopUpTargetPoint;

	if (CanHandleHopUp(HopUpTargetPoint))
	{
		SetMotionWarpTarget(FName("HopUpTargetPoint"), HopUpTargetPoint);

		PlayClimbMontage(HopUpMontage);
	}
}

bool UAPSCharacterMovementComponent::CanHandleHopUp(FVector& OutHopUpTargetPosition)
{
	FHitResult HopUpHit = TraceFromEyeHeight(100.f, -20.f);
	FHitResult SaftyLedgeHit = TraceFromEyeHeight(100.f, 150.f);

	if (HopUpHit.bBlockingHit && SaftyLedgeHit.bBlockingHit)
	{
		OutHopUpTargetPosition = HopUpHit.ImpactPoint;

		return true;
	}

	return false;
}

void UAPSCharacterMovementComponent::HandleHopDown()
{
	FVector HopDownTargetPoint;

	if (CanHandleHopDown(HopDownTargetPoint))
	{
		SetMotionWarpTarget(FName("HopDownTargetPoint"), HopDownTargetPoint);

		PlayClimbMontage(HopDownMontage);
	}
}

bool UAPSCharacterMovementComponent::CanHandleHopDown(FVector& OutHopDownTargetPosition)
{
	FHitResult HopDownHit = TraceFromEyeHeight(100.f, -300.f);

	if (HopDownHit.bBlockingHit)
	{
		OutHopDownTargetPosition = HopDownHit.ImpactPoint;

		return true;
	}

	return false;
}


FVector UAPSCharacterMovementComponent::GetUnrotatedClimbingVelocity() const
{
	return UKismetMathLibrary::Quat_UnrotateVector(UpdatedComponent->GetComponentQuat(), Velocity);
}


#pragma endregion

#pragma region SlideCore

void UAPSCharacterMovementComponent::ToggleCrouch_Sliding()
{
	if (!IsMovingOnGround() || IsSliding()) return;
	// Walking should still allow crouching; reserve slides for sprint speed.
	if (Velocity.SizeSquared2D() >= FMath::Square(600.f) && !IsCrouching())
	{
		FFindFloorResult FloorHit;
		if (CanSlide(FloorHit))
		{
			InitSlide();
		}
	}
	else
	{
		if (IsCrouching())
		{
			bWantsToCrouch = false;
			UnCrouch();
		}
		else
		{
			bWantsToCrouch = true;
			Crouch();
		}
	}
}

bool UAPSCharacterMovementComponent::CanSlide(FFindFloorResult& OutFloorHit) const
{
	// if (Velocity.IsNearlyZero())	
	// {
	// 	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Cannot Slide: Velocity is Zero"));
	// }
	FindFloor(UpdatedComponent->GetComponentLocation(), OutFloorHit, false);
	return OutFloorHit.bWalkableFloor;
}

bool UAPSCharacterMovementComponent::IsSliding() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Slide;
}

void UAPSCharacterMovementComponent::InitSlide()
{
	SetMovementMode(MOVE_Custom, CMOVE_Slide);


	PlayClimbMontage(SlideMontage);
	SetCollisionSizeToSlidingDimensions();
	bWantsToSlide = true;
}

void UAPSCharacterMovementComponent::PhysSliding(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME) { return; }

	CurrentSlideTime += deltaTime;
	
	
	FFindFloorResult FloorHit;
	
	if (!CanSlide(FloorHit))
	{
		ExitSlide();
		StartNewPhysics(deltaTime, Iterations);
		return;
	}


	//Align the character to the floor normal during slide
	FRotator TargetRotation = UKismetMathLibrary::MakeRotFromZX(FloorHit.HitResult.ImpactNormal,
	                                                            UpdatedComponent->GetForwardVector());
	FRotator NewRotation = FMath::RInterpTo(CharacterOwner->GetActorRotation(), TargetRotation, deltaTime, 5.f);
	CharacterOwner->SetActorRotation(NewRotation);

	//Check if we collide with anything during slide
	FHitResult SlideHitResult;
	bool bIsWallHitDuringSlide = UKismetSystemLibrary::SphereTraceSingle(this,
	                                                                     GetCharacterOwner()->GetMesh()->
	                                                                     GetSocketLocation(TraceSpawnSocketName),
	                                                                     GetCharacterOwner()->GetMesh()->
	                                                                     GetSocketLocation(TraceSpawnSocketName) +
	                                                                     FVector(
		                                                                     0.f, 0.f, TraceOffsetWhenSliding),
	                                                                     20.f,
	                                                                     static_cast<ETraceTypeQuery>(ECC_Visibility),
	                                                                     false,
	                                                                     TArray<AActor*>(
		                                                                     {FloorHit.HitResult.GetActor()}),
	                                                                     // EDrawDebugTrace::Persistent,
	                                                                     EDrawDebugTrace::ForDuration,
	                                                                     SlideHitResult,
	                                                                     false
	);

	if (bIsWallHitDuringSlide)
	{
		double Acos = UKismetMathLibrary::DegAcos(FVector::DotProduct(SlideHitResult.ImpactNormal, FVector::UpVector));
		// GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("CurrentFloorAngle: %f"), Acos));
		CurrentFloorAngle = FMath::Abs(Acos);
		if (CurrentFloorAngle > 80.0f)
		{
			// ExitSlide();
		}
	}


	FVector Delta = SlideSpeed * Velocity * deltaTime;

	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, NewRotation.Quaternion(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit, deltaTime, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}

	if (CurrentSlideTime >= SlideDuration )
	{
		ExitSlide();
		// StopSlide();
		return;
	}

}

void UAPSCharacterMovementComponent::SetCollisionSizeToSlidingDimensions()
{
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	const float ClampedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, CapsuleHalfHeightWhenSliding);
	const float HalfHeightAdjust = OldUnscaledHalfHeight - ClampedHalfHeight;
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, ClampedHalfHeight);

	UpdatedComponent->MoveComponent(
		FVector(0.f, 0.f, -(HalfHeightAdjust * ComponentScale)),
		UpdatedComponent->GetComponentQuat(), true, nullptr,
		MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);

	bForceNextFloorCheck = true;
	AdjustProxyCapsuleSize();
}

void UAPSCharacterMovementComponent::ExitSlide()
{
	bWantsToSlide = false;
	CurrentSlideTime = 0.f;
	OwningPlayerAnimInstance->Montage_Stop(0.2f);
	PlayClimbMontage(SlideEndMontage);
	FVector Start =UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetUpVector() * 50.f;
	FHitResult LineTraceForWhenUnderObstacle = DoLineCapsuleTraceSingleByObject(Start, Start + UpdatedComponent->GetUpVector() *100.f ,true,true);
	SetMovementMode(MOVE_Walking);
	// Reconcile the manually resized slide capsule with Character's crouch state.
	// UnCrouch performs its own overlap check before restoring standing height.
	bWantsToCrouch = true;
	Crouch();
	if (!LineTraceForWhenUnderObstacle.bBlockingHit)
	{
		bWantsToCrouch = false;
		UnCrouch();
	}
}

#pragma endregion

#pragma region CrouchCore

bool UAPSCharacterMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && !IsClimbing() && !IsWallRunning() && !bWantsToSlide;
}

void UAPSCharacterMovementComponent::NotifyJumpApex()
{
	FallApexHeight = UpdatedComponent->GetComponentLocation().Z;
	Super::NotifyJumpApex();
}

UAnimMontage* UAPSCharacterMovementComponent::SelectLandingMontage(float FallHeight) const
{
	UAnimMontage* SelectedMontage = nullptr;
	for (const FLandingMontageData MontageData : FallingMontageData)
	{
		if (FallHeight >= MontageData.MinFallHeight && FallHeight <= MontageData.MaxFallHeight)
		{
			SelectedMontage = MontageData.Montage;
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("Selected Montage: %s"), *SelectedMontage->GetName()));
			break;
		}
		
	}
	if (SelectedMontage == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("No Montage Found for Vault Distance"));
		return nullptr;
	}
	return SelectedMontage;
}


void UAPSCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	float const CurrentFallHeight = FMath::Max(0.f,FallApexHeight-Hit.ImpactPoint.Z);
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("Fall HEight: %f"), CurrentFallHeight));
	if (UAnimMontage* LandingMontage = SelectLandingMontage(CurrentFallHeight))
	{
		PlayClimbMontage(LandingMontage);
	}
	OnCharacterLandedDelegate.Broadcast(CurrentFallHeight);
	
	Super::ProcessLanded(Hit, remainingTime, Iterations);
}
#pragma endregion 
