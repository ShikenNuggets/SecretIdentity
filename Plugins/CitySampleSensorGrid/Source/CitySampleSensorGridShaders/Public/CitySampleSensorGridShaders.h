// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "Math/IntVector.h"
#include "RenderGraphFwd.h"
#include "RHIDefinitions.h"

class FRHIShaderResourceView;
class FRHIUnorderedAccessView;

class CITYSAMPLESENSORGRIDSHADERS_API FCitySampleSensorGridHelper
{
public:
	FCitySampleSensorGridHelper(ERHIFeatureLevel::Type InFeatureLevel, const FUintVector4& InSensorGridDimensions, uint32 FrameIndex);
	~FCitySampleSensorGridHelper();

	struct FResourceSizingInfo
	{
		uint32 SensorCount = 0;
		uint32 OwnerCount = 0;
		uint32 UserChannelCount = 0;
	};

	// c++ mirror of the struct defined in CitySampleSensorGridCommon.ush
	struct alignas(16) FSensorInfo
	{
		FVector Location;
		uint32 DistanceUint;
		FIntVector HitIndex;
		uint32 SearchCount;
	};

	struct FTransientResources
	{
		FTransientResources() = default;

		FTransientResources(const FTransientResources&) = delete;
		FTransientResources& operator=(const FTransientResources&) = delete;

		FTransientResources(FTransientResources&& InResources) = default;
		FTransientResources& operator=(FTransientResources&&) = default;

		CITYSAMPLESENSORGRIDSHADERS_API bool Supports(const FResourceSizingInfo& SizingInfo) const;
		CITYSAMPLESENSORGRIDSHADERS_API void Reset();
		CITYSAMPLESENSORGRIDSHADERS_API void Build(FRDGBuilder& GraphBuilder, const FResourceSizingInfo& SizingInfo);
		CITYSAMPLESENSORGRIDSHADERS_API void ResetTransitions(FRHICommandList& RHICmdList);

		FRDGBufferRef SensorLocations;
		FRDGBufferRef UserChannelData;
		FRDGBufferRef PartialBounds;
		FRDGBufferRef LeafIndices[2];
		FRDGBufferRef MortonCodes[2];
		FRDGBufferRef DuplicateCounts;
		FRDGBufferRef CopyCommands;
		FRDGBufferRef ParentIndices;
		FRDGBufferRef HierarchyGates;
		FRDGBufferRef OwnerBoundingBoxes;
		FRDGBufferRef InternalNodes;
		FRDGBufferRef SensorCounts;
		FRDGBufferRef TraversalResults;

		FRDGBufferSRVRef PreviousSensorInfoSRV;
		FRDGBufferSRVRef PreviousUserChannelDataSRV;

		FResourceSizingInfo SizingInfo;
		bool HasBuffers = false;
	};

	static uint32 GetMaxSensorDensity();
	static uint32 GetMaxOwnerCount();

	void NearestSensors(
		FRDGBuilder& GraphBuilder,
		const FVector2D& GlobalSensorRange,
		FTransientResources& TransientResources);

	void InitBuffers(
		FRDGBuilder& GraphBuilder,
		FTransientResources& TransientResources);

private:
	void ResetResults(
		FRDGBuilder& GraphBuilder,
		FRDGBufferUAVRef ResultsUav);

	void GenerateOwnerBounds(
		FRDGBuilder& GraphBuilder,
		FTransientResources& TransientResources,
		FRDGBufferSRVRef SensorLocationsSrv);

	void GenerateSortedLeaves(
		FRDGBuilder& GraphBuilder,
		FTransientResources& TransientResources,
		FRDGBufferSRVRef SensorLocationsSrv);

	void GenerateBvh(
		FRDGBuilder& GraphBuilder,
		FTransientResources& TransientResources,
		FRDGBufferSRVRef SensorLocationsSrv);

	void RunTraversals(
		FRDGBuilder& GraphBuilder,
		const FVector2D& GlobalSensorRange,
		FTransientResources& TransientResources,
		FRDGBufferSRVRef SensorLocationsSrv,
		FRDGBufferUAVRef ResultsUav);

	const ERHIFeatureLevel::Type FeatureLevel;
	const FUintVector4 SensorGridDimensions;
};
