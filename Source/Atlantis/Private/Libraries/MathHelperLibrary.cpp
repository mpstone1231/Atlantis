// Copyright


#include "Libraries/MathHelperLibrary.h"

FVector UMathHelperLibrary::GetVectorAsLinearCombinationOfBasis(const FVector& InVector, const FVector& U, const FVector& V, const FVector& W)
{
	if (U.IsNearlyZero() || V.IsNearlyZero() || W.IsNearlyZero()) return FVector::ZeroVector;

	FMatrix LinCombMatrix(FPlane(U.X, U.Y, U.Z, 0),
							FPlane(V.X, V.Y, V.Z, 0),
							FPlane(W.X, W.Y, W.Z, 0),
							FPlane(0, 0, 0, 1));


	FVector4 InVector4D = FVector4(InVector.X, InVector.Y, InVector.Z, 0.f);

	FVector4 LinCombResult = LinCombMatrix.Inverse().TransformFVector4(InVector4D); //acts as matrix multiplication

	return FVector(LinCombResult.X, LinCombResult.Y, LinCombResult.Z);
}

bool UMathHelperLibrary::LineSphereIntersection(const FVector& LineStart, const FVector& LineDirection, const FSphere& Sphere, UPARAM(ref)FVector& Intersection1, UPARAM(ref)FVector& Intersection2, UPARAM(ref) double& T1, UPARAM(ref) double& T2)
{
	// As per https://stackoverflow.com/questions/5883169/intersection-between-a-line-and-a-sphere (Thank you Markus Jaderot)
	if (LineDirection.IsNearlyZero()) return false;

//	FVector SecondPointOnLine = LineStart + LineDirection;

	double A = (LineDirection.X * LineDirection.X) + (LineDirection.Y * LineDirection.Y) + (LineDirection.Z * LineDirection.Z);
	double B = 2 * (LineDirection.X * (LineStart.X - Sphere.Center.X) + LineDirection.Y * (LineStart.Y - Sphere.Center.Y) + LineDirection.Z * (LineStart.Z - Sphere.Center.Z));
	double C = (Sphere.Center.X - LineStart.X) * (Sphere.Center.X - LineStart.X)
		+ (Sphere.Center.Y - LineStart.Y) * (Sphere.Center.Y - LineStart.Y)
		+ (Sphere.Center.Z - LineStart.Z) * (Sphere.Center.Z - LineStart.Z)
		- Sphere.W * Sphere.W; 

	double BSquareMinus4AC = (B * B) - (4 * A * C);
	if (BSquareMinus4AC < 0.0) return false; // No intersection!
	
	double RootOfBSquareMinus4AC = FMath::Sqrt(BSquareMinus4AC);
	double T1_Interim = (-B - RootOfBSquareMinus4AC) / (2 * A);
	double T2_Interim = (-B + RootOfBSquareMinus4AC) / (2 * A);

	if (FMath::Abs(T1_Interim) <= FMath::Abs(T2_Interim))
	{
		T1 = T1_Interim;
		T2 = T2_Interim;
	}
	else
	{
		T1 = T2_Interim;
		T2 = T1_Interim;
	}

	Intersection1 = LineStart + T1 * LineDirection;
	Intersection2 = LineStart + T2 * LineDirection;

	return true;
}
