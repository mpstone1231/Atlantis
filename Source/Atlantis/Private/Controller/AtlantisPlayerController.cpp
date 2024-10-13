// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/AtlantisPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/AtlantisCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "../Atlantis.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Interface/AtlantisCombatInterface.h"
#include "GameFramework/HUD.h"

AAtlantisPlayerController::AAtlantisPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void AAtlantisPlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AAtlantisPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &AAtlantisPlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &AAtlantisPlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &AAtlantisPlayerController::OnSetDestinationReleased);

		EnhancedInputComponent->BindAction(EnterCombatClickAction, ETriggerEvent::Started, this, &AAtlantisPlayerController::OnEnterCombatStarted);
		EnhancedInputComponent->BindAction(EnterCombatClickAction, ETriggerEvent::Completed, this, &AAtlantisPlayerController::OnEnterCombatReleased);
		EnhancedInputComponent->BindAction(EnterCombatClickAction, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::OnEnterCombatTriggered);

		EnhancedInputComponent->BindAction(MouseMotionAction, ETriggerEvent::Triggered, this, &AAtlantisPlayerController::OnMouseMotionTriggered);
		EnhancedInputComponent->BindAction(MouseMotionAction, ETriggerEvent::Completed, this, &AAtlantisPlayerController::OnMouseMotionStopped);
	}
	else
	{
		UE_LOG(LogAtlantis, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AAtlantisPlayerController::OnInputStarted()
{
	StopMovement();
}

// Triggered every frame when the input is held down
void AAtlantisPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	
	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void AAtlantisPlayerController::OnSetDestinationReleased()
{
	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

void AAtlantisPlayerController::OnEnterCombatTriggered()
{
	// TODO: Modify this so instead of MousePosition, we get Mouse "velocity" projected onto combat plane
	// Might not even need a plane to trace along. We can use the height of the pawn actor and a plane with a normal of UpVector
	// Direction is projected onto plane and magnitude is scaled with raw mouse input.
	// Adds a 'force' to weapon / cursor
	// How/where is force handled? In Controller or character? Maybe here. 
	// To start: Weapon/Cursor can have mass, 'friction' (radial and longitudinal?) and such. Make sure to preserve angular momentum, especially when reaching the limits
	// Later: Resistance at the start of a swing and followthrough in mid swing. "Dampen" zones when sword goes through swing (off to sides of target direction?) 
	// Control controls some sort of physics object? Probe only? Used to do physics calculations, and tick updates the sword to match position/orientation? Try some stuff!

	// UKismetMathLibrary::ProjectVectorOnToPlane(MouseMotion, FVector::UpVector);

	//double MouseDeltaX, MouseDeltaY;
	//GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	//UE_LOG(LogAtlantis, Display, TEXT("Mouse: %lf, %lf"), MouseDeltaX, MouseDeltaY);
	//FVector2D MouseMotion = FVector2D(MouseDeltaX, MouseDeltaY);

	APawn* ControlledPawn = GetPawn();
	const bool bValidAndImplementsCombatInterface = (ControlledPawn && ControlledPawn->Implements<UAtlantisCombatInterface>());

	if (ControlledPawn && ControlledPawn->Implements<UAtlantisCombatInterface>())
	{
		IAtlantisCombatInterface::Execute_UpdateCombatGeometery(ControlledPawn);

		UE_LOG(LogAtlantis, Display, TEXT("Mouse: %lf, %lf"), MouseMotion.X, MouseMotion.Y);

		// Collapse all this to a function (getting combat data from mouse motion)
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		bool bHit = false;
		FVector2D MousePosition;

		if (LocalPlayer && LocalPlayer->ViewportClient)
		{
			if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
			{
				if (!bIsStartOfNewMouseMotion) //If just entering combat mode, get initial mouse position first
				{
					//MouseEndPosition = MouseStartPosition + MouseMotion;

					FVector WorldMouseStart, WorldMouseStartDir;
					FVector WorldMousePos, WorldMouseDir;
					//Can get rid of this!! Planar mout motion is just mouse motion, isn't it? Input plane is on screen plane.
					//Then set mouse cursor to weapon position!
					if (UGameplayStatics::DeprojectScreenToWorld(this, PrevMousePosition, WorldMouseStart, WorldMouseStartDir) &&
						UGameplayStatics::DeprojectScreenToWorld(this, MousePosition, WorldMousePos, WorldMouseDir))
					{
						FVector WeaponPosition = IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn);
						FSphere CombatSphere = IAtlantisCombatInterface::Execute_GetCombatSphere(ControlledPawn);
						FPlane InputPlane = IAtlantisCombatInterface::Execute_GetInputPlaneFromCamera(ControlledPawn);// DetermineInputPlane(WeaponPosition);

						FVector MouseStartOnCombatPlane, MouseEndOnCombatPlane;
						float T; //Unused
						UKismetMathLibrary::LinePlaneIntersection(WorldMouseStart, WorldMouseStart + WorldMouseStartDir * HitResultTraceDistance, InputPlane, T, MouseStartOnCombatPlane);
						UKismetMathLibrary::LinePlaneIntersection(WorldMousePos, WorldMousePos + WorldMouseDir * HitResultTraceDistance, InputPlane, T, MouseEndOnCombatPlane);
						FVector PlanarMouseMotion = MouseEndOnCombatPlane - MouseStartOnCombatPlane;

						UKismetSystemLibrary::DrawDebugPlane(GetWorld(), InputPlane, MouseEndOnCombatPlane, 20.f, FLinearColor::Yellow, 0.f);

						FVector WeaponRadialAxis = IAtlantisCombatInterface::Execute_GetWeaponRadialAxis(ControlledPawn);
						FVector WeaponLatitudinalAxis = IAtlantisCombatInterface::Execute_GetWeaponLatitudinalAxis(ControlledPawn);

						FVector InputRadialAxis, InputLatitudinalAxis;// = FVector::VectorPlaneProject(WeaponRadialAxis, OperationalCombatPlane.GetSafeNormal()).GetSafeNormal();
						FVector DisambiguatingAxis = (CombatSphere.Center - WeaponPosition).GetSafeNormal();
						//FVector InputLatitudinalAxis = FVector::VectorPlaneProject(WeaponLatitudinalAxis, OperationalCombatPlane.GetSafeNormal()).GetSafeNormal();
						if (ProjectRadialAndLatitudinalAxesOntoInputSpace(WeaponRadialAxis, WeaponLatitudinalAxis, DisambiguatingAxis, InputPlane, InputRadialAxis, InputLatitudinalAxis))
						{
							//Input axes are NOT orthonormalized... ought they be at least normalized? if its janky, try to normalize first 
							FVector2D MouseMotionInInputSpace = BreakMouseInputToInputSpaceComponents(PlanarMouseMotion, InputRadialAxis, InputLatitudinalAxis, InputPlane.GetSafeNormal());
							//	float InputRadialMagnitudeNormalized = FVector::DotProduct(PlanarMouseMotion, InputRadialAxis) / PlanarMouseMotion.Length();
							//	float InputLatitudinalMagnitudeNormalized = FVector::DotProduct(PlanarMouseMotion, InputLatitudinalAxis) / PlanarMouseMotion.Length();

							UKismetSystemLibrary::DrawDebugArrow(GetWorld(), MouseEndOnCombatPlane, MouseEndOnCombatPlane + 15.f * InputRadialAxis, 5.f, FLinearColor::Red, 0.f, 2.f);
							UKismetSystemLibrary::DrawDebugArrow(GetWorld(), MouseEndOnCombatPlane, MouseEndOnCombatPlane + 15.f * InputLatitudinalAxis, 5.f, FLinearColor::Blue, 0.f, 2.f);

							UKismetSystemLibrary::DrawDebugArrow(GetWorld(), MouseEndOnCombatPlane, MouseEndOnCombatPlane + 3.f*(MouseMotionInInputSpace.X * InputRadialAxis + MouseMotionInInputSpace.Y * InputLatitudinalAxis), 5.f, FLinearColor::Green, 0.f, 2.f);

							IAtlantisCombatInterface::Execute_HandleCombatInputMouseMotion(ControlledPawn, MouseStartOnCombatPlane, MouseMotionInInputSpace);
						}


						/*
						FVector MouseStartOnCombatPlane, MouseEndOnCombatPlane;
						FPlane CombatPlane = IAtlantisCombatInterface::Execute_GetCombatPlane(ControlledPawn);
						float T; //Unused
						UKismetMathLibrary::LinePlaneIntersection(WorldMouseStart, WorldMouseStart + WorldMouseStartDir * HitResultTraceDistance, CombatPlane, T, MouseStartOnCombatPlane);
						UKismetMathLibrary::LinePlaneIntersection(WorldMouseEnd, WorldMouseEnd + WorldMouseEndDir * HitResultTraceDistance, CombatPlane, T, MouseEndOnCombatPlane);

						FVector PlanarMouseMotion = MouseEndOnCombatPlane - MouseStartOnCombatPlane;
						IAtlantisCombatInterface::Execute_HandleCombatInputMouseMotion(ControlledPawn , MouseStartOnCombatPlane, PlanarMouseMotion, MouseMotion.Length());
						*/
						/*
						FPlane CombatPlane = IAtlantisCombatInterface::Execute_GetCombatPlane(ControlledPawn);
						float T; //Unused

						UKismetMathLibrary::LinePlaneIntersection(WorldMouseStart, WorldMouseStart + WorldMouseStartDir * HitResultTraceDistance, CombatPlane, T, MouseStartOnCombatPlane);
						UKismetMathLibrary::LinePlaneIntersection(WorldMouseEnd, WorldMouseEnd + WorldMouseEndDir * HitResultTraceDistance, CombatPlane, T, MouseEndOnCombatPlane);

						UKismetSystemLibrary::DrawDebugArrow(GetWorld(), MouseStartOnCombatPlane, MouseEndOnCombatPlane, MouseMotion.Length(), FLinearColor::Red, DebugArrowPersistTime, MouseMotion.Length()/2.f);
						*/
					}
				}

				PrevMousePosition = MousePosition;
				bIsStartOfNewMouseMotion = false;

			}
		}
		/*
		FVector WorldMouseMotion;
		FVector WorldDirection; //Not needed for our context (this is direction of ray for a mouse trace, if we were using position, but we're getting mouse motin in world space)

		if (UGameplayStatics::DeprojectScreenToWorld(this, MouseMotion, WorldMouseMotion, WorldDirection))
		{
			FVector MouseMotionOnCombatPlane = UKismetMathLibrary::ProjectVectorOnToPlane(WorldMouseMotion, FVector::UpVector);

			FVector DebugStart = ControlledPawn->GetActorLocation();
			DebugStart.Z = MouseMotionOnCombatPlane.Z;
			UKismetSystemLibrary::DrawDebugArrow(GetWorld(), DebugStart, DebugStart + 25.f * MouseMotionOnCombatPlane, 1.f, FLinearColor::Red, 0.2f, 1.5f);
		}*/
	}

	if (bValidAndImplementsCombatInterface)
	{
		TArray<FHitResult> Hits = TArray<FHitResult>();
		bool bHitSuccessful = false;
		bHitSuccessful = GetMultiLineHitResultsUnderCursor(ECC_CombatTracePlane, true, Hits);

		if (!Hits.IsEmpty())
		{
			for (FHitResult& Hit : Hits)
			{
				if (Hit.GetActor() == ControlledPawn) //If Collision hit is with this controller's pawn's combat plane
				{
					IAtlantisCombatInterface::Execute_HandleCombatInputMouseLocation(ControlledPawn, Hit.ImpactPoint);
					return;
				}
			}
		}
	}
}

void AAtlantisPlayerController::OnEnterCombatStarted()
{
	bInCombatMode = true;
	if (GetPawn() && GetPawn()->Implements<UAtlantisCombatInterface>())
	{
		IAtlantisCombatInterface::Execute_EnterCombatMode(GetPawn());
	}

}

void AAtlantisPlayerController::OnEnterCombatReleased()
{
	bInCombatMode = false;
	if (GetPawn() && GetPawn()->Implements<UAtlantisCombatInterface>())
	{
		IAtlantisCombatInterface::Execute_ExitCombatMode(GetPawn());
	}
	bIsStartOfNewMouseMotion = true;

}

void AAtlantisPlayerController::OnMouseMotionTriggered(const FInputActionInstance& Instance)
{
	// TODO! Two considerations! 1: Mouse Debug vector not contiguous (not each mouse motion is triggering this... problem?)
	//							 2: Should Mouse end be where mouse is (start) + Mouse motion, or is where mouse is the end and start is end - motion? (Make debug arrows last longer)
	//							 3: Mouse positions/mouse motions in viewport coordinates, not pixel coordinates. Also a problem? Scaling up by DPI or whatever might give me contiguous vectors?
	if (!bInCombatMode) return;
	
	MouseMotion = Instance.GetValue().Get<FVector2D>();
}

void AAtlantisPlayerController::OnMouseMotionStopped(const FInputActionInstance& Instance)
{
	MouseMotion = FVector2D::ZeroVector;
}


bool AAtlantisPlayerController::GetMultiLineHitResultsUnderCursor(ECollisionChannel TraceChannel, bool bTraceComplex, TArray<FHitResult>& HitResults) const
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	bool bHit = false;
	if (LocalPlayer && LocalPlayer->ViewportClient)
	{
		FVector2D MousePosition;
		if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
		{
			bHit = GetMultiLineHitResultsAtScreenPosition(MousePosition, TraceChannel, bTraceComplex, HitResults);
		}
	}

	return bHit;
}

bool AAtlantisPlayerController::GetMultiLineHitResultsAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, bool bTraceComplex, TArray<FHitResult>& HitResults) const
{
	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), bTraceComplex);
	return GetMultiLineHitResultsAtScreenPosition(ScreenPosition, TraceChannel, CollisionQueryParams, HitResults);
}

