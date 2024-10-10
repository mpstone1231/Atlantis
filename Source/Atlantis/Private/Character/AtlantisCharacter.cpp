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
#include "Engine/World.h"
//#include "Components/"

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

	// Invisible Combat Trace plane (for mouse to HitTrace for swinging sword)
	CombatTracePlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CombatTracePlane"));
	CombatTracePlane->SetupAttachment(RootComponent);
	CombatTracePlane->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CombatTracePlane->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CombatTracePlane->SetCollisionResponseToChannel(ECC_CombatTracePlane, ECollisionResponse::ECR_Overlap);
	CombatTracePlane->SetVisibility(false);

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

void AAtlantisCharacter::HandleCombatInputMouseMotion_Implementation(const FVector& MouseLocationOnPlane)
{
}

FPlane AAtlantisCharacter::GetCombatPlane_Implementation()
{
	return FPlane(GetActorLocation() + FVector(0.f, 0.f, CombatPlaneHeight), FVector::UpVector);
}
