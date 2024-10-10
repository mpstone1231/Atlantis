// Copyright

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AtlantisCombatInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAtlantisCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ATLANTIS_API IAtlantisCombatInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintNativeEvent) void EnterCombatMode();
	UFUNCTION(BlueprintNativeEvent) void ExitCombatMode();
	UFUNCTION(BlueprintNativeEvent) void HandleCombatInputMouseLocation(const FVector& MouseLocationOnPlane);
	UFUNCTION(BlueprintNativeEvent) void HandleCombatInputMouseMotion(const FVector& MouseMotionOnPlane);
	UFUNCTION(BlueprintNativeEvent) FPlane GetCombatPlane();
};
