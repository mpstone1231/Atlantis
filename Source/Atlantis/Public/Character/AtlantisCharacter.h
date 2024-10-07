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

protected:

	/* Combat Interface Beign */
	virtual void EnterCombatMode_Implementation() override;
	virtual void ExitCombatMode_Implementation() override;
	virtual void HandleCombatInput_Implementation(const FVector& MouseLocationOnPlane) override;
	/* Combat Interface End */

	//TODO: Make Weapon a component of special class with a skeletal mesh that holds properties like mass and such
	UPROPERTY(EditDefaultsOnly, Category="Cosmetics|Weapon")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	// Combat
	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float MinSwordDistanceFromBody = 25.f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")	float MaxSwordDistanceFromBody = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)		TObjectPtr<UStaticMeshComponent> CombatTracePlane;

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	
};

