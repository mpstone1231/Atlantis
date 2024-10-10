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

}

void AAtlantisPlayerController::OnMouseMotionTriggered(const FInputActionInstance& Instance)
{
	// TODO! Two considerations! 1: Mouse Debug vector not contiguous (not each mouse motion is triggering this... problem?)
	//							 2: Should Mouse end be where mouse is (start) + Mouse motion, or is where mouse is the end and start is end - motion? (Make debug arrows last longer)
	//							 3: Mouse positions/mouse motions in viewport coordinates, not pixel coordinates. Also a problem? Scaling up by DPI or whatever might give me contiguous vectors?
	APawn* ControlledPawn = GetPawn();

	if (ControlledPawn && ControlledPawn->Implements<UAtlantisCombatInterface>())
	{
		FVector2D MouseMotion = Instance.GetValue().Get<FVector2D>();
		UE_LOG(LogAtlantis, Display, TEXT("Mouse: %lf, %lf"), MouseMotion.X, MouseMotion.Y);

		// Collapse all this to a function (getting combat data from mouse motion)
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		bool bHit = false;
		FVector2D MouseStartPosition;
		FVector2D MouseEndPosition;
		
		if (LocalPlayer && LocalPlayer->ViewportClient)
		{
			if (LocalPlayer->ViewportClient->GetMousePosition(MouseStartPosition))
			{
				MouseEndPosition = MouseStartPosition + MouseMotion;

				FVector WorldMouseStart, WorldMouseStartDir;
				FVector WorldMouseEnd, WorldMouseEndDir;
				if (UGameplayStatics::DeprojectScreenToWorld(this, MouseStartPosition, WorldMouseStart, WorldMouseStartDir) && 
					UGameplayStatics::DeprojectScreenToWorld(this, MouseEndPosition, WorldMouseEnd, WorldMouseEndDir))
				{
					FPlane CombatPlane = IAtlantisCombatInterface::Execute_GetCombatPlane(ControlledPawn);
					float T; //Unused
					FVector MouseStartOnCombatPlane, MouseEndOnCombatPlane;
					UKismetMathLibrary::LinePlaneIntersection(WorldMouseStart, WorldMouseStart + WorldMouseStartDir * HitResultTraceDistance, CombatPlane, T, MouseStartOnCombatPlane);
					UKismetMathLibrary::LinePlaneIntersection(WorldMouseEnd, WorldMouseEnd + WorldMouseEndDir * HitResultTraceDistance, CombatPlane, T, MouseEndOnCombatPlane);

					UKismetSystemLibrary::DrawDebugArrow(GetWorld(), MouseStartOnCombatPlane, MouseEndOnCombatPlane, 1.f, FLinearColor::Red, 0.1f, 1.5f);
				}

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
