// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleSensorGridShaders.h"

#include "Algo/RemoveIf.h"
#include "GPUSort.h"
#include "NiagaraRenderGraphUtils.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderPermutation.h"
#include "ShaderParameterStruct.h"

namespace CitySampleSensorGridShaders
{
	static const uint32 SensorsPerOwnerAlignment = 128;
	static const uint32 MortonCompactionBufferSize = 128;
	static const uint32 MortonCodeBitsReservedForOwner = 5;

	// todo - look at collapsing to 16 bytes (2 x half3 + 2 x int16)
	struct alignas(16) FInternalNode
	{
		FVector BoundsMin;
		int32 LeftChild;

		FVector BoundsMax;
		int32 RightChild;
	};
};

static bool GCitySampleSensorGridBuildDisabled = false;
static FAutoConsoleVariableRef CVarCitySampleSensorGridBuildDisabled(
	TEXT("CitySample.sensorgrid.BuildDisabled"),
	GCitySampleSensorGridBuildDisabled,
	TEXT("When true the sensor grid will not be build or traversed."),
	ECVF_Default
);

class FCitySampleSensorGridResetSensorLocationsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridResetSensorLocationsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridResetSensorLocationsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, SensorsToReset)
		SHADER_PARAMETER(uint32, SensorCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("RESET_SENSOR_LOCATIONS_CS"), 1);
		OutEnvironment.SetDefine(TEXT("RESET_SENSOR_LOCATIONS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridResetSensorLocationsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "ResetSensorLocations", SF_Compute);

class FCitySampleSensorGridClearResultsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridClearResultsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridClearResultsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FSensorInfo>, NearestSensors)
		SHADER_PARAMETER(uint32, SensorCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("CLEAR_RESULTS_CS"), 1);
		OutEnvironment.SetDefine(TEXT("CLEAR_RESULTS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridClearResultsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "ClearNearestSensors", SF_Compute);

class FCitySampleSensorGridBvhPrimeBoundsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhPrimeBoundsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhPrimeBoundsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, PartialBoundingBoxes)
		SHADER_PARAMETER(uint32, SensorCount)
		SHADER_PARAMETER(uint32, PaddedIntermediateCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("PRIME_BOUNDS_GENERATION_CS"), 1);
		OutEnvironment.SetDefine(TEXT("PRIME_BOUNDS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhPrimeBoundsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "PrimeBounds", SF_Compute);

class FCitySampleSensorGridBvhFinalizeBoundsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhFinalizeBoundsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhFinalizeBoundsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SourceBoundingBoxes)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, TargetBoundingBoxes)
		SHADER_PARAMETER(uint32, SourceBoundsCount)
		SHADER_PARAMETER(uint32, PaddedSourceBoundsCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("FINALIZE_BOUNDS_CS"), 1);
		OutEnvironment.SetDefine(TEXT("FINALIZE_BOUNDS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhFinalizeBoundsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "FinalizeBounds", SF_Compute);


class FCitySampleSensorGridBvhMortonCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhMortonCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhMortonCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, BoundingBoxes)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, LeafIndices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, MortonCodes)
		SHADER_PARAMETER(uint32, SensorCount)
		SHADER_PARAMETER(uint32, PaddedOutputCount)
		SHADER_PARAMETER(uint32, OwnerBitCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_GENERATTION_CS"), 1);
		OutEnvironment.SetDefine(TEXT("MORTON_GENERATION_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhMortonCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "MortonGeneration", SF_Compute);

class FCitySampleSensorGridMortonCompactionCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridMortonCompactionCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridMortonCompactionCs, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, InputValues)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutputValues)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, DuplicateCounts)
		SHADER_PARAMETER(uint32, ValueCount)
		SHADER_PARAMETER(uint32, OwnerBitCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_COMPACTION_CS"), 1);
		OutEnvironment.SetDefine(TEXT("MORTON_COMPACTION_CHUNK_SIZE"), CitySampleSensorGridShaders::MortonCompactionBufferSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridMortonCompactionCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "MortonCompaction", SF_Compute);

class FCitySampleSensorGridBuildCopyCommandsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBuildCopyCommandsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBuildCopyCommandsCs, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, DuplicateCounts)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, CompactedValues)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint4>, CopyCommands)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, ElementsPerOwner)
		SHADER_PARAMETER(uint32, OwnerCount)
		SHADER_PARAMETER(uint32, GroupsPerOwner)
		SHADER_PARAMETER(uint32, MaxElementsPerGroup)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_BUILD_COPY_COMMANDS_CS"), 1);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBuildCopyCommandsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "BuildCopyCommands", SF_Compute);

class FCitySampleSensorGridShuffleDataCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridShuffleDataCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridShuffleDataCs, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, InputValues)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint4>, CopyCommands)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutputValues)
		SHADER_PARAMETER(uint32, ValueCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_SHUFFLE_DATA_CS"), 1);
		OutEnvironment.SetDefine(TEXT("MORTON_SHUFFLE_CHUNK_SIZE"), CitySampleSensorGridShaders::MortonCompactionBufferSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridShuffleDataCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "MortonShuffleData", SF_Compute);

class FCitySampleSensorGridBvhGenTopDownCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhGenTopDownCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhGenTopDownCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, LeafCounts)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, LeafIndices)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, MortonCodes)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FInternalNode>, InternalNodes)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, ParentIndices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, AccumulationGates)
		SHADER_PARAMETER(uint32, InternalNodeParentOffset)
		SHADER_PARAMETER(uint32, PaddedLeafNodeCount)
		SHADER_PARAMETER(uint32, PaddedInternalNodeCount)
		SHADER_PARAMETER(uint32, PaddedParentCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("HIERARCHY_GENERATION_TOP_DOWN_CS"), 1);
		OutEnvironment.SetDefine(TEXT("HIERARCHY_GENERATION_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhGenTopDownCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "HierarchyGeneration_TopDown", SF_Compute);

class FCitySampleSensorGridBvhGenBottomUpCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhGenBottomUpCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhGenBottomUpCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, SensorCounts)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, ParentIndices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FInternalNode>, InternalNodes)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, AccumulationGates)
		SHADER_PARAMETER(uint32, InternalNodeParentOffset)
		SHADER_PARAMETER(uint32, MaxSensorsPerOwner)
		SHADER_PARAMETER(uint32, PaddedInternalNodeCount)
		SHADER_PARAMETER(uint32, PaddedParentCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("BOUNDS_GENERATION_BOTTOM_UP_CS"), 1);
		OutEnvironment.SetDefine(TEXT("BOUNDS_GENERATION_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhGenBottomUpCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "BoundsGeneration_BottomUp", SF_Compute);

class FCitySampleSensorGridBvhTraversalCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhTraversalCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhTraversalCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 32;
	static const uint32 SlackSize = 4;
	static const uint32 MinSensorCountLogTwo = 12;
	static const uint32 MaxSensorCountLogTwo = 14;

	class FMaxSensorCountLogTwo : SHADER_PERMUTATION_RANGE_INT("MAX_SENSOR_COUNT_LOG_TWO", MinSensorCountLogTwo, MaxSensorCountLogTwo - MinSensorCountLogTwo + 1);
	using FPermutationDomain = TShaderPermutationDomain<FMaxSensorCountLogTwo>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint>, SensorCounts)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FInternalNodes>, InternalNodes)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FSensorInfo>, NearestSensors)
		SHADER_PARAMETER(float, MaxDistance)
		SHADER_PARAMETER(uint32, MaxSensorsPerOwner)
		SHADER_PARAMETER(uint32, PaddedSensorCount)
		SHADER_PARAMETER(uint32, PaddedInternalNodeCount)
		SHADER_PARAMETER(uint32, OwnerCount)
		SHADER_PARAMETER(uint32, SensorGridFactor)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("BVH_TRAVERSAL_CS"), 1);
		OutEnvironment.SetDefine(TEXT("BVH_TRAVERSAL_CHUNK_SIZE"), ChunkSize);
		OutEnvironment.SetDefine(TEXT("BVH_TRAVERSAL_STACK_SLACK_SIZE"), SlackSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhTraversalCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "BvhTraversal", SF_Compute);

BEGIN_SHADER_PARAMETER_STRUCT(FCitySampleSensorGridSortPassParameters, )
	SHADER_PARAMETER_RDG_BUFFER_SRV_ARRAY(Buffer<uint>, MortonCodeSRVs, [2])
	SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY(RWBuffer<uint>, MortonCodeUAVs, [2])
	SHADER_PARAMETER_RDG_BUFFER_SRV_ARRAY(Buffer<uint>, LeafIndicesSRVs, [2])
	SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY(RWBuffer<uint>, LeafIndicesUAVs, [2])
