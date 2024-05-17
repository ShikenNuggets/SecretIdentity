// Copyright Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceCitySampleSensorGrid.h"

#include "CitySampleSensorGridShaders.h"
#include "GlobalDistanceFieldParameters.h"
#include "NiagaraComponent.h"
#include "NiagaraGpuComputeDispatchInterface.h"
#include "NiagaraRenderGraphUtils.h"
#include "NiagaraShaderParametersBuilder.h"
#include "NiagaraTypes.h"
#include "NiagaraShader.h"
#include "NiagaraWorldManager.h"
#include "RenderResource.h"
#include "Shader.h"
#include "ShaderCore.h"
#include "ShaderCompilerCore.h"
#include "ShaderParameterUtils.h"

static float GCitySampleSensorGridRadiusOverride = -1.0f;
static FAutoConsoleVariableRef CVarCitySampleSensorGridRadiusOverride(
	TEXT("CitySample.sensorgrid.RadiusOverride"),
	GCitySampleSensorGridRadiusOverride,
	TEXT("When > 0, this will override any CitySampleSensorGrid DI settings"),
	ECVF_Default
);

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceCitySampleSensorGrid"

namespace NDICitySampleSensorGridLocal
{
	BEGIN_SHADER_PARAMETER_STRUCT(FShaderParameters, )
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, RWSensorLocations)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FSensorInfo>, SensorInfo)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float4>, RWSensorGridUserChannelData)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float4>, SensorGridPreviousUserChannelData)

		SHADER_PARAMETER(FUintVector4, ReadSensorGridDimensions)
		SHADER_PARAMETER(FUintVector4, WriteSensorGridDimensions)
		SHADER_PARAMETER(int32, SensorGridWriteIndex)
		SHADER_PARAMETER(int32, SensorGridReadIndex)
		SHADER_PARAMETER(int32, SensorGridUserChannelCount)
	END_SHADER_PARAMETER_STRUCT()

	static const TCHAR* CommonShaderFile = TEXT("/CitySampleSensorGrid/NiagaraDataInterfaceCitySampleSensorGrid.ush");
	static const TCHAR* TemplateShaderFile = TEXT("/CitySampleSensorGrid/NiagaraDataInterfaceCitySampleSensorGridTemplate.ush");

	static const FName UpdateSensorName(TEXT("UpdateSensorGpu"));
	static const FText UpdateSensorDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("UpdateSensorDescription", "Updates the sensor location, should be called each frame"), FText());

	static const FName FindNearestName(TEXT("FindNearestGpu"));
	static const FText FindNearestDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("FindNearestDescription", "Grabs information on the closest sensor (from a different grid) from the previous frame"), FText());

	static const FName ReadUserChannelName(TEXT("ReadSensorUserChannel"));
	static const FText ReadUserChannelDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("ReadUserChannelDescription", "Reads a user data channel from the previous frame"), FText());

	static const FName WriteUserChannelName(TEXT("WriteSensorUserChannel"));
	static const FText WriteUserChannelDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("WriteUserChannelDescription", "Sets a user data channel, should be called each frame"), FText());

	enum NodeVersion : int32
	{
		NodeVersion_Initial = 0,
		Nodeversion_Reworked_Output,
		NodeVersion_Readded_SensorOutput,

		VersionPlusOne,
		NodeVersion_Latest = VersionPlusOne - 1,
	};

	static const FText SensorXDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorXDescription", "X coordinate of the sensor"), FText());
	static const FText SensorYDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorYDescription", "Y coordinate of the sensor"), FText());
	static const FText OwnerIndexDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OwnerIndexDescription", "Identifier of the grid the sensor belongs to"), FText());
	static const FText LocationDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("LocationDescription", "Position in 3D space"), FText());
	static const FText SensorRangeDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorRangeDescription", "Experimental - Per sensor radius (additive to the global search radius)"), FText());
	static const FText SensorValidDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorValidDescription", "Indicates that the sensor is findable from other grids"), FText());
	static const FText UserChannelIndexDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("UserChannelIndexDescription", "Index of the user channel"), FText());
	static const FText UserChannelValueDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("UserChannelValueDescription", "Value of the user channel"), FText());

	static const FText OutputLocationDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputLocationDescription", "Location of the closest sensor from the previous frame"), FText());
	static const FText OutputDistanceDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputDistanceDescription", "Distance to the closest sensor from the previous frame"), FText());
	static const FText OutputIsValidDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputIsValidDescription", "Indicates whether a valid sensor was found the previous frame"), FText());
	static const FText OutputSensorXDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputSensorXDescription", "X coordinate of the closest sensor from the previous frame"), FText());
	static const FText OutputSensorYDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputSensorYdDescription", "Y coordinate of the closest sensor from the previous frame"), FText());
	static const FText OutputOwnerIndexDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputOwnerIndexDescription", "Identifier of the grid associatd with the closest sensor from the previous frame - not guaranteed to be stable"), FText());
	static const FText OutputUserChannelDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputUserChanneldDescription", "Value of the user channel from the previous frame"), FText());

	struct FSensorGridNetworkProxy
	{
		TRefCountPtr<FRDGPooledBuffer> ExtractedSensorInfo;
		TRefCountPtr<FRDGPooledBuffer> ExtractedUserChannelData;

		TUniquePtr<FCitySampleSensorGridHelper::FTransientResources> TransientResources;

		// mapping between the system instance ID and the subnetwork within the buffers
		TMap<FNiagaraSystemInstanceID, int32> InstanceOwnerReadIndexMap;
		TMap<FNiagaraSystemInstanceID, int32> InstanceOwnerWriteIndexMap;

		TSet<FNiagaraSystemInstanceID> RegisteredInstances;

		int32 QueuedOwnerCount = 0;
		int32 AllocatedOwnerCount = 0;
		int32 ResultsOwnerCount = 0;

		// XY is the grid of sensors
		const uint32 SensorGridLayerCount;

		// Number of float4 channels to reserve for each sensor
		const int32 UserChannelCount;

		FSensorGridNetworkProxy() = delete;

		FSensorGridNetworkProxy(uint32 InSensorGridLayerCount, int32 InUserChannelCount)
			: SensorGridLayerCount(InSensorGridLayerCount)
			, UserChannelCount(InUserChannelCount)
		{
			TransientResources = MakeUnique<FCitySampleSensorGridHelper::FTransientResources>();
		}

		void PrepareSimulation(FRDGBuilder& GraphBuilder, ERHIFeatureLevel::Type InFeatureLevel)
		{
			FCitySampleSensorGridHelper::FResourceSizingInfo SizingInfo;
			SizingInfo.SensorCount = (1 << SensorGridLayerCount) * (1 << SensorGridLayerCount);
			SizingInfo.OwnerCount = QueuedOwnerCount;
			SizingInfo.UserChannelCount = UserChannelCount;

			AllocatedOwnerCount = QueuedOwnerCount;
			QueuedOwnerCount = 0;

			TransientResources->Build(GraphBuilder,	SizingInfo);
			if (TransientResources->HasBuffers)
			{
				FCitySampleSensorGridHelper Helper(InFeatureLevel, FUintVector4(SensorGridLayerCount, SensorGridLayerCount, AllocatedOwnerCount, 0), GFrameNumberRenderThread);
				Helper.InitBuffers(GraphBuilder, *TransientResources);
			}
		}

		void EndSimulation(FRDGBuilder& GraphBuilder, ERHIFeatureLevel::Type InFeatureLevel, const FVector2D& GlobalSensorRange)
		{
			FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;

			if (AllocatedOwnerCount)
			{
				const uint32 SensorCountPerSide = 1 << SensorGridLayerCount;

				FVector2D SensorRange = GlobalSensorRange;
				if (GCitySampleSensorGridRadiusOverride > 0.0f)
				{
					SensorRange.Y = GCitySampleSensorGridRadiusOverride;
				}

				if (TransientResources->HasBuffers)
				{
					FCitySampleSensorGridHelper Helper(InFeatureLevel, FUintVector4(SensorGridLayerCount, SensorGridLayerCount, AllocatedOwnerCount, 0), GFrameNumberRenderThread);
					Helper.NearestSensors(GraphBuilder, SensorRange, *TransientResources);

					GraphBuilder.QueueBufferExtraction(TransientResources->TraversalResults, &ExtractedSensorInfo);
					if (UserChannelCount)
					{
						GraphBuilder.QueueBufferExtraction(TransientResources->UserChannelData, &ExtractedUserChannelData);
					}
				}

				InstanceOwnerReadIndexMap = InstanceOwnerWriteIndexMap;
				InstanceOwnerWriteIndexMap.Reset();
			}
		}

	};

	struct FNDIProxy : public FNiagaraDataInterfaceProxy
	{
		FRWLock NetworkLock;
		TMap<const FNiagaraGpuComputeDispatchInterface*, TUniquePtr<FSensorGridNetworkProxy>> NetworkProxies;

		uint32 SensorGridSize = 0;
		FVector2D GlobalSensorRange = FVector2D(EForceInit::ForceInitToZero);
		int32 UserChannelCount = 0;

		int32 PerInstanceDataPassedToRenderThreadSize() const
		{
			return 0;
		}

		FSensorGridNetworkProxy* GetNetwork(const FNiagaraGpuComputeDispatchInterface* Batcher)
		{
			FReadScopeLock _Scope(NetworkLock);
			return NetworkProxies.FindChecked(Batcher).Get();
		}

		void RegisterNetworkInstance(const FNiagaraGpuComputeDispatchInterface* Batcher, FNiagaraSystemInstanceID SystemInstanceID)
		{
			FWriteScopeLock _Scope(NetworkLock);
			TUniquePtr<FSensorGridNetworkProxy>& Network = NetworkProxies.FindOrAdd(Batcher, nullptr);
			if (!Network)
			{
				Network = MakeUnique<FSensorGridNetworkProxy>(SensorGridSize, UserChannelCount);
			}

			Network->RegisteredInstances.Add(SystemInstanceID);
		}

		void UnregisterNetworkInstance(const FNiagaraGpuComputeDispatchInterface* Batcher, FNiagaraSystemInstanceID SystemInstanceID)
		{
			FWriteScopeLock _Scope(NetworkLock);
			if (TUniquePtr<FSensorGridNetworkProxy>* NetworkPtr = NetworkProxies.Find(Batcher))
			{
				if (FSensorGridNetworkProxy* Network = NetworkPtr->Get())
				{
					Network->RegisteredInstances.Remove(SystemInstanceID);
					if (!Network->RegisteredInstances.Num())
					{
						NetworkProxies.Remove(Batcher);
					}
				}
			}
		}

		virtual void PreStage(const FNDIGpuComputePreStageContext& Context) override
		{
			FNiagaraDataInterfaceProxy::PreStage(Context);

			if (FSensorGridNetworkProxy* Network = GetNetwork(&Context.GetComputeDispatchInterface()))
			{
				int32& OwnerIndex = Network->InstanceOwnerWriteIndexMap.FindOrAdd(Context.GetSystemInstanceID(), INDEX_NONE);
				if (OwnerIndex == INDEX_NONE)
				{
					if (ensure(Network->QueuedOwnerCount < ((int32)FCitySampleSensorGridHelper::GetMaxOwnerCount())))
					{
						OwnerIndex = Network->QueuedOwnerCount++;
					}
				}
			}
		}

		virtual bool RequiresPreStageFinalize() const override
		{
			return true;
		}

		virtual void FinalizePreStage(FRDGBuilder& GraphBuilder, const FNiagaraGpuComputeDispatchInterface& ComputeDispatchInterface) override
		{
			FNiagaraDataInterfaceProxy::FinalizePreStage(GraphBuilder, ComputeDispatchInterface);

			if (FSensorGridNetworkProxy* Network = GetNetwork(&ComputeDispatchInterface))
			{
				Network->PrepareSimulation(GraphBuilder, ComputeDispatchInterface.GetFeatureLevel());
			}
		}

		virtual bool RequiresPostStageFinalize() const override
		{
			return true;
		}

		virtual void FinalizePostStage(FRDGBuilder& GraphBuilder, const FNiagaraGpuComputeDispatchInterface& ComputeDispatchInterface) override
		{
			FNiagaraDataInterfaceProxy::FinalizePostStage(GraphBuilder, ComputeDispatchInterface);

			if (FSensorGridNetworkProxy* Network = GetNetwork(&ComputeDispatchInterface))
			{
				Network->EndSimulation(GraphBuilder, ComputeDispatchInterface.GetFeatureLevel(), GlobalSensorRange);
			}
		}

		void RenderThreadInitialize(int32 InSensorCount, const FVector2D& InGlobalSensorRange, int32 InUserChannelCount)
		{
			// SensorCount needs to be a power of two
			SensorGridSize = FMath::Min(FCitySampleSensorGridHelper::GetMaxSensorDensity(), FMath::CeilLogTwo(InSensorCount));
			GlobalSensorRange.X = FMath::Max(0.0f, InGlobalSensorRange.X);
			GlobalSensorRange.Y = FMath::Max(GlobalSensorRange.X, InGlobalSensorRange.Y);
			UserChannelCount = InUserChannelCount;

			NetworkProxies.Reset();
		}
	};
}

