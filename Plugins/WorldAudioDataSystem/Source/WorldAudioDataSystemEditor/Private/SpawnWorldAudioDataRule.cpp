// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpawnWorldAudioDataRule.h"
#include "WorldAudioDataClusterActor.h"
#include "WorldAudioDataSettings.h"
#include "Styling/SlateStyle.h"
#include "Brushes/SlateImageBrush.h"
#include "PointCloudView.h"
#include "PointCloudAssetHelpers.h"
#include "Misc/ScopedSlowTask.h"
#include "Brushes/SlateImageBrush.h"
#include "SoundscapeSubsystem.h"
#include "PointCloud.h"
#include "WorldAudioDataSubsystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "WorldPartition/DataLayer/DataLayer.h"
#include "DataLayer/DataLayerEditorSubsystem.h"


#define LOCTEXT_NAMESPACE "RuleProcessorSpawnWorldAudioDataRule"


namespace SpawnWorldAudioDataConstants
{
	const FString Name = FString("Spawn World Audio Data System");
	const FName TemplateActorName(TEXT("TemplateActor"));
	const FString Description = FString("Spawn a World Audio Data Cluster Actor and create a Hash of the Data Points");
}

FSpawnWorldAudioDataRuleData::FSpawnWorldAudioDataRuleData()
{
	NamePattern = "WorldAudioData_$IN_VALUE_$RULEPROCESSOR_ASSET";
	MetaDataKey = "type";
	ReverbKey = "reverb";

	RegisterOverrideableProperty("MetaDataKey");
	RegisterOverrideableProperty("ReverbKey");
	RegisterOverrideableProperty("NamePattern");
	RegisterOverrideableProperty("FolderPath");
	RegisterOverrideableProperty("DataLayers");
}

USpawnWorldAudioDataRule::USpawnWorldAudioDataRule()
{

}

FString USpawnWorldAudioDataRule::Description() const
{
	return SpawnWorldAudioDataConstants::Description;
}

FString USpawnWorldAudioDataRule::RuleName() const
{
	return SpawnWorldAudioDataConstants::Name;
}

FString USpawnWorldAudioDataRule::MakeName(UPointCloud* Pc, const FString& InNamePattern, const FString& InNameValue)
{
	if (Pc == nullptr)
	{
		return FString();
	}

	FString Result = InNamePattern;

	Result.ReplaceInline(TEXT("$IN_VALUE"), *InNameValue);
	Result.ReplaceInline(TEXT("$MANTLE_ASSET"), *Pc->GetName());
	Result.ReplaceInline(TEXT("$RULEPROCESSOR_ASSET"), *Pc->GetName());

	return Result;
}

bool USpawnWorldAudioDataRule::Compile(FSliceAndDiceContext& Context) const
{
	FPointCloudSliceAndDiceRuleReporter Reporter(this, Context);

	if (CompilationTerminated(Context))
	{
		// If compilation is intentionally terminated then the rule should return success 
		// as it is performing as expected
		return true;
	}

	for (FSliceAndDiceContext::FContextInstance& Instance : Context.Instances)
	{
		Instance.FinalizeInstance(MakeShareable(new FSpawnWorldAudioDataRuleInstance(this)));
	}

	return true;
}

void USpawnWorldAudioDataRule::ReportParameters(FSliceAndDiceContext& Context) const
{
	UPointCloudRule::ReportParameters(Context);
	Context.ReportObject.AddParameter(TEXT("MetaData Key"), Data.MetaDataKey);
	Context.ReportObject.AddParameter(TEXT("Reverb Key"), Data.ReverbKey);
	Context.ReportObject.AddParameter(TEXT("Name Pattern"), Data.NamePattern);
	Context.ReportObject.AddParameter(TEXT("Folder Path"), Data.FolderPath.ToString());
	Context.ReportObject.AddMessage(TEXT("Data Layers"));

	for (auto It = Data.DataLayers.CreateConstIterator(); It; ++It)
	{
		if (It)
		{
			Context.ReportObject.AddMessage(It->Name.ToString());
		}
	}
	Context.ReportObject.PushFrame(TEXT("World Audio Data"));

	Context.ReportObject.PopFrame();
}