END_SHADER_PARAMETER_STRUCT()

uint32 FCitySampleSensorGridHelper::GetMaxSensorDensity()
{
	return 1 << (FCitySampleSensorGridBvhTraversalCs::MaxSensorCountLogTwo / 2);
}

uint32 FCitySampleSensorGridHelper::GetMaxOwnerCount()
{
	return (1 << CitySampleSensorGridShaders::MortonCodeBitsReservedForOwner) - 1;
}

FCitySampleSensorGridHelper::FCitySampleSensorGridHelper(ERHIFeatureLevel::Type InFeatureLevel, const FUintVector4& InSensorGridDimensions, uint32 FrameIndex)
	: FeatureLevel(InFeatureLevel)
	, SensorGridDimensions(InSensorGridDimensions)
{
}

FCitySampleSensorGridHelper::~FCitySampleSensorGridHelper()
{
}

bool FCitySampleSensorGridHelper::FTransientResources::Supports(const FResourceSizingInfo& OtherSizingInfo) const
{
	return OtherSizingInfo.SensorCount <= SizingInfo.SensorCount && OtherSizingInfo.OwnerCount <= SizingInfo.OwnerCount;
}

void FCitySampleSensorGridHelper::FTransientResources::Reset()
{
	SensorLocations = nullptr;
	UserChannelData = nullptr;
	PartialBounds = nullptr;
	LeafIndices[0] = nullptr;
	LeafIndices[1] = nullptr;
	MortonCodes[0] = nullptr;
	MortonCodes[1] = nullptr;
	DuplicateCounts = nullptr;
	CopyCommands = nullptr;
	ParentIndices = nullptr;
	HierarchyGates = nullptr;
	OwnerBoundingBoxes = nullptr;
	InternalNodes = nullptr;
	SensorCounts = nullptr;

	HasBuffers = false;
}

void FCitySampleSensorGridHelper::FTransientResources::Build(FRDGBuilder& GraphBuilder, const FResourceSizingInfo& InSizingInfo)
{
	SizingInfo = InSizingInfo;

	if (SizingInfo.SensorCount <= 1)
	{
		Reset();
		return;
	}

	const uint32 InternalNodesPerOwner = SizingInfo.SensorCount - 1;
	const uint32 ParentsPerOwner = SizingInfo.SensorCount + InternalNodesPerOwner;
	const uint32 AlignedSensorsPerOwner = Align(InSizingInfo.SensorCount, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
	const uint32 DuplicateCountBlocksPerOwner = FMath::DivideAndRoundUp(AlignedSensorsPerOwner, CitySampleSensorGridShaders::MortonCompactionBufferSize);
	const uint32 DuplicateCountBlocks = DuplicateCountBlocksPerOwner * SizingInfo.OwnerCount;
	const uint32 IntermediaryBoundsPerOwner = FMath::DivideAndRoundUp(InSizingInfo.SensorCount, FCitySampleSensorGridBvhPrimeBoundsCs::ChunkSize);

	SensorLocations = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), SizingInfo.SensorCount * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridSensorLocations"));

	if (SizingInfo.UserChannelCount > 0)
	{
		UserChannelData = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), SizingInfo.SensorCount * SizingInfo.OwnerCount * SizingInfo.UserChannelCount), TEXT("CitySampleSensorGridUserChannelData"));
	}

	PartialBounds = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), Align(IntermediaryBoundsPerOwner, FCitySampleSensorGridBvhFinalizeBoundsCs::ChunkSize) * SizingInfo.OwnerCount * 2), TEXT("CitySampleSensorGridBvhPartialBounds"));

	for (FRDGBufferRef& LeafIndicesBuff : LeafIndices)
	{
		LeafIndicesBuff = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), AlignedSensorsPerOwner * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridBvhLeafIndicesSorting"));
	}

	for (FRDGBufferRef& MortonCodesBuff : MortonCodes)
	{
		MortonCodesBuff = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), AlignedSensorsPerOwner * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridBvhMortonCodesSorting"));
	}

	DuplicateCounts = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), DuplicateCountBlocks), TEXT("CitySampleSensorGridBvhDuplicateCounts"));

	CopyCommands = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(FUintVector4), DuplicateCountBlocks), TEXT("CitySampleSensorGridBvhCopyCommands"));

	ParentIndices = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Align(ParentsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment) * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridBvhParentIndices"));

	HierarchyGates = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment) * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridBvhAccumGates"));

	// buffer to store the bounds for each of the owners
	OwnerBoundingBoxes = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(FVector4f), SizingInfo.OwnerCount * 2), TEXT("CitySampleSensorGridBvhOwnerBounds"));

	InternalNodes = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(CitySampleSensorGridShaders::FInternalNode), Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment) * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridBvhInternalNodes"));

	SensorCounts = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), SizingInfo.OwnerCount), TEXT("CitySampleSensorGridBvhSensorCounts"));

	TraversalResults = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(FSensorInfo), SizingInfo.SensorCount * SizingInfo.OwnerCount), TEXT("CitySampleSensorGridTraversalResults"));

	HasBuffers = true;
}

