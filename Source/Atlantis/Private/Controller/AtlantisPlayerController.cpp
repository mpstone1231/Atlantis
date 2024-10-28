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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/HUD.h"
#include "Libraries/MathHelperLibrary.h"

AAtlantisPlayerController::AAtlantisPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}



void AAtlantisPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 1. Get Mouse Location on screen
	APawn* ControlledPawn = GetPawn();

	if (ControlledPawn && ControlledPawn->Implements<UAtlantisCombatInterface>())
	{
		UpdateCombatGeometry();

		FVector2D MousePositionScreen;
		if (GetMouseOnScreen(MousePositionScreen))
		{
			// Collect relevant Pawn Data
			FVector WeaponLocation = IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn);
			
			FVector MousePositionOnSphere;
			FVector TargetWeaponLocation = WeaponLocation;

			// Handle targets location and Mouse Input to determine a new sword location
			if (DetermineTargetWeaponLocationFromMouse(ControlledPawn, MousePositionScreen, MousePositionOnSphere))
			{
				TargetWeaponLocation = MousePositionOnSphere;
			}

			// Transforms the slashing plane by rotating it 
			if (!bSlashingPlaneIsLocked)
			{
				UpdateSlashingPlane(WeaponLocation, TargetWeaponLocation);
			}
			
			//Takes in MouseMotion right now, but doesn't do anything yet. Just sets weapon locaiton to target weapon location
			IAtlantisCombatInterface::Execute_HandleCombatInputMouseMotion(ControlledPawn, TargetWeaponLocation, MouseMotion);

			/* Update Cursor Location
			FVector2D UpdatedScreenPosition;
			if (UGameplayStatics::ProjectWorldToScreen(this, TargetWeaponLocation, UpdatedScreenPosition))
			{
				SetMouseLocation(UpdatedScreenPosition.X, UpdatedScreenPosition.Y);
			}*/

			if (bDrawDebug)
			{
				UKismetSystemLibrary::DrawDebugSphere(GetWorld(), CombatSphere.Center, CombatSphere.W, 20, FLinearColor::Blue, 0.f, 1.f);
				
			//	UKismetSystemLibrary::DrawDebugSphere(GetWorld(), MouseOnCombatPlane, 20.f, 12, FLinearColor::Yellow, 0.f, 1.f);
				UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetWeaponLocation, 20.f, 12, FLinearColor::Red, 0.f, 1.f);
			//	UKismetSystemLibrary::DrawDebugPlane(GetWorld(), SlashingPlane, CombatSphere.Center, CombatSphere.W, FLinearColor(1.f,0.f,1.f));
				
				FVector ToTargetWeaponLocation = (TargetWeaponLocation - CombatSphere.Center).GetUnsafeNormal();
				UKismetSystemLibrary::DrawDebugCircle(GetWorld(), CombatSphere.Center, CombatSphere.W, 24, FLinearColor::Yellow, 0.f, 2.f, ToTargetWeaponLocation, FVector::CrossProduct(ToTargetWeaponLocation, SlashingPlane.GetSafeNormal()));
			}
		}
	}

	/*
	if (bInCombatMode)
	{
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn && ControlledPawn->Implements<UAtlantisCombatInterface>())
		{
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
			FVector2D MousePosition;

			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
			{

				IAtlantisCombatInterface::Execute_UpdateCombatGeometery(ControlledPawn);
				FSphere CombatSphere = IAtlantisCombatInterface::Execute_GetCombatSphere(ControlledPawn);

				FVector WeaponLocation = IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn);
				const FVector WeaponLinearMomentum = IAtlantisCombatInterface::Execute_GetWeaponLinearMomentum(ControlledPawn);
				const FVector WeaponAngularMomentum = IAtlantisCombatInterface::Execute_GetWeaponAngularMomentum(ControlledPawn);
				//FVector WeaponPositionOnInputplane = UKismetMathLibrary::ProjectPointOnToPlane(WeaponPosition, InputPlane.GetOrigin(), InputPlane.GetNormal());

				FVector WorldMouseLocation, WorldMouseDir;
				if (UGameplayStatics::DeprojectScreenToWorld(this, MousePosition, WorldMouseLocation, WorldMouseDir))
				{
					FVector MouseOnSphereClose, MouseOnSphereFar;
					FVector TargetWeaponLocation = WeaponLocation;
					double T1, T2;
					if (UMathHelperLibrary::LineSphereIntersection(WorldMouseLocation, WorldMouseDir, CombatSphere, MouseOnSphereClose, MouseOnSphereFar, T1, T2))
					{
						bMouseWasIntersectingSphere = true;
						TargetWeaponLocation = FindBestSphereIntersectionAsInput(ControlledPawn, CombatSphere, WeaponLocation, WeaponAngularMomentum, MouseOnSphereClose, MouseOnSphereFar);

						if (!WeaponLinearMomentum.IsNearlyZero())
						{
							MouseSphereInputPlane = FPlane(CombatSphere.Center, WeaponLocation, WeaponLocation + WeaponLinearMomentum);
						}

						if (bDrawDebug) UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetWeaponLocation, 20.f, 12, FLinearColor::Red, 0.f, 1.f);
						//UKismetSystemLibrary::DrawDebugSphere(GetWorld(), MouseOnSphereFar, 20.f, 12, FLinearColor::Green, 0.f, 1.f);
					}
					else
					{

						if (bMouseWasIntersectingSphere)
						{
							if (!WeaponLinearMomentum.IsNearlyZero())
							{
								MouseSphereInputPlane = FPlane(CombatSphere.Center, WeaponLocation, WeaponLocation + WeaponLinearMomentum);
							}

							bMouseWasIntersectingSphere = false;
						}
						float T_Unused; //Unsued
						FVector MouseOnCombatPlane;
						if (UKismetMathLibrary::LinePlaneIntersection(WorldMouseLocation, WorldMouseLocation + WorldMouseDir * HitResultTraceDistance, MouseSphereInputPlane, T_Unused, MouseOnCombatPlane))
						{
							TargetWeaponLocation = CombatSphere.Center + (MouseOnCombatPlane - CombatSphere.Center).GetUnsafeNormal() * CombatSphere.W;

							if (bDrawDebug)
							{
								UKismetSystemLibrary::DrawDebugSphere(GetWorld(), MouseOnCombatPlane, 20.f, 12, FLinearColor::Yellow, 0.f, 1.f);
								UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetWeaponLocation, 20.f, 12, FLinearColor::Red, 0.f, 1.f);

								
							}
						}
					}

					if (bDrawDebug)
					{
						FVector ToTargetWeaponLocation = (TargetWeaponLocation - CombatSphere.Center).GetUnsafeNormal();
						UKismetSystemLibrary::DrawDebugCircle(GetWorld(), CombatSphere.Center, CombatSphere.W, 24, FLinearColor::Yellow, 0.f, 2.f, ToTargetWeaponLocation, FVector::CrossProduct(MouseSphereInputPlane, ToTargetWeaponLocation).GetSafeNormal());
					}

					IAtlantisCombatInterface::Execute_HandleCombatInputMouseMotion(ControlledPawn, TargetWeaponLocation, MouseMotion);

					FVector2D UpdatedWeaponLocationOnScreen;
					if (UGameplayStatics::ProjectWorldToScreen(this, IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn), UpdatedWeaponLocationOnScreen))
					{
						//SetMouseLocation(UpdatedWeaponLocationOnScreen.X, UpdatedWeaponLocationOnScreen.Z);
					}
				}
			}
		}
	} // if (bInCombatMode) */
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

void AAtlantisPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	if (InPawn && InPawn->Implements<UAtlantisCombatInterface>())
	{
		// Setup combat sphere based on controlled pawn's properties
		IAtlantisCombatInterface::Execute_GetCombatSphereProperties(InPawn, CombatSphereHeight, CombatSphereRadius);
		CombatSphere = FSphere(InPawn->GetActorLocation() + FVector(0.f, 0.f, CombatSphereHeight), CombatSphereRadius);
		SlashingPlane = FPlane(CombatSphere.Center, FVector::UpVector);
	}
	else
	{
		UE_LOG(LogAtlantis, Warning, TEXT("AtlantisPlayerController has possessed a pawn that does NOT implement the Combat Interface!"));
	}
}

// COMBAT AND HELPERS
void AAtlantisPlayerController::UpdateCombatGeometry()
{
	CombatSphere.Center = GetPawn()->GetActorLocation() + FVector::UpVector * CombatSphereHeight;
}

void AAtlantisPlayerController::UpdateSlashingPlane(const FVector& OldWeaponLocation, const FVector& NewWeaponLocation)
{
	// Problem with this... Plane looks right but the circle is a little skew... problem with normalization?
	
	const FVector ToOldWeapon = (OldWeaponLocation - CombatSphere.Center);
	const FVector ToNewWeapon = (NewWeaponLocation - CombatSphere.Center);
	FQuat qWeaponRotation = FQuat::FindBetween(ToOldWeapon, ToNewWeapon);
	FRotator RotationToApply = FRotator(qWeaponRotation);
	FVector SlashingPlaneNormal = SlashingPlane.GetSafeNormal();
	SlashingPlaneNormal = RotationToApply.RotateVector(SlashingPlaneNormal); //qWeaponRotation * SlashingPlaneNormal; //
	SlashingPlane = FPlane(CombatSphere.Center, SlashingPlaneNormal);
}

