// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "AtlantisPlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
struct FInputActionInstance;
//class UCurveFloat;


UCLASS()
class AAtlantisPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAtlantisPlayerController();
	
	virtual void Tick(float DeltaSeconds) override;

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float ShortPressThreshold;

	/** FX Class that we will spawn when clicking */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UNiagaraSystem* FXCursor;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputMappingContext* DefaultMappingContext;
	
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputAction* SetDestinationClickAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EnterCombatClickAction;

	/** Mouse Motion Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MouseMotionAction;

protected:
	
	virtual void SetupInputComponent() override;
	
	// Parent Overrides
	virtual void BeginPlay() override;
	virtual void SetPawn(APawn* InPawn) override;

	/** Input handlers for SetDestination action. */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();

	/** Input handlers for EnterCombat action. */
//	void OnEnterCombatTriggered();
	void OnLockSlashingPlaneStarted();
	void OnLockSlashingPlaneReleased();

	/** Input handlers for MouseMotion action. */
	void OnMouseMotionTriggered(const FInputActionInstance& Instance);
	void OnMouseMotionStopped(const FInputActionInstance& Instance);

	/** Explansions to Player Controller default helper functions */
	bool GetMultiLineHitResultsUnderCursor(ECollisionChannel TraceChannel, bool bTraceComplex, TArray<FHitResult>& HitResults) const;
	bool GetMultiLineHitResultsAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, bool bTraceComplex, TArray<FHitResult>& HitResults) const;
	bool GetMultiLineHitResultsAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams, TArray<FHitResult>& HitResults) const;

	

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/* Combat */
	bool bInCombatMode = true;
	bool bSlashingPlaneIsLocked = false;
	

	UPROPERTY(EditDefaultsOnly)
	float DisambiguationAlpha = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sphere Interaction")
	float ScreenDistanceScoreFactor = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sphere Interaction")
	float ScreenSpaceScoreCriticalRadius = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sphere Interaction")
	float ToCurrentWeaponDistanceScoreFactor = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Feel")
	float MouseLerpAlpha = 0.2f;

	UPROPERTY(EditDefaultsOnly)
	bool bDrawDebug = false;

private:

	/* Combat General */
	void UpdateCombatGeometry();
	void UpdateSlashingPlane(const FVector& OldWeaponLocation, const FVector& NewWeaponLocation);

	/* Combat Input */
	bool GetMouseOnScreen(FVector2D& MousePosition /*Out*/);
	bool DetermineTargetWeaponLocationFromMouse(APawn* ControlledPawn, const FVector2D& MouseOnScreen, FVector& TargetWeaponPosition /*Out*/);
	bool DetermineTargetWeaponLocationFromCursorOnCombatSphere(APawn* ControlledPawn, const FVector2D& MouseOnScreen, const FVector& MouseWorldSpace, const FVector& MouseWorldDir, FVector& MousePositionOnSphere /*Out*/);
	bool DetermineTargetWeaponLocationFromCursorOnPlane(APawn* ControlledPawn, const FVector& MouseWorldSpace, const FVector& MouseWorldDir, const FPlane& Plane, FVector& OutPositionOnSphere /*Out*/);
	bool TransformPointOntoSlashingPlane(FVector& PointToTransformOntoSlashingPlane);
	FVector DetermineSphereIntersectionToUseFromScore(const FVector2D& MouseOnScreen, const FVector& CurrentWeaponLocation, const FVector& SlashingPlaneClose, const FVector& SlashingPlaneFar);

	FPlane DetermineInputPlane(const FVector& InputPlaneOrigin);
	bool ProjectRadialAndLatitudinalAxesOntoInputSpace(const FVector& WeaponRadialAxis, const FVector& WeaponLatitudinalAxis, const FVector& DisambiguatingAxis, const FPlane& InputSpace, FVector& InputRadialAxis, FVector& InputLatitudinalAxis);
	FVector FindBestSphereIntersectionAsInput(const APawn* ControlledPawn, const FVector& WeaponPosition, const FVector& WeaponAngularMomentum, const FVector& IntersectionClose, const FVector& IntersectionFar);
	FVector FindSimpleBestSphereIntersectionAsInput(const APawn* ControlledPawn, const FVector& WeaponPosition, const FVector& IntersectionClose, const FVector& IntersectionFar);

	bool bSphereProjectionIsClose = true;
	bool bMouseWasInCombatSphere = false;
	FSphere CombatSphere = FSphere();
	float CombatSphereHeight = 0.f;
	float CombatSphereRadius = 1.f;
	float SphereIntersectionScoreBlendRange = 0.2f;
	//UCurveFloat* IntersectionBlendCurve;

	FVector2D MouseMotion = FVector2D::ZeroVector;
	FVector2D PrevMousePosition = FVector2D(-1.f, -1.f);
	bool bMouseWasIntersectingSphere = false;
	FPlane SlashingPlane = FPlane();

	/* Movement */
	FVector CachedDestination;

	float FollowTime; // For how long it has been pressed


};