void FCitySampleSensorGridHelper::InitBuffers(
	FRDGBuilder& GraphBuilder,
	FTransientResources& TransientResources)
{
	check(TransientResources.HasBuffers);
	FCitySampleSensorGridResetSensorLocationsCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridResetSensorLocationsCs::FParameters>();
	PassParameters->SensorsToReset = GraphBuilder.CreateUAV(TransientResources.SensorLocations, PF_A32B32G32R32F);
	PassParameters->SensorCount = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y) * SensorGridDimensions.Z;

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("CitySampleSensorGrid_InitBuffers"),
		TShaderMapRef<FCitySampleSensorGridResetSensorLocationsCs>(GetGlobalShaderMap(FeatureLevel)),
		PassParameters,
		FIntVector(FMath::DivideAndRoundUp(PassParameters->SensorCount, FCitySampleSensorGridResetSensorLocationsCs::ChunkSize), 1, 1));

	if (TransientResources.SizingInfo.UserChannelCount > 0)
	{
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(TransientResources.UserChannelData, PF_A32B32G32R32F), 0);
	}
}

void FCitySampleSensorGridHelper::ResetResults(
	FRDGBuilder& GraphBuilder,
	FRDGBufferUAVRef ResultsUav)
{
	FCitySampleSensorGridClearResultsCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridClearResultsCs::FParameters>();
	PassParameters->NearestSensors = ResultsUav;
	PassParameters->SensorCount = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y) * SensorGridDimensions.Z;

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("CitySampleSensorGrid_ClearResults"),
		TShaderMapRef<FCitySampleSensorGridClearResultsCs>(GetGlobalShaderMap(FeatureLevel)),
		PassParameters,
		FIntVector(FMath::DivideAndRoundUp(PassParameters->SensorCount, FCitySampleSensorGridResetSensorLocationsCs::ChunkSize), 1, 1));
}

void FCitySampleSensorGridHelper::GenerateOwnerBounds(
	FRDGBuilder& GraphBuilder,
	FTransientResources& TransientResources,
	FRDGBufferSRVRef SensorLocationsSrv)
{
	check(TransientResources.HasBuffers);
	RDG_EVENT_SCOPE(GraphBuilder, "GenerateOwnerBounds");

	const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
	const uint32 IntermediaryBoundsPerOwner = FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhPrimeBoundsCs::ChunkSize);

	// PrimeBounds
	{
		FCitySampleSensorGridBvhPrimeBoundsCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBvhPrimeBoundsCs::FParameters>();
		PassParameters->SensorLocations = SensorLocationsSrv;
		PassParameters->PartialBoundingBoxes = GraphBuilder.CreateUAV(TransientResources.PartialBounds, PF_A32B32G32R32F);
		PassParameters->SensorCount = SensorsPerOwner;
		PassParameters->PaddedIntermediateCount = Align(IntermediaryBoundsPerOwner, FCitySampleSensorGridBvhFinalizeBoundsCs::ChunkSize);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_PrimetLeafBounds"),
			TShaderMapRef<FCitySampleSensorGridBvhPrimeBoundsCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(IntermediaryBoundsPerOwner, SensorGridDimensions.Z, 1));
	}

	// FinalizeBounds
	{
		FCitySampleSensorGridBvhFinalizeBoundsCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBvhFinalizeBoundsCs::FParameters>();
		PassParameters->SourceBoundingBoxes = GraphBuilder.CreateSRV(TransientResources.PartialBounds, PF_A32B32G32R32F);
		PassParameters->TargetBoundingBoxes = GraphBuilder.CreateUAV(TransientResources.OwnerBoundingBoxes, PF_A32B32G32R32F);
		PassParameters->SourceBoundsCount = IntermediaryBoundsPerOwner;
		PassParameters->PaddedSourceBoundsCount = Align(IntermediaryBoundsPerOwner, FCitySampleSensorGridBvhFinalizeBoundsCs::ChunkSize);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_FinalizeLeafBounds"),
			TShaderMapRef<FCitySampleSensorGridBvhFinalizeBoundsCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(SensorGridDimensions.Z, 1, 1));
	}
}

