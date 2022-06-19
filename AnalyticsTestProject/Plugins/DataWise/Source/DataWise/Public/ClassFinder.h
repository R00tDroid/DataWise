#pragma once
#include "HAL/ThreadSafeBool.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ClassFinder.generated.h"


UCLASS()
class DATAWISE_API UClassFinder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static TArray<UClass*> FindSubclasses(UClass* base);

	UFUNCTION(BlueprintCallable)
	static TArray<UClass*> FindSubclassesCPP(UClass* base);

	UFUNCTION(BlueprintCallable)
	static TArray<UClass*> FindSubclassesBP(UClass* base);

private:
	static FThreadSafeBool working;
	static TArray<UClass*> threaded_result;

	static void FindSubclassesBP_internal(UClass* base);
};