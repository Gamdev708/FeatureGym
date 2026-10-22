// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// #include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "APSCharacterMovementComponent.generated.h"

class AAPSCharacter;

UENUM(BlueprintType)
enum ECustomMovementMode : int
{
	CMOVE_WallRunning UMETA(DisplayName = "WallRun Mode"),
	CMOVE_Climb UMETA(DisplayName = "Climb Mode"), 
	CMOVE_Slide UMETA(DisplayName = "Slide Mode"),
	// CMOVE_Crouch UMETA(DisplayName = "Crouch Mode"),
	CMOVE_Max UMETA(Hidden),
};

UENUM(BlueprintType, DisplayName = "Wall Run Side")
enum EWallRunSide:uint8
{
	EWRS_None UMETA(DisplayName = "None"),
	EWRS_LeftSide UMETA(DisplayName = "LeftSide"),
	EWRS_RightSide UMETA(DisplayName = "RightSide"),
	EWRS_Front UMETA(DisplayName = "Front"),
	EWRS_MAX UMETA(Hidden),
};

USTRUCT(BlueprintType)
struct FVaultMontageData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;

	// Inclusive lower bound, in cm, of the horizontal vault distance this montage covers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinVaultDistance = 0.f;

	// Exclusive upper bound, in cm, of the horizontal vault distance this montage covers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxVaultDistance = 100.f;

	// Inclusive lower bound, in cm, of the obstacle height this montage covers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinObstacleHeight = 0.f;

	// Exclusive upper bound, in cm, of the obstacle height this montage covers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxObstacleHeight = 200.f;
};

USTRUCT(BlueprintType)
struct FLandingMontageData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;

	// Inclusive lower bound, in cm, of the horizontal vault distance this montage covers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinFallHeight = 0.f;

	// Exclusive upper bound, in cm, of the horizontal vault distance this montage covers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxFallHeight = 100.f;
};

DECLARE_DELEGATE(FOnEnterWallRunState);
DECLARE_DELEGATE(FOnExitWallRunState);
DECLARE_DELEGATE(FOnEnterClimbState);
DECLARE_DELEGATE(FOnExitClimbState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterLanded, float, FallHeight);

// class UCurveFloat;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ADVANCEDPARKOURSYSTEM_API UAPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	FOnEnterWallRunState OnEnterWallRunStateDelegate;
	FOnExitWallRunState OnExitWallRunStateDelegate;

	FOnEnterClimbState OnEnterClimbStateDelegate;
	FOnExitClimbState OnExitClimbStateDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Character Movement: Falling")
	FOnCharacterLanded OnCharacterLandedDelegate;

	
	float GetMeshRelativeLocationZWhenCrouching() const { return MeshRelativeLocationZWhenCrouching; }
	
	virtual bool DoJump(bool bReplayingMoves) override;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void AddInputVector(FVector WorldVector, bool bForce = false) override;
	virtual bool CanAttemptJump() const override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override;

#pragma  region  WallRunTrace
	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                  FVector NormalImpulse, const FHitResult& Hit);
#pragma endregion
	
#pragma region ClimbTraces

	TArray<FHitResult> DoCapusleTraceMultipleByObject(const FVector& Start, const FVector& End, const bool bShowDebugShape = false, bool bDrawPersistantShapes = false);
	FHitResult DoLineCapsuleTraceSingleByObject(const FVector& Start, const FVector& End, const bool bShowDebugShape = false, bool bDrawPersistantShapes = false);
#pragma endregion
	