void FCitySampleSensorGridHelper::GenerateSortedLeaves(
	FRDGBuilder& GraphBuilder,
	FTransientResources& TransientResources,
	FRDGBufferSRVRef SensorLocationsSrv)
{
	check(TransientResources.HasBuffers);
	RDG_EVENT_SCOPE(GraphBuilder, "GenerateSortedLeaves");

	const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
	const uint32 InternalNodesPerOwner = SensorsPerOwner - 1;
	const uint32 AlignedSensorsPerOwner = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);

	// for compaction we'll collapse each set of sensors per owner
	check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % CitySampleSensorGridShaders::MortonCompactionBufferSize) == 0);

	const uint32 DuplicateCountBlocksPerOwner = FMath::DivideAndRoundUp(AlignedSensorsPerOwner, CitySampleSensorGridShaders::MortonCompactionBufferSize);
	const uint32 DuplicateCountBlocks = DuplicateCountBlocksPerOwner * SensorGridDimensions.Z;

	// Generate morton codes
	{
		FCitySampleSensorGridBvhMortonCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBvhMortonCs::FParameters>();
		PassParameters->SensorLocations = SensorLocationsSrv;
		PassParameters->BoundingBoxes = GraphBuilder.CreateSRV(TransientResources.OwnerBoundingBoxes, PF_A32B32G32R32F);
		PassParameters->LeafIndices = GraphBuilder.CreateUAV(TransientResources.LeafIndices[1], PF_R32_UINT);
		PassParameters->MortonCodes = GraphBuilder.CreateUAV(TransientResources.MortonCodes[0], PF_R32_UINT);
		PassParameters->SensorCount = SensorsPerOwner;
		PassParameters->PaddedOutputCount = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters->OwnerBitCount = CitySampleSensorGridShaders::MortonCodeBitsReservedForOwner;

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_BvhMorton"),
			TShaderMapRef<FCitySampleSensorGridBvhMortonCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhMortonCs::ChunkSize), SensorGridDimensions.Z, 1));
	}

	// GPUSort!
	{
		FCitySampleSensorGridSortPassParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridSortPassParameters>();
		PassParameters->MortonCodeSRVs[0] = GraphBuilder.CreateSRV(TransientResources.MortonCodes[0], PF_R32_UINT);
		PassParameters->MortonCodeSRVs[1] = GraphBuilder.CreateSRV(TransientResources.MortonCodes[1], PF_R32_UINT);
		PassParameters->MortonCodeUAVs[0] = GraphBuilder.CreateUAV(TransientResources.MortonCodes[0], PF_R32_UINT);
		PassParameters->MortonCodeUAVs[1] = GraphBuilder.CreateUAV(TransientResources.MortonCodes[1], PF_R32_UINT);

		PassParameters->LeafIndicesSRVs[0] = GraphBuilder.CreateSRV(TransientResources.LeafIndices[0], PF_R32_UINT);
		PassParameters->LeafIndicesSRVs[1] = GraphBuilder.CreateSRV(TransientResources.LeafIndices[1], PF_R32_UINT);
		PassParameters->LeafIndicesUAVs[0] = GraphBuilder.CreateUAV(TransientResources.LeafIndices[0], PF_R32_UINT);
		PassParameters->LeafIndicesUAVs[1] = GraphBuilder.CreateUAV(TransientResources.LeafIndices[1], PF_R32_UINT);
		
		const int32 NumTotalSensors = SensorsPerOwner * SensorGridDimensions.Z;

		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(TransientResources.MortonCodes[1], PF_R32_UINT), 0);
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(TransientResources.LeafIndices[0], PF_R32_UINT), 0);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("CitySampleSensorGrid_SortMortonCodes"),
			PassParameters,
			ERDGPassFlags::Compute,
			[
				PassParameters,
				NumTotalSensors,
				FeatureLevel = FeatureLevel
			](FRHIRayTracingCommandList& RHICmdList)
			{
				FGPUSortBuffers SortBuffers;
				SortBuffers.RemoteKeySRVs[0] = PassParameters->MortonCodeSRVs[0]->GetRHI();
				SortBuffers.RemoteKeySRVs[1] = PassParameters->MortonCodeSRVs[1]->GetRHI();
				SortBuffers.RemoteKeyUAVs[0] = PassParameters->MortonCodeUAVs[0]->GetRHI();
				SortBuffers.RemoteKeyUAVs[1] = PassParameters->MortonCodeUAVs[1]->GetRHI();

				SortBuffers.RemoteValueSRVs[0] = PassParameters->LeafIndicesSRVs[1]->GetRHI();
				SortBuffers.RemoteValueSRVs[1] = PassParameters->LeafIndicesSRVs[0]->GetRHI();
				SortBuffers.RemoteValueUAVs[0] = PassParameters->LeafIndicesUAVs[1]->GetRHI();
				SortBuffers.RemoteValueUAVs[1] = PassParameters->LeafIndicesUAVs[0]->GetRHI();

				const int32 BufferIndex = 0;
				const uint32 KeyMask = 0xFFFFFFFF;

				int32 ResultBufferIndex = SortGPUBuffers(RHICmdList, SortBuffers, BufferIndex, KeyMask, NumTotalSensors, FeatureLevel);
				check(ResultBufferIndex == 0);
			}
		);
	}

	// go through the sorted buffer and find (and get rid of) duplicates
	{
		FCitySampleSensorGridMortonCompactionCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridMortonCompactionCs::FParameters>();
		PassParameters->InputValues = GraphBuilder.CreateSRV(TransientResources.MortonCodes[0], PF_R32_UINT);
		PassParameters->OutputValues = GraphBuilder.CreateUAV(TransientResources.MortonCodes[1], PF_R32_UINT);
		PassParameters->DuplicateCounts = GraphBuilder.CreateUAV(TransientResources.DuplicateCounts, PF_R32_UINT);
		PassParameters->ValueCount = SensorsPerOwner * SensorGridDimensions.Z;
		PassParameters->OwnerBitCount = CitySampleSensorGridShaders::MortonCodeBitsReservedForOwner;

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_MortonCompaction"),
			TShaderMapRef<FCitySampleSensorGridMortonCompactionCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(DuplicateCountBlocks, 1, 1));
	}

	{
		FCitySampleSensorGridBuildCopyCommandsCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBuildCopyCommandsCs::FParameters>();
		PassParameters->DuplicateCounts = GraphBuilder.CreateSRV(TransientResources.DuplicateCounts, PF_R32_UINT);
		PassParameters->CompactedValues = GraphBuilder.CreateSRV(TransientResources.MortonCodes[1], PF_R32_UINT);
		PassParameters->CopyCommands = GraphBuilder.CreateUAV(TransientResources.CopyCommands, PF_R32G32B32A32_UINT);
		PassParameters->ElementsPerOwner = GraphBuilder.CreateUAV(TransientResources.SensorCounts, PF_R32_UINT);
		PassParameters->OwnerCount = SensorGridDimensions.Z;
		PassParameters->GroupsPerOwner = DuplicateCountBlocksPerOwner;
		PassParameters->MaxElementsPerGroup = CitySampleSensorGridShaders::MortonCompactionBufferSize;


		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_BuildCopyCommands"),
			TShaderMapRef<FCitySampleSensorGridBuildCopyCommandsCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(SensorGridDimensions.Z, 1, 1));
	}

	{
		TShaderMapRef<FCitySampleSensorGridShuffleDataCs> ShuffleDataShader(GetGlobalShaderMap(FeatureLevel));

		FCitySampleSensorGridShuffleDataCs::FParameters* ShuffleMortonParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridShuffleDataCs::FParameters>();
		ShuffleMortonParameters->InputValues = GraphBuilder.CreateSRV(TransientResources.MortonCodes[1], PF_R32_UINT);
		ShuffleMortonParameters->CopyCommands = GraphBuilder.CreateSRV(TransientResources.CopyCommands, PF_R32G32B32A32_UINT);
		ShuffleMortonParameters->OutputValues = GraphBuilder.CreateUAV(TransientResources.MortonCodes[0], PF_R32_UINT);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_ShuffleMorton"),
			ShuffleDataShader,
			ShuffleMortonParameters,
			FIntVector(DuplicateCountBlocks, 1, 1));

		FCitySampleSensorGridShuffleDataCs::FParameters* ShuffleIndicesParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridShuffleDataCs::FParameters>();
		ShuffleIndicesParameters->InputValues = GraphBuilder.CreateSRV(TransientResources.LeafIndices[1], PF_R32_UINT);
		ShuffleIndicesParameters->CopyCommands = GraphBuilder.CreateSRV(TransientResources.CopyCommands, PF_R32G32B32A32_UINT);
		ShuffleIndicesParameters->OutputValues = GraphBuilder.CreateUAV(TransientResources.LeafIndices[0], PF_R32_UINT);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_ShuffleIndices"),
			ShuffleDataShader,
			ShuffleIndicesParameters,
			FIntVector(DuplicateCountBlocks, 1, 1));
	}
}

