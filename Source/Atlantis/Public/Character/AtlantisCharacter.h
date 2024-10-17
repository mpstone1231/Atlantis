// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/AtlantisCombatInterface.h"

#include "AtlantisCharacter.generated.h"

UCLASS(Blueprintable)
class AAtlantisCharacter : public ACharacter, public IAtlantisCombatInterface
{
	GENERATED_BODY()

public:
	AAtlantisCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE float GetCombatPlaneHeight() const { return CombatPlaneHeight; }

protected:

	/* Combat Interface Beign */
	virtual void EnterCombatMode_Implementation() override;
	virtual void ExitCombatMode_Implementation() override;
	virtual void HandleCombatInputMouseLocation_Implementation(const FVector& MouseLocationOnPlane) override;
	virtual void HandleCombatInputMouseMotion_Implementation(const FVector& TargetWeaponPosition, const FVector2D& MouseMotion) override;
	virtual void UpdateCombatGeometery_Implementation() override;
	virtual FVector GetWeaponRadialAxis_Implementation() override;
	virtual FVector GetWeaponLatitudinalAxis_Implementation() override;
	virtual FVector GetWeaponLocation_Implementation() override;
	virtual FVector GetWeaponAngularMomentum_Implementation() override;
	virtual FPlane GetInputPlaneFromCamera_Implementation() override;
	virtual FPlane GetCombatPlane_Implementation() override;
	virtual FSphere GetCombatSphere_Implementation() override;
	virtual FPlane DetermineCombatSphereTangentialPlane_Implementation() override;
	
	/* Combat Interface End */

	//TODO: Make Weapon a component of special class with a skeletal mesh that holds properties like mass and such
	UPROPERTY(EditDefaultsOnly, Category="Cosmetics|Weapon")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	// Combat
	UFUNCTION()	void UpdateWeaponPosition(const FVector2D& TangentialInput);

	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float MinSwordDistanceFromBody = 25.f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float MaxSwordDistanceFromBody = 100.f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float CombatPlaneHeight = 50.f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float CombatSphereRadius = 200.f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float InputStrength = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Kinematics")	FVector WeaponAngularMomentum;
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Kinematics")	FVector WeaponLinearMomentum;

	/* Debug */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	TObjectPtr<UStaticMeshComponent> DebugWeaponMass;
	FVector WeaponRelativeLocation, WeaponLocation;
	UPROPERTY(EditAnywhere, Category = "Debug")
	float DebugArrowPersistTime = 0.f;
	UPROPERTY(EditAnywhere, Category = "Debug")
	float DebugArrowLength = 10.f;
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebug = false;

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	// Consideration... Should all this exist on the Controller? Weapon axes and such, combat sphere data. Can be updated from controlled pawn data when input processed
	// Fully processed data can passed to player to character
	FPlane CombatPlane;
	FSphere CombatSphere;

	FVector WeaponRadialAxis;
	FVector WeaponLatitudinalAxis;
	FVector WeaponToCombatOrigin;

	void UpdateWeaponTangentialAxes();
	void UpdateWeaponKinematics(const FVector& PreviousWeaponLocation);
	
};