UNiagaraDataInterfaceCitySampleSensorGrid::UNiagaraDataInterfaceCitySampleSensorGrid(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
    Proxy.Reset(new NDICitySampleSensorGridLocal::FNDIProxy());
}

void UNiagaraDataInterfaceCitySampleSensorGrid::PostInitProperties()
{
	Super::PostInitProperties();

	//Can we register data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
	}
}

void UNiagaraDataInterfaceCitySampleSensorGrid::PostLoad()
{
	Super::PostLoad();

	if (SensorCountPerSide)
	{
		MarkRenderDataDirty();
	}
}

#if WITH_EDITORONLY_DATA
void UNiagaraDataInterfaceCitySampleSensorGrid::GetFunctionsInternal(TArray<FNiagaraFunctionSignature>& OutFunctions) const
{
	{
		FNiagaraFunctionSignature SigUpdateSensor;
		SigUpdateSensor.Name = NDICitySampleSensorGridLocal::UpdateSensorName;
		SigUpdateSensor.bRequiresExecPin = true;
		SigUpdateSensor.bMemberFunction = true;
		SigUpdateSensor.bRequiresContext = false;
		SigUpdateSensor.bSupportsCPU = false;
		SigUpdateSensor.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigUpdateSensor.Description = NDICitySampleSensorGridLocal::UpdateSensorDescription;

		SigUpdateSensor.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Location")), NDICitySampleSensorGridLocal::LocationDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("SensorRange")), NDICitySampleSensorGridLocal::SensorRangeDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("IsValid")), NDICitySampleSensorGridLocal::SensorValidDescription);
		OutFunctions.Add(SigUpdateSensor);
	}

	{
		FNiagaraFunctionSignature SigFindNearest;
		SigFindNearest.Name = NDICitySampleSensorGridLocal::FindNearestName;
		SigFindNearest.bMemberFunction = true;
		SigFindNearest.bRequiresContext = false;
		SigFindNearest.bSupportsCPU = false;
		SigFindNearest.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigFindNearest.Description = NDICitySampleSensorGridLocal::FindNearestDescription;

		SigFindNearest.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigFindNearest.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigFindNearest.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Out_Location")), NDICitySampleSensorGridLocal::OutputLocationDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Out_Distance")), NDICitySampleSensorGridLocal::OutputDistanceDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorX")),
			NDICitySampleSensorGridLocal::OutputSensorXDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorY")),
			NDICitySampleSensorGridLocal::OutputSensorYDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_OwnerIndex")),
			NDICitySampleSensorGridLocal::OutputOwnerIndexDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Out_IsValid")), NDICitySampleSensorGridLocal::OutputIsValidDescription);
		OutFunctions.Add(SigFindNearest);
	}

	{
		FNiagaraFunctionSignature SigReadUserData;
		SigReadUserData.Name = NDICitySampleSensorGridLocal::ReadUserChannelName;
		SigReadUserData.bMemberFunction = true;
		SigReadUserData.bRequiresContext = false;
		SigReadUserData.bSupportsCPU = false;
		SigReadUserData.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigReadUserData.Description = NDICitySampleSensorGridLocal::ReadUserChannelDescription;

		SigReadUserData.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("OwnerIndex")),
			NDICitySampleSensorGridLocal::OwnerIndexDescription);
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Channel")), NDICitySampleSensorGridLocal::UserChannelIndexDescription);
		SigReadUserData.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Out_UserData")), NDICitySampleSensorGridLocal::OutputUserChannelDescription);
		SigReadUserData.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Out_IsValid")), NDICitySampleSensorGridLocal::OutputIsValidDescription);
		OutFunctions.Add(SigReadUserData);
	}

	{
		FNiagaraFunctionSignature SigWriteUserData;
		SigWriteUserData.Name = NDICitySampleSensorGridLocal::WriteUserChannelName;
		SigWriteUserData.bRequiresExecPin = true;
		SigWriteUserData.bMemberFunction = true;
		SigWriteUserData.bRequiresContext = false;
		SigWriteUserData.bSupportsCPU = false;
		SigWriteUserData.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigWriteUserData.Description = NDICitySampleSensorGridLocal::WriteUserChannelDescription;

		SigWriteUserData.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Channel")), NDICitySampleSensorGridLocal::UserChannelIndexDescription);
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Value")), NDICitySampleSensorGridLocal::UserChannelValueDescription);
		OutFunctions.Add(SigWriteUserData);
	}
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
	if ( (FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::UpdateSensorName) ||
		(FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::FindNearestName) ||
		(FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::ReadUserChannelName) ||
		(FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::WriteUserChannelName))
	{
		return true;
	}

	return false;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	TMap<FString, FStringFormatArg> TemplateArgs =
	{
		{TEXT("ParameterName"),	ParamInfo.DataInterfaceHLSLSymbol},
	};

	AppendTemplateHLSL(OutHLSL, NDICitySampleSensorGridLocal::TemplateShaderFile, TemplateArgs);
}