void FCitySampleSensorGridHelper::GenerateBvh(
	FRDGBuilder& GraphBuilder,
	FTransientResources& TransientResources,
	FRDGBufferSRVRef SensorLocationsSrv)
{
	check(TransientResources.HasBuffers);
	RDG_EVENT_SCOPE(GraphBuilder, "GenerateBvh");

	const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
	const uint32 InternalNodesPerOwner = SensorsPerOwner - 1;
	const uint32 ParentsPerOwner = SensorsPerOwner + InternalNodesPerOwner;

	// top down pass for generating node relationships
	{
		check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % FCitySampleSensorGridBvhGenTopDownCs::ChunkSize) == 0);

		FCitySampleSensorGridBvhGenTopDownCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBvhGenTopDownCs::FParameters>();
		PassParameters->LeafCounts = GraphBuilder.CreateSRV(TransientResources.SensorCounts, PF_R32_UINT);
		PassParameters->LeafIndices = GraphBuilder.CreateSRV(TransientResources.LeafIndices[0], PF_R32_UINT);
		PassParameters->MortonCodes = GraphBuilder.CreateSRV(TransientResources.MortonCodes[0], PF_R32_UINT);
		PassParameters->InternalNodes = GraphBuilder.CreateUAV(TransientResources.InternalNodes, PF_Unknown);
		PassParameters->ParentIndices = GraphBuilder.CreateUAV(TransientResources.ParentIndices, PF_R32_UINT);
		PassParameters->AccumulationGates = GraphBuilder.CreateUAV(TransientResources.HierarchyGates, PF_R32_UINT);
		PassParameters->InternalNodeParentOffset = SensorsPerOwner;
		PassParameters->PaddedLeafNodeCount = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters->PaddedInternalNodeCount = Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters->PaddedParentCount = Align(ParentsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_BvhGenTopDown"),
			TShaderMapRef<FCitySampleSensorGridBvhGenTopDownCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(FMath::DivideAndRoundUp(InternalNodesPerOwner, FCitySampleSensorGridBvhGenTopDownCs::ChunkSize), SensorGridDimensions.Z, 1));
	}

	// bottom up pass for completing bounds generation
	{
		check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % FCitySampleSensorGridBvhGenBottomUpCs::ChunkSize) == 0);

		FCitySampleSensorGridBvhGenBottomUpCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBvhGenBottomUpCs::FParameters>();
		PassParameters->SensorCounts = GraphBuilder.CreateSRV(TransientResources.SensorCounts, PF_R32_UINT);
		PassParameters->SensorLocations = SensorLocationsSrv;
		PassParameters->ParentIndices = GraphBuilder.CreateSRV(TransientResources.ParentIndices, PF_R32_UINT);
		PassParameters->InternalNodes = GraphBuilder.CreateUAV(TransientResources.InternalNodes, PF_Unknown);
		PassParameters->AccumulationGates = GraphBuilder.CreateUAV(TransientResources.HierarchyGates, PF_R32_UINT);
		PassParameters->InternalNodeParentOffset = SensorsPerOwner;
		PassParameters->MaxSensorsPerOwner = SensorsPerOwner;
		PassParameters->PaddedInternalNodeCount = Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters->PaddedParentCount = Align(ParentsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_BvhGenBottomUp"),
			TShaderMapRef<FCitySampleSensorGridBvhGenBottomUpCs>(GetGlobalShaderMap(FeatureLevel)),
			PassParameters,
			FIntVector(FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhGenBottomUpCs::ChunkSize), SensorGridDimensions.Z, 1));
	}
}

