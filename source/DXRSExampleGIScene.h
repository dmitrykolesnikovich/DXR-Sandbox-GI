﻿#pragma once

#include "DXRSGraphics.h"
#include "DXRSTimer.h"
#include "DXRSModel.h"
#include "DXRSRenderTarget.h"
#include "DXRSDepthBuffer.h"
#include "DXRSBuffer.h"
#include "DXRSCamera.h"

#include "RootSignature.h"
#include "PipelineStateObject.h"

#include "RaytracingPipelineGenerator.h"
#include "ShaderBindingTableGenerator.h"

#define SHADOWMAP_SIZE 2048
#define RSM_SIZE 2048
#define RSM_SAMPLES_COUNT 512
#define LPV_DIM 32
#define VCT_SCENE_VOLUME_SIZE 256
#define VCT_MIPS 6
#define LOCKED_CAMERA_VIEWS 3

class DXRSExampleGIScene
{
	enum RenderQueue {
		GRAPHICS_QUEUE,
		COMPUTE_QUEUE
	};

public:
	DXRSExampleGIScene();
	~DXRSExampleGIScene();

	void Init(HWND window, int width, int height);
	void Clear(ID3D12GraphicsCommandList* cmdList);
	void Run();
	void OnWindowSizeChanged(int width, int height);

private:
	void Update(DXRSTimer const& timer);
	void UpdateTransforms(DXRSTimer const& timer);
	void UpdateBuffers(DXRSTimer const& timer);
	void UpdateLights(DXRSTimer const& timer);
	void UpdateCamera(DXRSTimer const& timer);
	void UpdateImGui();
	