void UNiagaraDataInterfaceCitySampleSensorGrid::GetCommonHLSL(FString& OutHlsl)
{
	OutHlsl.Appendf(TEXT("#include \"%s\"\n"), NDICitySampleSensorGridLocal::CommonShaderFile);
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const
{
	bool bSuccess = Super::AppendCompileHash(InVisitor);

	bSuccess &= InVisitor->UpdateShaderFile(NDICitySampleSensorGridLocal::CommonShaderFile);
	bSuccess &= InVisitor->UpdateShaderFile(NDICitySampleSensorGridLocal::TemplateShaderFile);
	bSuccess &= InVisitor->UpdateShaderParameters<NDICitySampleSensorGridLocal::FShaderParameters>();

	return bSuccess;
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::UpgradeFunctionCall(FNiagaraFunctionSignature& FunctionSignature)
{
	bool WasChanged = false;

	if (FunctionSignature.Name == NDICitySampleSensorGridLocal::UpdateSensorName && FunctionSignature.FunctionVersion == NDICitySampleSensorGridLocal::NodeVersion::NodeVersion_Initial)
	{
		FunctionSignature.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("IsValid")), NDICitySampleSensorGridLocal::SensorValidDescription);
		FunctionSignature.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;

		WasChanged = true;
	}

	if (FunctionSignature.Name == NDICitySampleSensorGridLocal::FindNearestName)
	{
		if (FunctionSignature.FunctionVersion == NDICitySampleSensorGridLocal::NodeVersion::NodeVersion_Initial
			|| FunctionSignature.FunctionVersion == NDICitySampleSensorGridLocal::NodeVersion::Nodeversion_Reworked_Output)
		{
			FunctionSignature.Outputs.Reset();
			FunctionSignature.OutputDescriptions.Reset();

			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Out_Location")), NDICitySampleSensorGridLocal::OutputLocationDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Out_Distance")), NDICitySampleSensorGridLocal::OutputDistanceDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorX")),
			NDICitySampleSensorGridLocal::OutputSensorXDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorY")),
			NDICitySampleSensorGridLocal::OutputSensorYDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_OwnerIndex")),
			NDICitySampleSensorGridLocal::OutputOwnerIndexDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Out_IsValid")), NDICitySampleSensorGridLocal::OutputIsValidDescription);
			FunctionSignature.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;

			WasChanged = true;
		}
	}

	return WasChanged;
}

