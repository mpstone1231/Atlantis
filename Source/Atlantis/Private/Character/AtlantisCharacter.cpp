// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/AtlantisCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "../Atlantis.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Libraries/MathHelperLibrary.h"

AAtlantisCharacter::AAtlantisCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Setup weapon
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), FName("weapon_r_socket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Setup debug 
 	DebugWeaponMass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugWeaponMass"));
	DebugWeaponMass->SetupAttachment(GetMesh());
	DebugWeaponMass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponRelativeLocation = -FVector::ForwardVector * CombatSphereRadius;

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AAtlantisCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
	//WeaponLocation = CombatSphere.Center + WeaponRelativeLocation;
	DebugWeaponMass->SetWorldLocation(WeaponLocation);

	if (bDrawDebug)
	{

		FVector CombatSphereCenter = GetActorLocation() + FVector(0.f, 0.f, CombatSphereHeight);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), CombatSphereCenter, CombatSphereCenter + DebugArrowLength * WeaponAngularMomentum, 5.f, FLinearColor::Green, DebugArrowPersistTime, 2.f);
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), WeaponLocation, WeaponLocation + DebugArrowLength * WeaponLinearMomentum, 5.f, FLinearColor::Black, DebugArrowPersistTime, 2.f);
	
		const FVector PredictedWeaponLocation = UMathHelperLibrary::ExtrapolateNewPointFromAngularMomentum(CombatSphereCenter, WeaponLocation, WeaponAngularMomentum);
		UKismetSystemLibrary::DrawDebugSphere(GetWorld(), PredictedWeaponLocation, 20.f, 12, FLinearColor(1.f, 0.f, 1.f), 0.f, 1.f);
	}
}

/***********************************************************************************
********																	********
********				C O M B A T			I N T E R F A C E				********
********																	********
***********************************************************************************/

void AAtlantisCharacter::EnterCombatMode_Implementation()
{
	Weapon->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void AAtlantisCharacter::ExitCombatMode_Implementation()
{
	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("weapon_r_socket"));
}

void AAtlantisCharacter::HandleCombatInputMouseLocation_Implementation(const FVector& MouseLocationOnPlane)
{
	// Geometric Housekeeping
	const FVector ActorLocation = GetActorLocation();
	const FVector ActorLocationOnCombatPlane = FVector(ActorLocation.X, ActorLocation.Y, MouseLocationOnPlane.Z);
	const FVector ActorToMouse = (MouseLocationOnPlane - ActorLocationOnCombatPlane);
	const FVector ActorToMouseDir = ActorToMouse.GetSafeNormal();

	// Handle Position update
	const bool bClipWeaponPositionMax = (ActorToMouse.SquaredLength() > MaxSwordDistanceFromBody * MaxSwordDistanceFromBody);
	const bool bClipWeaponPositionMin = (ActorToMouse.SquaredLength() < MinSwordDistanceFromBody * MinSwordDistanceFromBody);
	FVector NewWeaponLocation = bClipWeaponPositionMax ? (ActorLocationOnCombatPlane + MaxSwordDistanceFromBody * ActorToMouseDir) : 
									bClipWeaponPositionMin ? (ActorLocationOnCombatPlane + MinSwordDistanceFromBody * ActorToMouseDir) : MouseLocationOnPlane;
	Weapon->SetWorldLocation(NewWeaponLocation);

	// Handle Rotation update (hilt points towards player)
	FRotator RotTowardsPlayer = UKismetMathLibrary::MakeRotFromZX(ActorToMouseDir, FVector::CrossProduct(ActorToMouseDir, FVector::UpVector));// (NewWeaponLocation, ActorLocationOnCombatPlane);
	Weapon->SetWorldRotation(RotTowardsPlayer);
}

void AAtlantisCharacter::HandleCombatInputMouseMotion_Implementation(const FVector& TargetWeaponPosition, const FVector2D& MouseMotion)
{
	//TODO: Find out what radial impulse would need to be done to alter momentum such that the weapon ends in the TargetWeaponPosition and apply, keeping track of existing momentum
	FVector PrevWeaponLocation = WeaponLocation;
	WeaponLocation = TargetWeaponPosition;

	// TODO: Store linear AND angular momentum to be retrieved by Controller when deciding next target from intersections
//	UpdateWeaponKinematics(PrevWeaponLocation);

	//UKismetSystemLibrary::DrawDebugArrow(GetWorld(), MouseLocationStart, MouseLocationStart + TangentialPlaneInput, TangentialPlaneInput.Length(), FLinearColor::Red, DebugArrowPersistTime, TangentialPlaneInput.Length() / 2.f);
	
	//Draw Debug arrow on weapon location with input vectors... also, draw axes of tangential/operational plane back in controller?
//	FVector WeaponInput = TangentialPlaneInput.X * WeaponRadialAxis + TangentialPlaneInput.Y * WeaponLatitudinalAxis;
//	UKismetSystemLibrary::DrawDebugArrow(GetWorld(), WeaponLocation, WeaponLocation + 5.f*WeaponInput, 2.f, FLinearColor(1.f, 0.f, 1.f), 0.f, 3.f);

//	UpdateWeaponPosition(TangentialPlaneInput);
//	UKismetSystemLibrary::DrawDebugSphere(GetWorld(), WeaponLocation, 10.f);

}

void AAtlantisCharacter::GetCombatSphereProperties_Implementation(float& OutCombatSphereHeight, float& OutCombatSphereRadius)
{
	OutCombatSphereHeight = CombatSphereHeight;
	OutCombatSphereRadius = CombatSphereRadius;
}

FVector AAtlantisCharacter::GetWeaponLocation_Implementation()
{
	return WeaponLocation;
}

FVector AAtlantisCharacter::GetWeaponLinearMomentum_Implementation()
{
	return WeaponLinearMomentum;
}

FVector AAtlantisCharacter::GetWeaponAngularMomentum_Implementation()
{
	return WeaponAngularMomentum;
}

FVector AAtlantisCharacter::GetCameraFacingDirection_Implementation()
{
	return TopDownCameraComponent->GetForwardVector();
}

/*
void AAtlantisCharacter::UpdateWeaponPosition(const FVector2D& TangentialInput)
{
	// Rotates weapon position on sphere by translating tangential planar input into rotational motion along sphere
	float ArmLength = CombatSphere.W;

	double RadialArcLength = TangentialInput.X * InputStrength;
	double LatitudinalArcLength = TangentialInput.Y * InputStrength;

	double RadialEffectAngle = 2.f * PI * RadialArcLength / ArmLength;
	double LatitudinalEffectAngle = 2.f * PI * LatitudinalArcLength / ArmLength;

	WeaponRelativeLocation = WeaponRelativeLocation.RotateAngleAxisRad(RadialEffectAngle, FVector::UpVector);
	WeaponRelativeLocation = WeaponRelativeLocation.RotateAngleAxisRad(LatitudinalEffectAngle, FVector::CrossProduct(WeaponLatitudinalAxis, WeaponToCombatOrigin)); //WeaponRadialAxis);

	WeaponRelativeLocation *= CombatSphere.W/WeaponRelativeLocation.Length(); //Ensure no drift in length
	
}*/