#pragma region WallRunCore
	bool CanWallRun() const;
	bool CanWallRunUp(FVector WallNormal)const;
	virtual void InitWallRun();
	virtual void CalcWallRunRotation(FRotator& OutWallRotation);
	UFUNCTION()
	virtual void OnWallRunInitComplete();
	virtual void PhysWallRunning(float deltaTime, int32 Iterations);
	virtual void PhysWallRunningUp(float deltaTime, int32 Iterations);
	virtual void HandleWallRunCorner();
	UFUNCTION()
	virtual void OnWallRunCornerTurnComplete();
	UFUNCTION(BlueprintCallable)
	bool IsWallRunning() const;
	UFUNCTION(BlueprintCallable)
	FORCEINLINE EWallRunSide GetWallRunSide() const { return WallRunSide; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsTurningAroundCorner() const { return bIsTurningAroundCorner; }
#pragma endregion
	
#pragma region WallJumpCore
	bool CanWallJump(FHitResult& OutWallHit) const;
	void DoWallJump(const FHitResult& WallHit);
#pragma endregion 
	
#pragma region ClimbCore

	bool TraceClimbableSurfaces();
	FHitResult TraceFromEyeHeight(float TraceDistance, float TraceStartOffset = 0.5f, const bool bShowDebugShape = false, bool bDrawPersistantShapes = false);

	bool CanStartClimb();
	
	bool CanClimbDown();
	void StartClimbing();
	void StopClimbing();

	void PhysClimbing(float deltaTime, int32 Iterations);

	void ProcessClimbableInfo();
	
	bool CheckShouldStopClimbing();
	bool CheckHasReachedFloor();
	bool CheckHasReachedLedge();
	FQuat GetClimbRotation(float DeltaTime);
	void SnapMovementToClimableSurface(float DeltaTime);
	void TryStartVaulting();
	bool CanStartVaulting(FVector& OutVaultStartPosition, FVector& OutVaultLandPosition,float& OutDistance);
	bool IsVaultMontage(const UAnimMontage* Montage) const;
	void PlayClimbMontage(UAnimMontage* MontageToPlay);
	UFUNCTION()	
	void OnClimbMontageEnded(UAnimMontage* Montage,bool bInterrupted);
	void SetMotionWarpTarget(const FName& InWarpTargetName, const FVector& InTargetPosition);

	void HandleHopUp();
	bool CanHandleHopUp(FVector& OutHopUpTargetPosition);
	void HandleHopDown();
	bool CanHandleHopDown(FVector& OutHopDownTargetPosition);

#pragma endregion
	
#pragma region SlideCore
	bool CanSlide(FFindFloorResult& OutFloorHit) const;
	bool IsSliding() const;
	virtual void InitSlide();
	void PhysSliding(float deltaTime, int32 Iterations);
	void SetCollisionSizeToSlidingDimensions();
	void ExitSlide();
#pragma endregion
	
#pragma region CrouchCore
	virtual bool CanCrouchInCurrentState() const override;
#pragma endregion
	
#pragma region FallCore
	virtual void NotifyJumpApex() override;
	UAnimMontage* SelectLandingMontage(float FallHeight) const;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
#pragma endregion 
	
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void ToggleWallRun(bool bEnableWallrun);
	void ToggleClimbing(bool bEnableCliming);
	void ToggleCrouch_Sliding();
	void RequestHopping();
	bool IsClimbing() const;
	FORCEINLINE FVector GetClimbableSurfaceNormal() const { return CurrentClimbableSurfaceNormal; }
	FVector GetUnrotatedClimbingVelocity() const;
private:
	
#pragma region WallRunCoreVariables
	bool IsWallRunCooldownActive() const;
	bool bWallRunInitialized = false;
	bool bIsTurningAroundCorner = false;
	bool bWantsToWallRun = false;
	
	FHitResult WallRunHitResult{};
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PrevNonWallrunnableActor{nullptr};

	EWallRunSide WallRunSide{EWRS_None};
	FVector WallRunInputVector;
	FTimerHandle WallRunCooldownTimerHandle;
	float CurrentWallRunUpTime = 0.f;
#pragma endregion
	
#pragma region WallRunBPVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallSearchTraceDistance = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallRunRotationInterpSpeed = 5.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallRunSpeed = 550.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallRunCooldownDuration = 0.7f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallRunCornerTurnDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	bool bAutoWallRun = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallForwardFacingThreshold = 0.7f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallRunUpSpeed = 400.f;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning" ,meta = (AllowPrivateAccess = "true"))
	float WallRunUpDuration = 3.f;
	
	
#pragma endregion
	
#pragma region WallJumpBpVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning|WallJump" ,meta = (AllowPrivateAccess = "true"))
	float WallJumpTraceDistance = 75.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning|WallJump" ,meta = (AllowPrivateAccess = "true"))
	float WallJumpOffPower = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning|WallJump" ,meta = (AllowPrivateAccess = "true"))
	float WallJumpUpPower = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: WallRunning|WallJump" ,meta = (AllowPrivateAccess = "true"))
	float MaxWallJumpSurfaceNormalZ = 0.3;
	
	
#pragma endregion 

#pragma region ClimbCoreVaribles
	TArray<FHitResult> ClimbableSurfacesTracedResults;

	FVector CurrentClimbableSurfaceNormal;
	FVector CurrentClimbableSurfaceLocation;


	UPROPERTY()	UAnimInstance* OwningPlayerAnimInstance;
	UPROPERTY()	AAPSCharacter* OwningPlayerCharacter;
	bool bWantsToVault = false;

#pragma endregion
	
#pragma region ClimbBPVariables

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	TArray <TEnumAsByte<EObjectTypeQuery>> ClimbableSurfaceTraceTypes;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float ClimbCapusleTraceRadius = 50.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float ClimbCapusleHalfHeight = 72.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float ClimbDownWalkableSurfaceTraceOffset = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float ClimbDownLedgeTraceOffset = 50.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float MaxBreakClimbDeceleration = 400.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float MaxClimbSpeed = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	float MaxClimbAcceleration = 300.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing|Vault" ,meta = (AllowPrivateAccess = "true"))
	TArray <TEnumAsByte<EObjectTypeQuery>> VaultableSurfaceTraceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleToClimbMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbToTopMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ClimbDownLedgeMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopUpMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HopDownMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Climbing|Vault" ,meta = (AllowPrivateAccess = "true"))
	TArray<FVaultMontageData> VaultMontageData;
#pragma endregion
	
#pragma region SlideCoreVariables
	// float SlideSpeed = 800.0f;
	float CurrentSlideTime = 0.0f;
	float CurrentFloorAngle = 0.0f;
	bool bWantsToSlide = false;
#pragma endregion 	
	
#pragma region SlideCoreBPVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	float SlideDuration = 3.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	float SlideSpeed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	float CapsuleHalfHeightWhenSliding = 30.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	float MeshRelativeLocationZWhenSliding = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	float TraceOffsetWhenSliding;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	FName TraceSpawnSocketName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch|Slide" ,meta = (AllowPrivateAccess = "true"))
	TArray <TEnumAsByte<EObjectTypeQuery>> SlideSurfaceTraceTypes;
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	// UCurveFloat* SlideCurve;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* SlideMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* SlideEndMontage;
#pragma endregion	
		
#pragma region CrouchCoreBPVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	float CapsuleDefaultHeight = 90.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	float CapsuleDefaultRadius = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	float MeshDefaultLocationZ = -90.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	float CrouchSpeed = 300.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	float CapsuleHalfHeightWhenCrouching = 45.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Crouch" ,meta = (AllowPrivateAccess = "true"))
	float MeshRelativeLocationZWhenCrouching = -45.0f;
#pragma endregion
	
#pragma region FallingCoreVariables
	//Highest Z Height reached before the Current fall
	float FallApexHeight;
#pragma endregion 
	
#pragma region FallingCoreBPVariables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Falling" ,meta = (AllowPrivateAccess = "true"))
	float MidJumpVelocity = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Falling" ,meta = (AllowPrivateAccess = "true"))
	UAnimMontage* MidJumpMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement: Falling" ,meta = (AllowPrivateAccess = "true"))
	TArray <FLandingMontageData> FallingMontageData; 
#pragma endregion 
};