#endif

int32 UNiagaraDataInterfaceCitySampleSensorGrid::PerInstanceDataSize() const
{
	return 4;
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	if (SystemInstance)
	{
		NDICitySampleSensorGridLocal::FNDIProxy* RT_Proxy = GetProxyAs<NDICitySampleSensorGridLocal::FNDIProxy>();

		// Push Updates to Proxy
		ENQUEUE_RENDER_COMMAND(FRegisterInstance)(
			[RT_Proxy, RT_Batcher = SystemInstance->GetComputeDispatchInterface(), RT_InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& RHICmdList)
		{
			RT_Proxy->RegisterNetworkInstance(RT_Batcher, RT_InstanceID);
		});
	}

	return true;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	if (SystemInstance)
	{
		NDICitySampleSensorGridLocal::FNDIProxy* RT_Proxy = GetProxyAs<NDICitySampleSensorGridLocal::FNDIProxy>();

		// Push Updates to Proxy
		ENQUEUE_RENDER_COMMAND(FUnregisterInstance)(
			[RT_Proxy, RT_Batcher = SystemInstance->GetComputeDispatchInterface(), RT_InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& RHICmdList)
		{
			RT_Proxy->UnregisterNetworkInstance(RT_Batcher, RT_InstanceID);
		});
	}
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}

	const UNiagaraDataInterfaceCitySampleSensorGrid* OtherTyped = CastChecked<const UNiagaraDataInterfaceCitySampleSensorGrid>(Other);
	return OtherTyped->SensorCountPerSide == SensorCountPerSide
		&& OtherTyped->GlobalSensorAccuracy == GlobalSensorAccuracy
		&& OtherTyped->GlobalSensorRange == GlobalSensorRange
		&& OtherTyped->UserChannelCount == UserChannelCount;
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceCitySampleSensorGrid* OtherTyped = CastChecked<UNiagaraDataInterfaceCitySampleSensorGrid>(Destination);
	OtherTyped->SensorCountPerSide = SensorCountPerSide;
	OtherTyped->GlobalSensorAccuracy = GlobalSensorAccuracy;
	OtherTyped->GlobalSensorRange = GlobalSensorRange;
	OtherTyped->UserChannelCount = UserChannelCount;
	OtherTyped->MarkRenderDataDirty();
	return true;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::PushToRenderThreadImpl()
{
	NDICitySampleSensorGridLocal::FNDIProxy* RT_Proxy = GetProxyAs<NDICitySampleSensorGridLocal::FNDIProxy>();

	// Push Updates to Proxy, first release any resources
	ENQUEUE_RENDER_COMMAND(FUpdateDI)(
		[RT_Proxy,
		RT_SensorCount = SensorCountPerSide,
		RT_GlobalSensorRange = FVector2D(GlobalSensorAccuracy, GlobalSensorRange),
		RT_UserChannelCount = UserChannelCount](FRHICommandListImmediate& RHICmdList)
		{
			RT_Proxy->RenderThreadInitialize(RT_SensorCount, RT_GlobalSensorRange, RT_UserChannelCount);
		});
}