/* Passes out a Vector3D of the Mouse in the viewport*/
bool AAtlantisPlayerController::GetMouseOnScreen(FVector2D& MousePosition)
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
	{
		return true;
	}
	return false;

}

bool AAtlantisPlayerController::DetermineTargetWeaponLocationFromMouse(APawn* ControlledPawn, const FVector2D& MouseOnScreen, FVector& TargetWeaponPosition /*Out*/)
{
	FVector WorldMouseLocation, WorldMouseDir;
	if (UGameplayStatics::DeprojectScreenToWorld(this, MouseOnScreen, WorldMouseLocation, WorldMouseDir))
	{
		if (!bSlashingPlaneIsLocked)
		{
			// Try first to intersect mouse with combat sphere...
			if (DetermineTargetWeaponLocationFromCursorOnCombatSphere(ControlledPawn, WorldMouseLocation, WorldMouseDir, TargetWeaponPosition))
			{
				bMouseWasInCombatSphere = true;
				return true;
			}
			else //...if not, intersect with CombatPlane (origin shared with combat sphere, coplanar with camera plane)
			{
				FPlane CombatPlane = FPlane(CombatSphere.Center, -IAtlantisCombatInterface::Execute_GetCameraFacingDirection(ControlledPawn));
				if (DetermineTargetWeaponLocationFromCursorOnPlane(ControlledPawn, WorldMouseLocation, WorldMouseDir, CombatPlane, TargetWeaponPosition))
				{
					//If this is the first time mouse has gone outside the CombatSphere, then switch if cursor trace goes to far or close side 
					if (bMouseWasInCombatSphere) bSphereProjectionIsClose = !bSphereProjectionIsClose;
					bMouseWasInCombatSphere = false;
					return true;
				}
			}
		}
		else // Slashing plane is locked
		{
			// TODO: Handle this better for slashing planes near perpendicular to camera
			// I think this should be less about mouse plane intersections and more, where the target is now, how are you moving the mouse? In what direction?
			if (DetermineTargetWeaponLocationFromCursorOnPlane(ControlledPawn, WorldMouseLocation, WorldMouseDir, SlashingPlane, TargetWeaponPosition))
			{
				return true;
			}
		}
	}

	return false;
}

