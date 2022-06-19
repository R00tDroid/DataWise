#include "ClassFinder.h"
#include "DataWise.h"
#include "AssetRegistryModule.h"
#include "UObject/UObjectIterator.h"
#include "Async/Async.h"

FThreadSafeBool UClassFinder::working = false;
TArray<UClass*> UClassFinder::threaded_result;


TArray<UClass*> UClassFinder::FindSubclasses(UClass* base)
{
	TArray<UClass*> classes_cpp = FindSubclassesCPP(base);
	TArray<UClass*> classes_bp = FindSubclassesBP(base);

	TArray<UClass*> classes;
	classes.Empty();

	for (int i=0;i<classes_cpp.Num();i++)
	{
		classes.Add(classes_cpp[i]);
	}

	for (int i = 0; i<classes_bp.Num(); i++)
	{
		classes.Add(classes_bp[i]);
	}

	return classes;
}

TArray<UClass*> UClassFinder::FindSubclassesCPP(UClass* base)
{
	TArray<UClass*> classes;
	classes.Empty();

	for (TObjectIterator<UClass> iterator; iterator; ++iterator)
	{
		UClass* found_class = *iterator;
		if (found_class->IsChildOf(base) && !found_class->HasAnyClassFlags(CLASS_Abstract) && found_class->ClassGeneratedBy == nullptr && found_class !=base)
		{
			classes.Add(found_class);
		}
	}

	return classes;
}

TArray<UClass*> UClassFinder::FindSubclassesBP(UClass* base)
{
	if(IsInGameThread())
	{
		FindSubclassesBP_internal(base);
		return threaded_result;
	}
	else 
	{
		working = true;

		AsyncTask(ENamedThreads::GameThread, [base]()
		{
			FindSubclassesBP_internal(base);
			working = false;
		});

		while (working)
		{
			FPlatformProcess::Sleep(.1f);
		}

		return threaded_result;
	}
}


void UClassFinder::FindSubclassesBP_internal(UClass* base)
{
	TArray<UClass*> classes;
	classes.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> ContentPaths;
	ContentPaths.Add(TEXT("/Game"));
	AssetRegistry.ScanPathsSynchronous(ContentPaths);

	FName BaseClassName = base->GetFName();

	FARFilter Filter;
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	TSet<FName> DerivedClass;
	TArray<FName> BaseNames;
	BaseNames.Add(BaseClassName);

	TSet<FName> Excluded;
	AssetRegistry.GetDerivedClassNames(BaseNames, Excluded, DerivedClass);

	for (int32 i = 0; i < AssetList.Num(); i++)
	{
		if (auto GeneratedClassPathPtr = AssetList[i].TagsAndValues.Find(TEXT("GeneratedClass")))
		{
			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(*GeneratedClassPathPtr);
			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

			if (!DerivedClass.Contains(*ClassName))
			{
				continue;
			}

			UObject* object = AssetList[i].GetAsset();

			if (object != nullptr) 
			{
				UBlueprint* blueprint = static_cast<UBlueprint*>(object);

				if (!blueprint->bDeprecate) 
				{
					UClass* Class = blueprint->GeneratedClass;

					if (Class != nullptr)
					{
						classes.Add(Class);
					}
				}
			}
		}
	}

	threaded_result = classes;
}