bool AAtlantisPlayerController::GetMultiLineHitResultsAtScreenPosition(const FVector2D ScreenPosition, const ECollisionChannel TraceChannel, const FCollisionQueryParams& CollisionQueryParams, TArray<FHitResult>& HitResults) const
{
	// Early out if we clicked on a HUD hitbox
	if (GetHUD() != NULL && GetHUD()->GetHitBoxAtCoordinates(ScreenPosition, true))
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (UGameplayStatics::DeprojectScreenToWorld(this, ScreenPosition, WorldOrigin, WorldDirection) == true)
	{
		return GetWorld()->LineTraceMultiByChannel(HitResults, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, TraceChannel, CollisionQueryParams);
	}

	return false;
}

FPlane AAtlantisPlayerController::DetermineInputPlane(const FVector& InputPlaneOrigin)
{
	FVector2D PlaneOriginScreenSpace;
	if (UGameplayStatics::ProjectWorldToScreen(this, InputPlaneOrigin, PlaneOriginScreenSpace))
	{
		FVector PlaneOriginOnScreenWorldSpace, PlaneOriginWorldDir;
		if (UGameplayStatics::DeprojectScreenToWorld(this, PlaneOriginScreenSpace, PlaneOriginOnScreenWorldSpace, PlaneOriginWorldDir))
		{
			return FPlane(InputPlaneOrigin, -PlaneOriginWorldDir);
		}
	}

	UE_LOG(LogAtlantis, Warning, TEXT("Could not determine an appropriate Input Plane in AAtlantisPlayerController::DetermineInputPlane!"));
	return FPlane();
}