void UNiagaraDataInterfaceCitySampleSensorGrid::BuildShaderParameters(FNiagaraShaderParametersBuilder& ShaderParametersBuilder) const
{
	ShaderParametersBuilder.AddNestedStruct<NDICitySampleSensorGridLocal::FShaderParameters>();
}

void UNiagaraDataInterfaceCitySampleSensorGrid::SetShaderParameters(const FNiagaraDataInterfaceSetShaderParametersContext& Context) const
{
	using namespace NDICitySampleSensorGridLocal;

	FRDGBuilder& GraphBuilder = Context.GetGraphBuilder();
	FNDIProxy& DataInterfaceProxy = Context.GetProxy<FNDIProxy>();
	FSensorGridNetworkProxy* NetworkProxy = DataInterfaceProxy.GetNetwork(&Context.GetComputeDispatchInterface());
	FShaderParameters* ShaderParameters = Context.GetParameterNestedStruct<FShaderParameters>();

	// access to the shared buffers of sensor locations & user channel data should be localized for each instance of the sensor simulation
	// and so we enable UAV overlap
	const bool bEnableUAVOverlap = true;
	const ERDGUnorderedAccessViewFlags UAVFlags = bEnableUAVOverlap ? ERDGUnorderedAccessViewFlags::SkipBarrier : ERDGUnorderedAccessViewFlags::None;

	if (Context.IsResourceBound(&ShaderParameters->RWSensorLocations))
	{
		ShaderParameters->RWSensorLocations = NetworkProxy->TransientResources->SensorLocations
			? GraphBuilder.CreateUAV(NetworkProxy->TransientResources->SensorLocations, PF_A32B32G32R32F, UAVFlags)
			: Context.GetComputeDispatchInterface().GetEmptyBufferUAV(GraphBuilder, PF_A32B32G32R32F);
	}

	if (Context.IsResourceBound(&ShaderParameters->SensorInfo))
	{
		if (NetworkProxy->ExtractedSensorInfo)
		{
			ShaderParameters->SensorInfo = GraphBuilder.CreateSRV(GraphBuilder.RegisterExternalBuffer(NetworkProxy->ExtractedSensorInfo));
		}
		else
		{
			FCitySampleSensorGridHelper::FSensorInfo DummySensorInfoData;
			FRDGBufferRef DummySensorInfoBuffer = CreateStructuredBuffer<FCitySampleSensorGridHelper::FSensorInfo>(GraphBuilder, TEXT("CitySampleSensorGridDummySensorInfo"), { DummySensorInfoData });
			ShaderParameters->SensorInfo = GraphBuilder.CreateSRV(DummySensorInfoBuffer);
		}
	}

	if (Context.IsResourceBound(&ShaderParameters->RWSensorGridUserChannelData))
	{
		ShaderParameters->RWSensorGridUserChannelData = NetworkProxy->TransientResources->UserChannelData
			? GraphBuilder.CreateUAV(NetworkProxy->TransientResources->UserChannelData, PF_A32B32G32R32F, UAVFlags)
			: Context.GetComputeDispatchInterface().GetEmptyBufferUAV(GraphBuilder, PF_A32B32G32R32F);
	}

	if (Context.IsResourceBound(&ShaderParameters->SensorGridPreviousUserChannelData))
	{
		ShaderParameters->SensorGridPreviousUserChannelData = NetworkProxy->ExtractedUserChannelData
			? GraphBuilder.CreateSRV(GraphBuilder.RegisterExternalBuffer(NetworkProxy->ExtractedUserChannelData))
			: Context.GetComputeDispatchInterface().GetEmptyBufferSRV(GraphBuilder, PF_A32B32G32R32F);
	}

	const FUintVector4 SensorGridDimensions(NetworkProxy->SensorGridLayerCount, NetworkProxy->SensorGridLayerCount, NetworkProxy->AllocatedOwnerCount, 1);

	// we tag the read dimensions as being invalid (w component being 0) so that ee don't even bother reading from the empty srv
	ShaderParameters->ReadSensorGridDimensions = NetworkProxy->ExtractedSensorInfo ? SensorGridDimensions : FUintVector4(0, 0, 0, 0);
	ShaderParameters->WriteSensorGridDimensions = NetworkProxy->TransientResources->HasBuffers ? SensorGridDimensions : FUintVector4(0, 0, 0, 0);

	const int32* WriteIndexPtr = NetworkProxy->InstanceOwnerWriteIndexMap.Find(Context.GetSystemInstanceID());
	ShaderParameters->SensorGridWriteIndex = WriteIndexPtr ? *WriteIndexPtr : INDEX_NONE;

	const int32* ReadIndexPtr = NetworkProxy->InstanceOwnerReadIndexMap.Find(Context.GetSystemInstanceID());
	ShaderParameters->SensorGridReadIndex = ReadIndexPtr ? *ReadIndexPtr : INDEX_NONE;

	ShaderParameters->SensorGridUserChannelCount = NetworkProxy->UserChannelCount;
}

#if WITH_EDITOR
void UNiagaraDataInterfaceCitySampleSensorGrid::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		(PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, SensorCountPerSide)
		|| PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, GlobalSensorAccuracy)
		|| PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, GlobalSensorRange)
		|| PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, UserChannelCount)))
	{
		MarkRenderDataDirty();
	}
}
#endif

#undef LOCTEXT_NAMESPACE