//If a trace from the cursor intersects the CombatSphere, passes out the intersection closest to the camera
bool AAtlantisPlayerController::DetermineTargetWeaponLocationFromCursorOnCombatSphere(APawn* ControlledPawn, const FVector& MouseWorldSpace, const FVector& MouseWorldDir, FVector& MousePositionOnSphere /*Out*/)
{
	// TODO! After drawing debug spheres, try and go back to using the intersection with closest geodesic distance to weapon location, and if intersecting with sphere set cursor to weapon location,
	// if intersecting with plane, ignore
	FVector MouseOnSphereClose, MouseOnSphereFar;
	double T1, T2;
	const FVector WeaponLocation = IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn);
	if (UMathHelperLibrary::LineSphereIntersection(MouseWorldSpace, MouseWorldDir, CombatSphere, MouseOnSphereClose, MouseOnSphereFar, T1, T2))
	{
		MousePositionOnSphere = bSphereProjectionIsClose ? MouseOnSphereClose : MouseOnSphereFar;//FindSimpleBestSphereIntersectionAsInput(ControlledPawn, CombatSphere, WeaponLocation, MouseOnSphereClose, MouseOnSphereFar);
		return true;
	}
	
	return false;
}

//First finds where the cursor intersects the given Plane (normal parallel to camera, intersecting origin of combat sphere).
//Next, we project the planar intersection onto the nearest point on the CombatSphere.
bool AAtlantisPlayerController::DetermineTargetWeaponLocationFromCursorOnPlane(APawn* ControlledPawn, const FVector& MouseWorldSpace, const FVector& MouseWorldDir, const FPlane& Plane, FVector& OutPositionOnSphere /*Out*/)
{
//	FPlane CombatPlane = FPlane(CombatSphere.Center, -IAtlantisCombatInterface::Execute_GetCameraFacingDirection(ControlledPawn));
	FVector WeaponLocation = IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn);

	float T_Unused; //Unsued
	FVector MousePositionOnPlane;
	if (UKismetMathLibrary::LinePlaneIntersection(MouseWorldSpace, MouseWorldSpace + MouseWorldDir * HitResultTraceDistance, Plane, T_Unused, MousePositionOnPlane))
	{
		// Get the location of the cursor if it were projected from the plane directly onto the nearest point on the combat sphere
		const FVector CursorProjectedOnSphereRelative = (MousePositionOnPlane - CombatSphere.Center).GetUnsafeNormal() * CombatSphere.W;
	
		OutPositionOnSphere = CombatSphere.Center + CursorProjectedOnSphereRelative;
		return true;
	}

	return false;
}

// INPUT SYSTEM

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

		EnhancedInputComponent->BindAction(EnterCombatClickAction, ETriggerEvent::Started, this, &AAtlantisPlayerController::OnLockSlashingPlaneStarted);
		EnhancedInputComponent->BindAction(EnterCombatClickAction, ETriggerEvent::Completed, this, &AAtlantisPlayerController::OnLockSlashingPlaneReleased);

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