bool FSpawnWorldAudioDataRuleInstance::Execute(FSliceAndDiceExecutionContextPtr Context)
{
	if (Data.World == nullptr)
	{
		return false;
	}

	const UWorldAudioDataSettings* ProjectSettings = GetDefault<UWorldAudioDataSettings>();

	// Get plugin settings on Subsystem initialization
	if (ProjectSettings == nullptr)
	{
		return false;
	}

	if (ProjectSettings->MantleTypeToSoundscapeColorPointMap.Num() == 0)
	{
		return false;
	}


	if (!GenerateAssets())
	{
		return true;
	}

	// find the value of the given Metadatakey for each point in the view
	// i.e. 
	// 3,"ValueA"
	// 4,"ValueB"
	TMap< int, FString > MetadataValues = GetView()->GetMetadataValues(Data.MetaDataKey);

	TMap< int, FString> ReverbValues = GetView()->GetMetadataValues(Data.ReverbKey);

	// Get all of the transforms and Ids for points in the given view
	TArray<FTransform> Transforms;
	TArray<int32> OutIds;
	GetView()->GetTransformsAndIds(Transforms, OutIds);

	TMap<FString, TArray<FVector>> MetaDataValueVectors;

	TMap<FString, TArray<FVector>> ReverbValueVectors;

	// If there is a mismatch between the ids and the transform, quit
	if (Transforms.Num() != OutIds.Num())
	{
		return false;
	}

	if (Transforms.Num() <= 0)
	{
		return false;
	}

	bool Result = false;

	// iterate over the returned transforms and get the Id of that point and the associated Metadata Value
	for (int i = 0; i < OutIds.Num(); i++)
	{
		const int32 PointId = OutIds[i];
		const FString* MetadataValue = MetadataValues.Find(PointId);
		const FString* ReverbValue = ReverbValues.Find(PointId);

		// do what you need to do with this Id, Transform, Metadata tuple.	
		if (MetadataValue)
		{
			TArray<FVector>& VectorArray = MetaDataValueVectors.FindOrAdd(*MetadataValue);
			VectorArray.Add(Transforms[i].GetLocation());
		}

		if(ReverbValue)
		{
			TArray<FVector>& VectorArray = ReverbValueVectors.FindOrAdd(*ReverbValue);
			VectorArray.Add(Transforms[i].GetLocation());
		}
	}

	// Prepare the target data layers we will push the new actors into
	UDataLayerEditorSubsystem* DataLayerEditorSubsystem = UDataLayerEditorSubsystem::Get();
	TArray<UDataLayerInstance*> DataLayers;

	if (DataLayerEditorSubsystem && Data.DataLayers.Num() > 0)
	{
		for (const FActorDataLayer& DataLayerInfo : Data.DataLayers)
		{
			if (UDataLayerInstance* DataLayer = DataLayerEditorSubsystem->GetDataLayerInstance(DataLayerInfo.Name))
			{
				DataLayers.Emplace(DataLayer);
			}
		}

		if (DataLayers.Num() != Data.DataLayers.Num())
		{
			UE_LOG(PointCloudLog, Log, TEXT("A target data layer wasn't found for the Spawn Blueprint Rule : %s"), *Rule->Label);
		}
	}

	FScopedSlowTask SlowTask(Transforms.Num(), LOCTEXT("CreatingBlueprints", "Creating Blueprints"));
	SlowTask.MakeDialog();

	// work out the name for this actors
	FString Label = USpawnWorldAudioDataRule::MakeName(PointCloud, Data.NamePattern, Data.NameValue);

	// create an actor 
	FBox BoundaryBox = GetView()->GetResultsBoundingBox();

	FVector BoundaryCenter = BoundaryBox.GetCenter();

	// Spawn Actor and Set parameters
	AWorldAudioDataClusterActor* WorldAudioDataClusterActor = GetWorld()->SpawnActor<AWorldAudioDataClusterActor>();
	WorldAudioDataClusterActor->InitializeActor();
	USceneComponent* ClusterActorRootComponent = NewObject<USceneComponent>(WorldAudioDataClusterActor, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
	ClusterActorRootComponent->Mobility = EComponentMobility::Static;
	WorldAudioDataClusterActor->SetRootComponent(ClusterActorRootComponent);
	WorldAudioDataClusterActor->AddInstanceComponent(ClusterActorRootComponent);
	WorldAudioDataClusterActor->SetActorLocation(BoundaryCenter);
	WorldAudioDataClusterActor->SetFolderPath(Data.FolderPath);
	WorldAudioDataClusterActor->SetActorLabel(Label);
	ClusterActorRootComponent->RegisterComponent();


	// Record some statistics
	if (GetStats())
	{
		GetStats()->IncrementCounter(TEXT("Root Component"));
	}

	if (DataLayerEditorSubsystem && DataLayers.Num() > 0)
	{
		if (!DataLayerEditorSubsystem->AddActorToDataLayers(WorldAudioDataClusterActor, DataLayers))
		{
			UE_LOG(PointCloudLog, Log, TEXT("Actor %s was unable to be added to its target data layers"), *WorldAudioDataClusterActor->GetActorLabel());
		}
	}

	if (USoundscapeColorPointHashMapCollection* HashMapCollection = WorldAudioDataClusterActor->SoundscapeColorPointHashMapCollection)
	{
		HashMapCollection->InitializeCollection();

		for (auto It = MetaDataValueVectors.CreateConstIterator(); It; ++It)
		{
			const FGameplayTag* ColorPoint = ProjectSettings->MantleTypeToSoundscapeColorPointMap.Find(It.Key());

			const TArray<FVector> ColorPointLocations = It.Value();

			if (ColorPoint)
			{
				WorldAudioDataClusterActor->SoundscapeColorPointHashMapCollection->AddColorPointArrayToHashMapCollection(ColorPointLocations, *ColorPoint);
			}
		}

	}

	if(UWorldAudioReverbDataCollection* ReverbDataCollection = WorldAudioDataClusterActor->ReverbCollection)
	{
		for(const auto& ReverbValueVector : ReverbValueVectors)
		{
			const float ReverbValue = FCString::Atof(*ReverbValueVector.Key);

			for(const auto& Vector : ReverbValueVector.Value)
			{
				ReverbDataCollection->AddReverbDataPoint(Vector, ReverbValue);
			}
		}
	}

	for(auto MetaDataValueVector : MetaDataValueVectors)
	{
		if(const FSoftObjectPath* ObjectPathPtr = ProjectSettings->ContinuousSoundMap.Find(MetaDataValueVector.Key))
		{
			if(UObject* Object = ObjectPathPtr->TryLoad())
			{
				if(USoundBase* SoundBase = Cast<USoundBase>(Object))
				{
					UContinuousSoundSystemVectorCollection* ContinuousSoundSystemVectorCollection = NewObject<UContinuousSoundSystemVectorCollection>(WorldAudioDataClusterActor);
					ContinuousSoundSystemVectorCollection->PointcloudDataKey = MetaDataValueVector.Key;
					ContinuousSoundSystemVectorCollection->VectorCollection = MetaDataValueVector.Value;
					WorldAudioDataClusterActor->ContinuousSoundSystemVectorCollections.Add(ContinuousSoundSystemVectorCollection);
				}
			}
		}
	}

	Result = true;

#if WITH_EDITOR
	// Data summary generated for Editor time data only
	WorldAudioDataClusterActor->GenerateSummary();
#endif

	NewActorAdded(WorldAudioDataClusterActor, GetView());

	return Result;
}

FSpawnWorldAudioDataFactory::FSpawnWorldAudioDataFactory(TSharedPtr<ISlateStyle> Style)
{
	FSlateStyleSet* AsStyleSet = static_cast<FSlateStyleSet*>(Style.Get());

	if (AsStyleSet)
	{
		Icon = new FSlateImageBrush(AsStyleSet->RootToContentDir(TEXT("Resources/SingleObjectRule"), TEXT(".png")), FVector2D(128.f, 128.f));
		AsStyleSet->Set("RuleThumbnail.SingleObjectRule", Icon);
	}
	else
	{
		Icon = nullptr;
	}
}

FSpawnWorldAudioDataFactory::~FSpawnWorldAudioDataFactory()
{

}

FString FSpawnWorldAudioDataFactory::Name() const
{
	return SpawnWorldAudioDataConstants::Name;
}

FString FSpawnWorldAudioDataFactory::Description() const
{
	return SpawnWorldAudioDataConstants::Description;
}

UPointCloudRule* FSpawnWorldAudioDataFactory::Create(UObject* Parent)
{
	UPointCloudRule* Result = NewObject<USpawnWorldAudioDataRule>(Parent);
	return Result;
}

FSlateBrush* FSpawnWorldAudioDataFactory::GetIcon() const
{
	return Icon;
}

#undef LOCTEXT_NAMESPACE
