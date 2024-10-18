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


UCLASS()
class AAtlantisPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAtlantisPlayerController();

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
	
	// To add mapping context
	virtual void BeginPlay();

	/** Input handlers for SetDestination action. */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();

	/** Input handlers for EnterCombat action. */
	void OnEnterCombatTriggered();
	void OnEnterCombatStarted();
	void OnEnterCombatReleased();

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
	bool bInCombatMode = false;

	UPROPERTY(EditDefaultsOnly)
	float DisambiguationAlpha = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	bool bDrawDebug = false;

private:

	/* Combat Input */
	FPlane DetermineInputPlane(const FVector& InputPlaneOrigin);
	bool ProjectRadialAndLatitudinalAxesOntoInputSpace(const FVector& WeaponRadialAxis, const FVector& WeaponLatitudinalAxis, const FVector& DisambiguatingAxis, const FPlane& InputSpace, FVector& InputRadialAxis, FVector& InputLatitudinalAxis);
	FVector FindBestSphereIntersectionAsInput(const APawn* ControlledPawn, const FSphere& CombatSphere, const FVector& WeaponPosition, const FVector& WeaponAngularMomentum, const FVector& IntersectionClose, const FVector& IntersectionFar);

	/* Movement */
	FVector CachedDestination;

	float FollowTime; // For how long it has been pressed

	FVector2D MouseMotion = FVector2D::ZeroVector;
	FVector2D PrevMousePosition = FVector2D(-1.f, -1.f);
	bool bMouseWasIntersectingSphere = false;
	FPlane MouseOutsideSphereInputPlane = FPlane();

};