/*
void AAtlantisPlayerController::OnEnterCombatTriggered()
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn && ControlledPawn->Implements<UAtlantisCombatInterface>())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		FVector2D MousePosition;
		 
		if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
		{

			IAtlantisCombatInterface::Execute_UpdateCombatGeometery(ControlledPawn);
			FSphere CombatSphere = IAtlantisCombatInterface::Execute_GetCombatSphere(ControlledPawn);
			
			FVector WeaponLocation = IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn);
			const FVector WeaponLinearMomentum = IAtlantisCombatInterface::Execute_GetWeaponLinearMomentum(ControlledPawn);
			const FVector WeaponAngularMomentum = IAtlantisCombatInterface::Execute_GetWeaponAngularMomentum(ControlledPawn);
			//FVector WeaponPositionOnInputplane = UKismetMathLibrary::ProjectPointOnToPlane(WeaponPosition, InputPlane.GetOrigin(), InputPlane.GetNormal());

			FVector WorldMouseLocation, WorldMouseDir;
			if (UGameplayStatics::DeprojectScreenToWorld(this, MousePosition, WorldMouseLocation, WorldMouseDir))
			{
				FVector MouseOnSphereClose, MouseOnSphereFar;
				FVector TargetWeaponLocation = WeaponLocation;
				double T1, T2;
				if (UMathHelperLibrary::LineSphereIntersection(WorldMouseLocation, WorldMouseDir, CombatSphere, MouseOnSphereClose, MouseOnSphereFar, T1, T2))
				{
					bMouseWasIntersectingSphere = true;
					TargetWeaponLocation = FindBestSphereIntersectionAsInput(ControlledPawn, CombatSphere, WeaponLocation, WeaponAngularMomentum, MouseOnSphereClose, MouseOnSphereFar);
					
					if (bDrawDebug) UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetWeaponLocation, 20.f, 12, FLinearColor::Red, 0.f, 1.f);
					//UKismetSystemLibrary::DrawDebugSphere(GetWorld(), MouseOnSphereFar, 20.f, 12, FLinearColor::Green, 0.f, 1.f);
				}
				else
				{
					//Consideration! If mouse goes outside of sphere, should the plane onto which mouse projects be the "slashing plane"?
					//i.e., if sword has momentum, its tangential vector should define a circle on the sphere along which it is already travelling
					//Try this! TODO: When leaving the combat sphere, keep track of last "circle" on which MOUSE INPUT (not sword) was trravelling,
					// and project mouse location on plane that intersects sphere to make such a circle... Project that intersection back onto sphere for new target location!
					//Consideration... What if that plane is coincident with Camera? As in, its a line...? Then just in and out? Since Mouse gets snapped back to combat sphere/Weapon position....
					//What if mouse is allowed to go out, but its sort of yanked back by weapon position?
					//What is mouse is allowed to move "Freely" but based on the mass and momentum of the weapon the cursor gets yanked back, like there's a spring between the cursor and weapon...
					
					//FPlane InputPlane = IAtlantisCombatInterface::Execute_GetInputPlaneFromCamera(ControlledPawn);
					//float T_Unused; //Unsued
					//FVector MouseOnCombatPlane;
					//if (UKismetMathLibrary::LinePlaneIntersection(WorldMouseLocation, WorldMouseLocation + WorldMouseDir * HitResultTraceDistance, InputPlane, T_Unused, MouseOnCombatPlane))
					//{
					//	TargetWeaponLocation = CombatSphere.Center + (MouseOnCombatPlane - CombatSphere.Center).GetUnsafeNormal() * CombatSphere.W;

					//	if (bDrawDebug)
					//	{
					//		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), MouseOnCombatPlane, 20.f, 12, FLinearColor::Yellow, 0.f, 1.f);
					//		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetWeaponLocation, 20.f, 12, FLinearColor::Red, 0.f, 1.f);

					//		UKismetSystemLibrary::DrawDebugCircle(GetWorld(), CombatSphere.Center, CombatSphere.W, 24, FLinearColor::Yellow, 0.f, 2.f, WeaponLinearMomentum.GetSafeNormal(), WeaponAngularMomentum.GetSafeNormal());
					//	}
					//}
					//
					
					if (bMouseWasIntersectingSphere)
					{
						if (!WeaponLinearMomentum.IsNearlyZero())
						{
							MouseOutsideSphereInputPlane = FPlane(CombatSphere.Center, WeaponLocation, WeaponLocation + WeaponLinearMomentum);
						}
						else
						{
							MouseOutsideSphereInputPlane = IAtlantisCombatInterface::Execute_GetInputPlaneFromCamera(ControlledPawn);
						}

						bMouseWasIntersectingSphere = false;
					}
					float T_Unused; //Unsued
					FVector MouseOnCombatPlane;
					if (UKismetMathLibrary::LinePlaneIntersection(WorldMouseLocation, WorldMouseLocation + WorldMouseDir * HitResultTraceDistance, MouseOutsideSphereInputPlane, T_Unused, MouseOnCombatPlane))
					{
						TargetWeaponLocation = CombatSphere.Center + (MouseOnCombatPlane - CombatSphere.Center).GetUnsafeNormal() * CombatSphere.W;

						if (bDrawDebug)
						{
							UKismetSystemLibrary::DrawDebugSphere(GetWorld(), MouseOnCombatPlane, 20.f, 12, FLinearColor::Yellow, 0.f, 1.f);
							UKismetSystemLibrary::DrawDebugSphere(GetWorld(), TargetWeaponLocation, 20.f, 12, FLinearColor::Red, 0.f, 1.f);

							FVector ToMouse = (MouseOnCombatPlane - CombatSphere.Center).GetUnsafeNormal();
							UKismetSystemLibrary::DrawDebugCircle(GetWorld(), CombatSphere.Center, CombatSphere.W, 24, FLinearColor::Yellow, 0.f, 2.f, ToMouse, FVector::CrossProduct(MouseOutsideSphereInputPlane, ToMouse).GetSafeNormal());
						}
					}
				}

				IAtlantisCombatInterface::Execute_HandleCombatInputMouseMotion(ControlledPawn, TargetWeaponLocation, MouseMotion);

				FVector2D UpdatedWeaponLocationOnScreen;
				if (UGameplayStatics::ProjectWorldToScreen(this, IAtlantisCombatInterface::Execute_GetWeaponLocation(ControlledPawn), UpdatedWeaponLocationOnScreen))
				{
					//SetMouseLocation(UpdatedWeaponLocationOnScreen.X, UpdatedWeaponLocationOnScreen.Z);
				}
			}
		}
	}
}*/