bool AAtlantisPlayerController::ProjectRadialAndLatitudinalAxesOntoInputSpace(const FVector& WeaponRadialAxis, const FVector& WeaponLatitudinalAxis, const FVector& DisambiguatingAxis, const FPlane& InputSpace, FVector& InputRadialAxis, FVector& InputLatitudinalAxis)
{
	FVector InputPlaneNormal = InputSpace.GetSafeNormal();
	// Measures of how well each vector will project onto space

	// A measure of how well axes will project onto the input space
	//float RadialScore = FMath::Pow( (1.f - FMath::Abs(FVector::DotProduct(WeaponRadialAxis, InputSpace.GetSafeNormal()))), PowerOfDisambiguation);
	
	//FVector DisambiguatedRadialAxis = UKismetMathLibrary::Vector_SlerpNormals(DisambiguatingAxis, WeaponRadialAxis, RadialScore);// RadialScore* DisambiguatingAxis + (1.f - RadialScore) * WeaponRadialAxis; //FVector::SlerpVectorToDirection
	//FVector DisambiguatedLatitudinalAxis = UKismetMathLibrary::Vector_SlerpNormals(DisambiguatingAxis, WeaponLatitudinalAxis, LatitudinalScore);//LatitudinalScore * DisambiguatingAxis + (1.f - LatitudinalScore) * WeaponLatitudinalAxis;

	// Input Axes Not orthogonal, per se

	InputRadialAxis = FVector::VectorPlaneProject(WeaponRadialAxis, InputPlaneNormal).GetSafeNormal();
	InputLatitudinalAxis = FVector::CrossProduct(InputRadialAxis, InputPlaneNormal).GetSafeNormal();

	//FVector InputLatitudinalAxis_Interim = FVector::VectorPlaneProject(WeaponLatitudinalAxis, InputPlaneNormal).GetSafeNormal();
	FVector DisambiguatedAxisOnInputPlane = FVector::VectorPlaneProject(DisambiguatingAxis, InputPlaneNormal);
	FVector InputLatitudinalAxis_Interim = FVector::VectorPlaneProject(WeaponLatitudinalAxis, InputPlaneNormal).GetSafeNormal();//DisambiguatedAxisOnInputPlane.ProjectOnTo//FVector::VectorPlaneProject(WeaponLatitudinalAxis, InputPlaneNormal).GetSafeNormal();


	//float DisambiguatedLatitudinalScore = FMath::Abs(FVector::DotProduct(InputRadialAxis, InputLatitudinalAxis_Interim));
	
	//InputLatitudinalAxis = UKismetMathLibrary::Vector_SlerpNormals(WeaponLatitudinalAxis, DisambiguatedAxisOnInputPlane, DisambiguationAlpha * DisambiguatedLatitudinalScore);

	return true;
	/*
	float RadialScore = 1.f - FMath::Abs(FVector::DotProduct(WeaponRadialAxis, InputPlaneNormal));
	float LatitudinalScore = 1.f - FMath::Abs(FVector::DotProduct(WeaponLatitudinalAxis, InputPlaneNormal));

	if (RadialScore > 0.1f && RadialScore >= LatitudinalScore)
	{
		InputRadialAxis = FVector::VectorPlaneProject(WeaponRadialAxis, InputPlaneNormal).GetSafeNormal();
		InputLatitudinalAxis = FVector::CrossProduct(InputRadialAxis, InputPlaneNormal);

		return true;
	}
	else if (LatitudinalScore > 0.1f && LatitudinalScore >= RadialScore)
	{
		InputLatitudinalAxis = FVector::VectorPlaneProject(WeaponLatitudinalAxis, InputPlaneNormal).GetSafeNormal();
		InputRadialAxis = FVector::CrossProduct(InputLatitudinalAxis, -InputPlaneNormal);
		return true;
	}


	InputRadialAxis = FVector::Zero();
	InputLatitudinalAxis = FVector::Zero();
	UE_LOG(LogAtlantis, Warning, TEXT("No suitable tangential combat axes can be projected onto input space! In AtlanthisPlayerController::ProjectRadialAndLatitudinalAxesOntoInputSpace()"));
	return false; 
	*/
}

