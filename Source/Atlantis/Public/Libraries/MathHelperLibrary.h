// Copyright

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MathHelperLibrary.generated.h"


/**
 * 
 */
UCLASS()
class ATLANTIS_API UMathHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Returns the Vector [a,b,c] such that InVector = a*U + b*V + c*W. THe UVW basis need not be orthonormal
	UFUNCTION(BlueprintPure)
	static FVector GetVectorAsLinearCombinationOfBasis(const FVector& InVector, const FVector& U, const FVector& V, const FVector& W);

	// Returns true if the defined line intersects the sphere, passing out two positions on the sphere which the line intersects and the two T values (Intersection = Start + T*Direction). 
	// Returns false for no intersection. Intersection 1 will be the point closest to LineStart.
	UFUNCTION(BlueprintPure)
	static bool LineSphereIntersection(const FVector& LineStart, const FVector& LineDirection, const FSphere& Sphere, UPARAM(ref) FVector& Intersection1 /*Out*/, UPARAM(ref) FVector& Intersection2 /*Out*/, UPARAM(ref) double& T1/*Out*/, UPARAM(ref) double& T2/*Out*/);

	// Using the difference between two points, calculate the angular and linear momentum they are undergoing. Angular Momentum vector
	// Is in direction of rotational axis and magnitude is angular velocity in radians. Follows left hand rule convention.
	UFUNCTION(BlueprintPure)
	static void DetermineAngularAndLinearMomentumBetweenTwoPoints(const FVector& A, const FVector& B, UPARAM(ref) FVector& AngularMomentum, UPARAM(ref) FVector& LinearMomemntum);

	// Given an angular momentum, determine where one point will translate to.
	UFUNCTION(BlueprintPure)
	static FVector ExtrapolateNewPointFromAngularMomentum(const FVector& Origin, const FVector& Point, const FVector& AngularMomentum);
};