void AAtlantisPlayerController::OnLockSlashingPlaneStarted()
{
	bSlashingPlaneIsLocked = true;
}

void AAtlantisPlayerController::OnLockSlashingPlaneReleased()
{
	bSlashingPlaneIsLocked = false;
}

void AAtlantisPlayerController::OnMouseMotionTriggered(const FInputActionInstance& Instance)
{
	// TODO! Two considerations! 1: Mouse Debug vector not contiguous (not each mouse motion is triggering this... problem?)
	//							 2: Should Mouse end be where mouse is (start) + Mouse motion, or is where mouse is the end and start is end - motion? (Make debug arrows last longer)
	//							 3: Mouse positions/mouse motions in viewport coordinates, not pixel coordinates. Also a problem? Scaling up by DPI or whatever might give me contiguous vectors?
	if (!bInCombatMode) return;
	
	MouseMotion = Instance.GetValue().Get<FVector2D>();
	UE_LOG(LogAtlantis, Display, TEXT("Mouse Motion: %s"), *MouseMotion.ToString());
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

FVector AAtlantisPlayerController::FindBestSphereIntersectionAsInput(const APawn* ControlledPawn, const FVector& WeaponPosition, const FVector& WeaponAngularMomentum, const FVector& IntersectionClose, const FVector& IntersectionFar)
{
	const FVector PredictedWeaponLocation = UMathHelperLibrary::ExtrapolateNewPointFromAngularMomentum(CombatSphere.Center, WeaponPosition, WeaponAngularMomentum);
	
	const FVector ToPredictedWeaponLocation = PredictedWeaponLocation - CombatSphere.Center;
	const FVector ToCloseIntersection = IntersectionClose - CombatSphere.Center;
	const FVector ToFarIntersection = IntersectionFar - CombatSphere.Center;
	double CloseAngularDistance = FQuat::FindBetweenVectors(ToPredictedWeaponLocation, ToCloseIntersection).GetAngle();
	double FarAngularDistance = FQuat::FindBetweenVectors(ToPredictedWeaponLocation, ToCloseIntersection).GetAngle();
	if (FarAngularDistance <= CloseAngularDistance)
	{
		return IntersectionFar;
	}
	else
	{
		return IntersectionClose;
	}
}

FVector AAtlantisPlayerController::FindSimpleBestSphereIntersectionAsInput(const APawn* ControlledPawn, const FVector& WeaponPosition, const FVector& IntersectionClose, const FVector& IntersectionFar)
{
	const FVector ToWeapon = WeaponPosition - CombatSphere.Center;
	const FVector ToCloseIntersection = IntersectionClose - CombatSphere.Center;
	const FVector ToFarIntersection = IntersectionFar - CombatSphere.Center;
	double CloseAngularDistance = FQuat::FindBetweenVectors(ToWeapon, ToCloseIntersection).GetAngle();
	double FarAngularDistance = FQuat::FindBetweenVectors(ToWeapon, ToCloseIntersection).GetAngle();
	if (FarAngularDistance <= CloseAngularDistance)
	{
		return IntersectionFar;
	}
	else
	{
		return IntersectionClose;
	}
}