FVector2D AAtlantisPlayerController::BreakMouseInputToInputSpaceComponents(const FVector& PlanarMouseInput, const FVector& RadialAxis, const FVector& LatitudinalAxis, const FVector& PlanarNormal)
{
	
	FMatrix LinCombMatrix(FPlane(RadialAxis.X, RadialAxis.Y, RadialAxis.Z, 0),
							FPlane(LatitudinalAxis.X, LatitudinalAxis.Y, LatitudinalAxis.Z, 0),
							FPlane(PlanarNormal.X, PlanarNormal.Y, PlanarNormal.Z, 0),
							FPlane(0, 0, 0, 1));
	
	/*
	FMatrix LinCombMatrix(FPlane(RadialAxis.X, LatitudinalAxis.X, PlanarNormal.X, 0),
							FPlane(RadialAxis.Y, LatitudinalAxis.Y, PlanarNormal.Y, 0),
							FPlane(RadialAxis.Z, LatitudinalAxis.Z, PlanarNormal.Z, 0),
							FPlane(0, 0, 0, 1));
	*/
	FVector4 PlanarMouseInput4D = FVector4(PlanarMouseInput.X, PlanarMouseInput.Y, PlanarMouseInput.Z, 0);
//	FMatrix LinCombMatrixInv = LinCombMatrix.Inverse();

	//FVector4 LinCombResult = FVector4::Zero();// = FMath::Matrix//PlanarMouseInput4D * LinCombMatrix;
//	for (int i = 0; i < 4; i++)
//	{
//		LinCombMatrixInv.M[i]
//	}

	FVector4 LinCombResult = LinCombMatrix.Inverse().TransformFVector4(PlanarMouseInput4D); //acts as matrix multiplication

	return FVector2D(LinCombResult.X, LinCombResult.Y); //The radial axis and Latitudinal axis components of planar mouse input

}