void FCitySampleSensorGridHelper::RunTraversals(
	FRDGBuilder& GraphBuilder,
	const FVector2D& GlobalSensorRange,
	FTransientResources& TransientResources,
	FRDGBufferSRVRef SensorLocationsSrv,
	FRDGBufferUAVRef ResultsUav)
{
	check(TransientResources.HasBuffers);

	{
		const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
		const uint32 SensorsPerOwnerLogTwo = FMath::Max(FCitySampleSensorGridBvhTraversalCs::MinSensorCountLogTwo, FMath::CeilLogTwo(SensorsPerOwner));
		const uint32 InternalNodesPerOwner = SensorsPerOwner - 1;

		if (!ensure(SensorsPerOwnerLogTwo <= FCitySampleSensorGridBvhTraversalCs::MaxSensorCountLogTwo))
		{
			return;
		}

		FCitySampleSensorGridBvhTraversalCs::FParameters* PassParameters = GraphBuilder.AllocParameters<FCitySampleSensorGridBvhTraversalCs::FParameters>();
		PassParameters->SensorCounts = GraphBuilder.CreateSRV(TransientResources.SensorCounts, PF_R32_UINT);
		PassParameters->SensorLocations = SensorLocationsSrv;
		PassParameters->InternalNodes = GraphBuilder.CreateSRV(TransientResources.InternalNodes);
		PassParameters->NearestSensors = ResultsUav;
		PassParameters->MaxDistance = GlobalSensorRange.Y;
		PassParameters->MaxSensorsPerOwner = SensorsPerOwner;
		PassParameters->PaddedSensorCount = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters->PaddedInternalNodeCount = Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters->OwnerCount = SensorGridDimensions.Z;
		PassParameters->SensorGridFactor = SensorGridDimensions.X;

		FCitySampleSensorGridBvhTraversalCs::FPermutationDomain PermutationVector;
		PermutationVector.Set<FCitySampleSensorGridBvhTraversalCs::FMaxSensorCountLogTwo>(SensorsPerOwnerLogTwo);

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("CitySampleSensorGrid_BvhTraversal"),
			TShaderMapRef<FCitySampleSensorGridBvhTraversalCs>(GetGlobalShaderMap(FeatureLevel), PermutationVector),
			PassParameters,
			FIntVector(FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhTraversalCs::ChunkSize), SensorGridDimensions.Z, 1));
	}
}

void FCitySampleSensorGridHelper::NearestSensors(
	FRDGBuilder& GraphBuilder,
	const FVector2D& GlobalSensorRange,
	FTransientResources& TransientResources)
{
	RDG_EVENT_SCOPE(GraphBuilder, "CitySampleSensorGrid_NearestSensors");

	FRDGBufferSRVRef LocationsSRV = GraphBuilder.CreateSRV(TransientResources.SensorLocations, PF_A32B32G32R32F);
	FRDGBufferUAVRef ResultsUAV = GraphBuilder.CreateUAV(TransientResources.TraversalResults, PF_Unknown);

	ResetResults(GraphBuilder, ResultsUAV);

	if (TransientResources.SizingInfo.OwnerCount > 1 && TransientResources.HasBuffers && !GCitySampleSensorGridBuildDisabled)
	{
		GenerateOwnerBounds(GraphBuilder, TransientResources, LocationsSRV);
		GenerateSortedLeaves(GraphBuilder, TransientResources, LocationsSRV);
		GenerateBvh(GraphBuilder, TransientResources, LocationsSRV);
		RunTraversals(GraphBuilder, GlobalSensorRange, TransientResources, LocationsSRV, ResultsUAV);
	}
}