	void InitGbuffer(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void InitShadowMapping(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void InitReflectiveShadowMapping(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void InitLightPropagationVolume(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void InitVoxelConeTracing(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void InitLighting(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void InitComposite(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);

	void RenderSync();
	void RenderAsync();

	void RenderGbuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap);
	void RenderShadowMapping(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap);
	void RenderReflectiveShadowMapping(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap, RenderQueue aQueue = GRAPHICS_QUEUE, bool useAsyncCompute = false);
	void RenderLightPropagationVolume(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap, RenderQueue aQueue = GRAPHICS_QUEUE, bool useAsyncCompute = false);
	void RenderVoxelConeTracing(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap, RenderQueue aQueue = GRAPHICS_QUEUE, bool useAsyncCompute = false);
	void RenderLighting(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap);
	void RenderComposite(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap);
	void RenderObject(U_PTR<DXRSModel>& aModel, std::function<void(U_PTR<DXRSModel>&)> aCallback);
	void RenderReflectionsDXR(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap);

	void InitReflectionsDXR(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager);
	void CreateRaytracingPSO();
	void CreateRaytracingAccelerationStructures(bool toUpdateTLAS = false);
	void CreateRaytracingShaders();
	void CreateRaytracingShaderTable();
	void CreateRaytracingResourceHeap();

	void ThrowFailedErrorBlob(ID3DBlob* blob);

	DXRSGraphics* mSandboxFramework;

	U_PTR<GamePad>                                       mGamePad;
	U_PTR<Keyboard>                                      mKeyboard;
	U_PTR<Mouse>                                         mMouse;
	U_PTR<DXRSCamera>                                    mCamera;
	DirectX::GamePad::ButtonStateTracker                 mGamePadButtons;
	DirectX::Keyboard::KeyboardStateTracker              mKeyboardButtons;

	DXRSTimer                                            mTimer;

	std::vector<CD3DX12_RESOURCE_BARRIER>                mBarriers;

	U_PTR<GraphicsMemory>                                mGraphicsMemory;
	U_PTR<CommonStates>                                  mStates;

	std::vector<U_PTR<DXRSModel>>						 mObjects;

	// Gbuffer
	RootSignature                                        mGbufferRS;
	std::vector<DXRSRenderTarget*>                       mGbufferRTs;
	GraphicsPSO                                          mGbufferPSO;
	DXRSBuffer* mGbufferCB;
	DXRSDepthBuffer* mDepthStencil;
	DXRS::DescriptorHandle                               mNullDescriptor;

	__declspec(align(16)) struct GBufferCBData
	{
		XMMATRIX ViewProjection;
		XMMATRIX InvViewProjection;
		XMFLOAT4 CameraPos;
		XMFLOAT4 ScreenSize;
		XMFLOAT4 LightColor;
	};

	// RSM
	RootSignature                                        mRSMRS;
	RootSignature                                        mRSMRS_Compute;
	RootSignature                                        mRSMBuffersRS;
	RootSignature                                        mRSMUpsampleAndBlurRS;
	RootSignature                                        mRSMDownsampleRS;
	RootSignature                                        mRSMDownsampleRS_Compute;
	DXRSRenderTarget*			                         mRSMRT;
	std::vector<DXRSRenderTarget*>                       mRSMBuffersRTs;
	std::vector<DXRSRenderTarget*>                       mRSMBuffersRTs_CopiesForAsync;
	std::vector<DXRSRenderTarget*>                       mRSMDownsampledBuffersRTs;
	DXRSRenderTarget*			                         mRSMUpsampleAndBlurRT;
	GraphicsPSO                                          mRSMPSO;
	ComputePSO                                           mRSMPSO_Compute;
	GraphicsPSO											 mRSMBuffersPSO;
	GraphicsPSO											 mRSMDownsamplePSO;
	ComputePSO											 mRSMDownsamplePSO_Compute;
	ComputePSO											 mRSMUpsampleAndBlurPSO;
	DXRSBuffer* mRSMCB;
	DXRSBuffer* mRSMCB2;
	DXRSBuffer* mRSMDownsampleCB;

	__declspec(align(16)) struct RSMCBData
	{
		XMMATRIX ShadowViewProjection;
		float RSMIntensity;
		float RSMRMax;
		XMFLOAT2 UpsampleRatio;
	};

	__declspec(align(16)) struct RSMCBDataRandomValues
	{
		XMFLOAT4 xi[RSM_SAMPLES_COUNT];
	};

	__declspec(align(16)) struct RSMCBDataDownsample
	{
		XMFLOAT4 LightDir;
		int ScaleSize;
	};

	// LPV
	RootSignature                                        mLPVInjectionRS;
	RootSignature                                        mLPVPropagationRS;
	GraphicsPSO											 mLPVInjectionPSO;
	GraphicsPSO											 mLPVPropagationPSO;
	std::vector<DXRSRenderTarget*>                       mLPVSHColorsRTs;
	std::vector<DXRSRenderTarget*>                       mLPVAccumulationSHColorsRTs;
	__declspec(align(16)) struct LPVCBData
	{
		XMMATRIX worldToLPV;
		float LPVCutoff;
		float LPVPower;
		float LPVAttenuation;
	};
	DXRSBuffer* mLPVCB;

	//we will store a pair of bundles due to double frame GPU descriptor heap
	ComPtr<ID3D12CommandAllocator>						 mLPVPropagationBundle1Allocator;
	ComPtr<ID3D12CommandAllocator>						 mLPVPropagationBundle2Allocator;
	ComPtr<ID3D12GraphicsCommandList>					 mLPVPropagationBundle1;
	ComPtr<ID3D12GraphicsCommandList>					 mLPVPropagationBundle2;
	DXRS::GPUDescriptorHeap*							 mLPVPropagationBundle1UsedGPUHeap;
	DXRS::GPUDescriptorHeap*							 mLPVPropagationBundle2UsedGPUHeap;
	bool mUseBundleForLPVPropagation = false;
	bool mLPVPropagationBundlesClosed = false;
	bool mLPVPropagationBundle1Closed = false;
	bool mLPVPropagationBundle2Closed = false;

	RootSignature                                        mVCTVoxelizationRS;
	RootSignature                                        mVCTMainRS;
	RootSignature                                        mVCTMainRS_Compute;
	RootSignature                                        mVCTMainUpsampleAndBlurRS;
	RootSignature                                        mVCTAnisoMipmappingPrepareRS;
	RootSignature                                        mVCTAnisoMipmappingMainRS;
	GraphicsPSO											 mVCTVoxelizationPSO;
	GraphicsPSO											 mVCTMainPSO;
	ComputePSO											 mVCTMainPSO_Compute;
	ComputePSO											 mVCTAnisoMipmappingPreparePSO;
	ComputePSO											 mVCTAnisoMipmappingMainPSO;
	ComputePSO											 mVCTMainUpsampleAndBlurPSO;
	DXRSRenderTarget*									 mVCTVoxelization3DRT;
	DXRSRenderTarget*									 mVCTVoxelization3DRT_CopyForAsync;
	RootSignature                                        mVCTVoxelizationDebugRS;
	GraphicsPSO											 mVCTVoxelizationDebugPSO;
	DXRSRenderTarget*									 mVCTVoxelizationDebugRT;
	DXRSRenderTarget*									 mVCTMainRT;
	DXRSRenderTarget*									 mVCTMainUpsampleAndBlurRT;
	std::vector<DXRSRenderTarget*>						 mVCTAnisoMipmappinPrepare3DRTs;
	std::vector<DXRSRenderTarget*>						 mVCTAnisoMipmappinMain3DRTs;
	__declspec(align(16)) struct VCTVoxelizationCBData
	{
		XMMATRIX WorldVoxelCube;
		XMMATRIX ViewProjection;
		XMMATRIX ShadowViewProjection;
		float WorldVoxelScale;
	};
	__declspec(align(16)) struct VCTAnisoMipmappingCBData
	{
		int MipDimension;
		int MipLevel;
	};
	__declspec(align(16)) struct VCTMainCBData
	{
		XMFLOAT4 CameraPos;
		XMFLOAT2 UpsampleRatio;
		float IndirectDiffuseStrength;
		float IndirectSpecularStrength;
		float MaxConeTraceDistance;
		float AOFalloff;
		float SamplingFactor;
		float VoxelSampleOffset;
	};

	DXRSBuffer* mVCTVoxelizationCB;
	DXRSBuffer* mVCTAnisoMipmappingCB;
	DXRSBuffer* mVCTMainCB;
	std::vector<DXRSBuffer*> mVCTAnisoMipmappingMainCB;

	// Composite
	RootSignature mCompositeRS;
	GraphicsPSO mCompositePSO;

	// UI
	ComPtr<ID3D12DescriptorHeap> mUIDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> mPostAsyncDescriptorHeap;
	DXRS::GPUDescriptorHeap* mMainDescriptorHeap;

	// Lighting
	RootSignature mLightingRS;
	std::vector<DXRSRenderTarget*> mLightingRTs;
	GraphicsPSO mLightingPSO;
	DXRSBuffer* mLightingCB;
	DXRSBuffer* mLightsInfoCB;
	DXRSBuffer* mIlluminationFlagsCB;

	__declspec(align(16)) struct LightingCBData
	{
		XMMATRIX InvViewProjection;
		XMMATRIX ShadowViewProjection;
		XMFLOAT4 CameraPos;
		XMFLOAT4 ScreenSize;
		XMFLOAT2 ShadowTexelSize;
		float ShadowIntensity;
		float pad;
	};
	__declspec(align(16)) struct LightsInfoCBData
	{
		XMFLOAT4 LightDirection;
		XMFLOAT4 LightColor;
		float LightIntensity;
		XMFLOAT3 pad;
	};

	__declspec(align(16)) struct IlluminationFlagsCBData
	{
		int useDirect;
		int useShadows;
		int useRSM;
		int useLPV;
		int useVCT;
		int useVCTDebug;
		int useDXR;
		float dxrReflectionsBlend;
		int showOnlyAO;
	};

	// Directional light
	float mDirectionalLightColor[4]{ 0.9, 0.9, 0.9, 1.0 };
	float mDirectionalLightDir[4]{ 0.191, 1.0f, 0.574f, 1.0 };
	float mDirectionalLightIntensity = 3.0f;
	bool mDynamicDirectionalLight = false;
	float mDynamicDirectionalLightSpeed = 1.0f;

	bool mUseDirectLight = true;
	bool mUseShadows = true;
	bool mUseRSM = false;
	bool mUseLPV = false;
	bool mUseVCT = false;
	bool mShowOnlyAO = false;

	// Shadows
	GraphicsPSO mShadowMappingPSO;
	DXRSDepthBuffer* mShadowDepth;
	XMMATRIX mLightViewProjection;
	XMMATRIX mLightView;
	XMMATRIX mLightProj;
	DXRSBuffer* mShadowMappingCB;
	RootSignature mShadowMappingRS;
	float mShadowIntensity = 0.5f;

	__declspec(align(16)) struct ShadowMappingCBData
	{
		XMMATRIX LightViewProj;
		XMFLOAT4 LightColor;
		XMFLOAT4 LightDir;
	};

	XMFLOAT3 mCameraEye{ 0.0f, 0.0f, 0.0f };
	XMMATRIX mCameraView;
	XMMATRIX mCameraProjection;
	XMFLOAT2 mLastMousePosition;

	float mRSMIntensity = 0.146f;
	float mRSMRMax = 0.015f;
	float mRSMRTRatio = 0.33333f; // from MAX_SCREEN_WIDTH/HEIGHT
	bool mRSMUseUpsampleAndBlur = true;
	bool mRSMComputeVersion = true;
	UINT mRSMDownsampleScaleSize = 4;
	bool mRSMDownsampleForLPV = false;
	bool mRSMDownsampleUseCS = false;

	int mLPVPropagationSteps = 50;
	float mLPVCutoff = 0.2f;
	float mLPVPower = 1.8f;
	float mLPVAttenuation = 1.0f;
	XMMATRIX mWorldToLPV;

	bool mVCTRenderDebug = false;
	float mWorldVoxelScale = VCT_SCENE_VOLUME_SIZE * 0.5f;
	float mVCTIndirectDiffuseStrength = 1.0f;
	float mVCTIndirectSpecularStrength = 1.0f; 
	float mVCTMaxConeTraceDistance = 100.0f;
	float mVCTAoFalloff = 0.03f;
	float mVCTSamplingFactor = 0.5f;
	float mVCTVoxelSampleOffset = 0.0f;
	float mVCTRTRatio = 0.5f; // from MAX_SCREEN_WIDTH/HEIGHT
	bool mVCTUseMainCompute = true;
	bool mVCTMainRTUseUpsampleAndBlur = true;

	// RT 
	IDxcBlob* mRaygenBlob;
	IDxcBlob* mClosestHitBlob;
	IDxcBlob* mMissBlob;

	RootSignature mRaygenRS;
	RootSignature mClosestHitRS;
	RootSignature mMissRS;

	ComPtr<ID3D12DescriptorHeap>        mRaytracingDescriptorHeap;
	ComPtr<ID3D12StateObject>           mRaytracingPSO;
	ComPtr<ID3D12StateObjectProperties> mRaytracingPSOProperties;
	ComPtr<ID3D12Resource>              mRaytracingShaderTableBuffer;
	ShaderBindingTableGenerator         mRaytracingShaderBindingTableHelper;
	ComPtr<ID3D12RootSignature>         mGlobalRaytracingRootSignature;
	ComputePSO							mRaytracingBlurPSO;
	RootSignature						mRaytracingBlurRS;
	DXRSRenderTarget*					mDXRReflectionsRT;
	DXRSRenderTarget*					mDXRReflectionsBlurredRT;
	DXRSRenderTarget*					mDXRReflectionsBlurredRT_Copy;
	DXRSBuffer*							mTLASBuffer; // top level acceleration structure of the scene
	DXRSBuffer*							mTLASScratchBuffer; 
	DXRSBuffer*							mTLASInstanceDescriptionBuffer;
	DXRSBuffer*							mDXRBuffer; //cbuffer for DXR pass

	// DXR
	__declspec(align(16)) struct DXRBuffer
	{
		XMMATRIX ViewMatrix;
		XMMATRIX ProjectionMatrix;
		XMMATRIX InvViewMatrix;
		XMMATRIX InvProjectionMatrix;
		XMMATRIX ShadowViewProjection;
		XMFLOAT4 CamPos;
		XMFLOAT2 ScreenResolution;
	};

	D3D12_DEPTH_STENCIL_DESC mDepthStateRW;
	D3D12_DEPTH_STENCIL_DESC mDepthStateRead;
	D3D12_DEPTH_STENCIL_DESC mDepthStateDisabled;
	D3D12_BLEND_DESC mBlendState;
	D3D12_BLEND_DESC mBlendStateLPVPropagation;
	D3D12_RASTERIZER_DESC mRasterizerState;
	D3D12_RASTERIZER_DESC mRasterizerStateShadow;
	D3D12_SAMPLER_DESC mBilinearSampler;

	bool mUseDynamicObjects = false;

	bool mUseAsyncCompute = false;

	bool mUseDXRReflections = false;
	bool mDXRBlurReflections = true;
	int mDXRBlurPasses = 1;
	float mDXRReflectionsBlend = 0.8f;

	float mFOV = 60.0f;
	bool mLockCamera = false;

	XMVECTOR mLockedCameraPositions[LOCKED_CAMERA_VIEWS] = {
		{2.88f, 16.8f, -0.6f},
		{-23.3f, 10.7f, 25.6f},
		{0.0f, 7.0f, 33.0f}
	};

	XMMATRIX mLockedCameraRotMatrices[LOCKED_CAMERA_VIEWS] = {
		XMMatrixRotationX(-XMConvertToRadians(20.0f))* XMMatrixRotationY(-XMConvertToRadians(40.0f)),
		XMMatrixRotationX(-XMConvertToRadians(10.0f)) * XMMatrixRotationY(-XMConvertToRadians(30.0f)),
		XMMatrixIdentity()
	};

	DXRSBuffer* mGIUpsampleAndBlurBuffer;
	DXRSBuffer* mDXRBlurBuffer;
	__declspec(align(16)) struct UpsampleAndBlurBuffer
	{
		bool Upsample;
	};
};
