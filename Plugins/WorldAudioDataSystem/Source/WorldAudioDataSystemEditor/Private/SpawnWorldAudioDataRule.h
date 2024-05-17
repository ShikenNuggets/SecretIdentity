// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include "PointCloud.h"
#include "PointCloudSliceAndDiceRule.h"
#include "PointCloudSliceAndDiceRuleInstance.h"
#include "PointCloudSliceAndDiceRuleData.h"
#include "PointCloudSliceAndDiceRuleFactory.h"
#include "PointCloudAssetHelpers.h"
#include "WorldPartition/DataLayer/ActorDataLayer.h"

#include "SpawnWorldAudioDataRule.generated.h"

class UPointCloud;
struct FSlateImageBrush;
class ISlateStyle;
class AWorldAudioDataClusterActor;

USTRUCT(BlueprintType)
struct FSpawnWorldAudioDataRuleData : public FPointCloudRuleData
{
	GENERATED_BODY()

	virtual UScriptStruct* GetStruct() const override { return StaticStruct(); }

public:
	FSpawnWorldAudioDataRuleData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	FString MetaDataKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	FString ReverbKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	FString NamePattern;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
	FName FolderPath;

	/** DataLayers the generated actors will belong to.*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = DataLayers)
	TArray<FActorDataLayer> DataLayers;
};

UCLASS(BlueprintType, hidecategories = (Object))
class USpawnWorldAudioDataRule : public UPointCloudRule
{
	GENERATED_BODY()

protected:
	USpawnWorldAudioDataRule();

public:
	/**
	* Make The Name String for the Given Point Cloud By Substituting tokens in the RuleName Template
	* @return A String containing the name of the new actor to create, or empty string on failure
	* @param Pc - A pointer to the point cloud
	* @param InNamePattern - the naming pattern
	* @param InNameValue - the token to replace $IN_VALUE with
	*/
	static FString MakeName(UPointCloud* Pc, const FString& InNamePattern, const FString& InNameValue);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes, meta = (ShowOnlyInnerProperties))
	FSpawnWorldAudioDataRuleData Data;

	//~ Begin UPointCloudRule Interface
public:
	virtual FString Description() const override;
	virtual FString RuleName() const override;
	virtual RuleType GetType() const override { return RuleType::GENERATOR; }
	virtual bool Compile(FSliceAndDiceContext& Context) const override;

protected:
	virtual void ReportParameters(FSliceAndDiceContext& Context)  const override;
	//~ End UPointCloudRule Interface
};

class FSpawnWorldAudioDataRuleInstance : public FPointCloudRuleInstanceWithData<FSpawnWorldAudioDataRuleInstance, FSpawnWorldAudioDataRuleData>
{
public:
	explicit FSpawnWorldAudioDataRuleInstance(const USpawnWorldAudioDataRule* InRule)
		: FPointCloudRuleInstanceWithData(InRule, InRule->Data)
	{
	}

	virtual bool Execute(FSliceAndDiceExecutionContextPtr Context) override;

	virtual bool CanBeExecutedOnAnyThread() const override { return false; }
};

class FSpawnWorldAudioDataFactory : public FSliceAndDiceRuleFactory
{
public:

	FSpawnWorldAudioDataFactory(TSharedPtr<ISlateStyle> Style);
	virtual ~FSpawnWorldAudioDataFactory();

	//~ Begin FSliceAndDiceRuleFactory Interface
public:
	virtual FString Name() const override;
	virtual FString Description() const override;
	virtual FSlateBrush* GetIcon() const override;
	virtual UPointCloudRule::RuleType GetType() const override { return UPointCloudRule::RuleType::GENERATOR; }

protected:
	virtual UPointCloudRule* Create(UObject* Parent) override;
	//~ End FSliceAndDiceRuleFactory Interface

private:
	FSlateImageBrush* Icon;
};
