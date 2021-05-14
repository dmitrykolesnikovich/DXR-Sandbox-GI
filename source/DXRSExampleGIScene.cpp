#include "DXRSExampleGIScene.h"

#include "DescriptorHeap.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

namespace {
	D3D12_HEAP_PROPERTIES UploadHeapProps = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
	D3D12_HEAP_PROPERTIES DefaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

	static const float clearColorBlack[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const float clearColorWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
}

DXRSExampleGIScene::DXRSExampleGIScene()
{
	mSandboxFramework = new DXRSGraphics();
}

DXRSExampleGIScene::~DXRSExampleGIScene()
{
	if (mSandboxFramework)
		mSandboxFramework->WaitForGpu();

	delete mLightingCB;
	delete mLightsInfoCB;
	delete mShadowMappingCB;
	delete mGbufferCB;
	delete mRSMCB;
	delete mRSMCB2;
}

void DXRSExampleGIScene::Init(HWND window, int width, int height)
{
	mGamePad = std::make_unique<DirectX::GamePad>();
	mKeyboard = std::make_unique<DirectX::Keyboard>();
	mMouse = std::make_unique<DirectX::Mouse>();
	mMouse->SetWindow(window);
	mSandboxFramework->SetWindow(window, width, height);

	mSandboxFramework->CreateResources();
	mSandboxFramework->CreateFullscreenQuadBuffers();

	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\room.fbx"), true, XMMatrixIdentity() * XMMatrixRotationX(-3.14f / 2.0f), XMFLOAT4(0.7, 0.7, 0.7, 0.0)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\dragon.fbx"), true, XMMatrixIdentity() * XMMatrixTranslation(1.5f, 0.0f, -7.0f), XMFLOAT4(0.044f, 0.627f, 0, 0.0)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\bunny.fbx"), true, XMMatrixIdentity() * XMMatrixRotationY(-0.3752457f) * XMMatrixTranslation(21.0f, 13.9f, -19.0f), XMFLOAT4(0.8f, 0.71f, 0, 0.0)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\torus.fbx"), true, XMMatrixIdentity() * XMMatrixRotationX(-3.14f / 2.0f) * XMMatrixRotationX(1.099557f) * XMMatrixTranslation(21.0f, 4.0f, -9.6f), XMFLOAT4(0.329f, 0.26f, 0.8f, 1.0f)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\sphere_big.fbx"), true, XMMatrixIdentity() * XMMatrixTranslation(-17.25f, -1.15f, -24.15f), XMFLOAT4(0.692f, 0.215f, 0.0f, 1.0f)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\sphere_medium.fbx"), true, XMMatrixIdentity() * XMMatrixTranslation(-21.0f, -0.95f, -13.20f), XMFLOAT4(0.005, 0.8, 0.426, 0.7f)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\sphere_small.fbx"), true, XMMatrixIdentity() * XMMatrixTranslation(-11.25f, -0.45f, -16.20f), XMFLOAT4(0.01, 0.0, 0.8, 0.75f)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\block.fbx"), true, XMMatrixIdentity() * XMMatrixRotationX(-3.14f / 2.0f) * XMMatrixTranslation(3.0f, 8.0f, -30.0f), XMFLOAT4(0.9, 0.15, 1.0, 0.0)));
	mObjects.emplace_back(new DXRSModel(*mSandboxFramework, mSandboxFramework->GetFilePath("content\\models\\cube.fbx"), true, XMMatrixIdentity() * XMMatrixRotationX(-3.14f / 2.0f) * XMMatrixRotationY(-0.907571f) * XMMatrixTranslation(21.0f, 5.0f, -19.0f) , XMFLOAT4(0.1, 0.75, 0.8, 0.0)));

	mSandboxFramework->FinalizeResources();
	ID3D12Device* device = mSandboxFramework->GetD3DDevice();

	mStates = std::make_unique<CommonStates>(device);
	mGraphicsMemory = std::make_unique<GraphicsMemory>(device);

	mSandboxFramework->CreateWindowResources();

	auto size = mSandboxFramework->GetOutputSize();
	float aspectRatio = float(size.right) / float(size.bottom);
	mCamera = std::make_unique<DXRSCamera>(mFOV * XM_PI / 180.0f, aspectRatio, 0.01f, 500.0f);
	mCamera->Initialize();
	mCamera->SetPosition(0.0f, 7.0f, 33.0f);

	auto descriptorManager = mSandboxFramework->GetDescriptorHeapManager();

	#pragma region ImGui

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(window);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mUIDescriptorHeap)));
	ImGui_ImplDX12_Init(device, 3, DXGI_FORMAT_R8G8B8A8_UNORM, mUIDescriptorHeap.Get(), mUIDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), mUIDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

#pragma endregion
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mPostAsyncDescriptorHeap)));

	// create a null descriptor for unbound textures
	mNullDescriptor = descriptorManager->CreateCPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// DS buffer
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(nullptr, &srvDesc, mNullDescriptor.GetCPUHandle());
	mDepthStencil = new DXRSDepthBuffer(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, DXGI_FORMAT_D32_FLOAT);

	#pragma region States
	mDepthStateRW.DepthEnable = TRUE;
	mDepthStateRW.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	mDepthStateRW.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	mDepthStateRW.StencilEnable = FALSE;
	mDepthStateRW.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	mDepthStateRW.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	mDepthStateRW.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	mDepthStateRW.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	mDepthStateRW.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mDepthStateRW.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mDepthStateRW.BackFace = mDepthStateRW.FrontFace;

	mDepthStateRead = mDepthStateRW;
	mDepthStateRead.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	mDepthStateDisabled.DepthEnable = FALSE;
	mDepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mDepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	mDepthStateDisabled.StencilEnable = FALSE;
	mDepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	mDepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	mDepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	mDepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	mDepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mDepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mDepthStateDisabled.BackFace = mDepthStateRW.FrontFace;

	mBlendState.IndependentBlendEnable = FALSE;
	mBlendState.RenderTarget[0].BlendEnable = FALSE;
	mBlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	mBlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	mBlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	mBlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	mBlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	mBlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	mBlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	mBlendStateLPVPropagation.IndependentBlendEnable = TRUE;
	for (size_t i = 0; i < 6; i++)
	{
		mBlendStateLPVPropagation.RenderTarget[i].BlendEnable = (i < 3) ? FALSE : TRUE;
		mBlendStateLPVPropagation.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		mBlendStateLPVPropagation.RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
		mBlendStateLPVPropagation.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		mBlendStateLPVPropagation.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		mBlendStateLPVPropagation.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;
		mBlendStateLPVPropagation.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		mBlendStateLPVPropagation.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	mRasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	mRasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	mRasterizerState.FrontCounterClockwise = FALSE;
	mRasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	mRasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	mRasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	mRasterizerState.DepthClipEnable = TRUE;
	mRasterizerState.MultisampleEnable = FALSE;
	mRasterizerState.AntialiasedLineEnable = FALSE;
	mRasterizerState.ForcedSampleCount = 0;
	mRasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	mRasterizerStateShadow.FillMode = D3D12_FILL_MODE_SOLID;
	mRasterizerStateShadow.CullMode = D3D12_CULL_MODE_BACK;
	mRasterizerStateShadow.FrontCounterClockwise = FALSE;
	mRasterizerStateShadow.SlopeScaledDepthBias = 10.0f;
	mRasterizerStateShadow.DepthBias = 0.05f;
	mRasterizerStateShadow.DepthClipEnable = FALSE;
	mRasterizerStateShadow.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	mRasterizerStateShadow.MultisampleEnable = FALSE;
	mRasterizerStateShadow.AntialiasedLineEnable = FALSE;
	mRasterizerStateShadow.ForcedSampleCount = 0;
	mRasterizerStateShadow.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	mBilinearSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	mBilinearSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	mBilinearSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	mBilinearSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//bilinearSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;
	mBilinearSampler.MipLODBias = 0;
	mBilinearSampler.MaxAnisotropy = 16;
	mBilinearSampler.MinLOD = 0.0f;
	mBilinearSampler.MaxLOD = D3D12_FLOAT32_MAX;
	mBilinearSampler.BorderColor[0] = 0.0f;
	mBilinearSampler.BorderColor[1] = 0.0f;
	mBilinearSampler.BorderColor[2] = 0.0f;
	mBilinearSampler.BorderColor[3] = 0.0f;

#pragma endregion

	auto descriptorHeapManager = mSandboxFramework->GetDescriptorHeapManager();
	mMainDescriptorHeap = descriptorHeapManager->GetGPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mMainDescriptorHeap->Reset();

	InitGbuffer(device, descriptorManager);
	InitShadowMapping(device, descriptorManager);
	InitReflectiveShadowMapping(device, descriptorManager);
	InitLightPropagationVolume(device, descriptorManager);
	InitVoxelConeTracing(device, descriptorManager);
	InitLighting(device, descriptorManager);
	InitComposite(device, descriptorManager);
}

void DXRSExampleGIScene::Clear(ID3D12GraphicsCommandList* cmdList)
{
	auto rtvDescriptor = mSandboxFramework->GetRenderTargetView();
	auto dsvDescriptor = mSandboxFramework->GetDepthStencilView();

	//commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
	//commandList->ClearRenderTargetView(rtvDescriptor, Colors::Green, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	auto viewport = mSandboxFramework->GetScreenViewport();
	auto scissorRect = mSandboxFramework->GetScissorRect();
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);
}

void DXRSExampleGIScene::Run()
{
	mTimer.Run([&]()
	{
		Update(mTimer);
	});

	if (mUseAsyncCompute)
		RenderAsync();
	else
		RenderSync();
}

void DXRSExampleGIScene::RenderAsync()
{
	if (mTimer.GetFrameCount() == 0)
		return;

	mSandboxFramework->Prepare(D3D12_RESOURCE_STATE_PRESENT, mUseAsyncCompute ? (mTimer.GetFrameCount() == 1) : true);

	auto commandListGraphics = mSandboxFramework->GetCommandListGraphics();
	auto commandListGraphics2 = mSandboxFramework->GetCommandListGraphics2();
	auto commandListCompute = mSandboxFramework->GetCommandListCompute();

	ID3D12CommandList* ppCommandLists[] = { commandListGraphics };
	ID3D12CommandList* ppCommandLists2[] = { commandListGraphics2 };

	Clear(commandListGraphics);

	auto device = mSandboxFramework->GetD3DDevice();
	auto descriptorHeapManager = mSandboxFramework->GetDescriptorHeapManager();

	DXRS::GPUDescriptorHeap* gpuDescriptorHeap = descriptorHeapManager->GetGPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gpuDescriptorHeap->Reset();

	ID3D12DescriptorHeap* ppHeaps[] = { gpuDescriptorHeap->GetHeap() };

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// 																									//
	// Frame timeline:																					//
	//     Gfx Queue:     [----gfx cmd list #1----|----gfx cmd list #2----|----gfx cmd list #1----]		//
	//     Async Queue:   [-----------------------|-------cmd list--------|-----------------------]		//
	//																									//
	// 																									//
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	//process compute command list - async during the frame (GI)
	{
		if (mTimer.GetFrameCount() > 1) {
			commandListCompute->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

			RenderReflectiveShadowMapping(device, commandListCompute, gpuDescriptorHeap, COMPUTE_QUEUE, true);
			//RenderLightPropagationVolume(device, commandListCompute, gpuDescriptorHeap); // nothing to do for compute queue in LPV...
			RenderVoxelConeTracing(device, commandListCompute, gpuDescriptorHeap, COMPUTE_QUEUE, true);

			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mRSMRT->TransitionTo(mBarriers, commandListCompute, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mRSMUpsampleAndBlurRT->TransitionTo(mBarriers, commandListCompute, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mVCTMainRT->TransitionTo(mBarriers, commandListCompute, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mVCTMainUpsampleAndBlurRT->TransitionTo(mBarriers, commandListCompute, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandListCompute);

			mSandboxFramework->WaitForGraphicsFence2ToFinish(mSandboxFramework->GetCommandQueueCompute());
			mSandboxFramework->PresentCompute(); //execute compute queue and signal graphics queue to continue its' execution
		}
	}

	//process graphics command list 1 - start of the frame (G-buffer, copies for async compute)
	{
		commandListGraphics->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
		CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
		commandListGraphics->RSSetViewports(1, &viewport);
		commandListGraphics->RSSetScissorRects(1, &rect);

		if (mTimer.GetFrameCount() > 1) {
			PIXBeginEvent(commandListGraphics, 0, "Copy buffers for async");
			{
				auto stateRSMbuffer0 = mRSMBuffersRTs[0]->GetCurrentState();
				auto stateRSMbuffer1 = mRSMBuffersRTs[1]->GetCurrentState();
				auto stateRSMbuffer2 = mRSMBuffersRTs[2]->GetCurrentState();
				auto stateVCT3D = mVCTVoxelization3DRT->GetCurrentState();

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mRSMBuffersRTs[0]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_SOURCE);
				mRSMBuffersRTs[1]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_SOURCE);
				mRSMBuffersRTs[2]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_SOURCE);
				mRSMBuffersRTs_CopiesForAsync[0]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_DEST);
				mRSMBuffersRTs_CopiesForAsync[1]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_DEST);
				mRSMBuffersRTs_CopiesForAsync[2]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_DEST);
				mVCTVoxelization3DRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_SOURCE);
				mVCTVoxelization3DRT_CopyForAsync->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COPY_DEST);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandListGraphics);

				commandListGraphics->CopyResource(mRSMBuffersRTs_CopiesForAsync[0]->GetResource(), mRSMBuffersRTs[0]->GetResource());
				commandListGraphics->CopyResource(mRSMBuffersRTs_CopiesForAsync[1]->GetResource(), mRSMBuffersRTs[1]->GetResource());
				commandListGraphics->CopyResource(mRSMBuffersRTs_CopiesForAsync[2]->GetResource(), mRSMBuffersRTs[2]->GetResource());
				commandListGraphics->CopyResource(mVCTVoxelization3DRT_CopyForAsync->GetResource(), mVCTVoxelization3DRT->GetResource());

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mRSMBuffersRTs[0]->TransitionTo(mBarriers, commandListGraphics, stateRSMbuffer0);
				mRSMBuffersRTs[1]->TransitionTo(mBarriers, commandListGraphics, stateRSMbuffer1);
				mRSMBuffersRTs[2]->TransitionTo(mBarriers, commandListGraphics, stateRSMbuffer2);
				mVCTVoxelization3DRT->TransitionTo(mBarriers, commandListGraphics, stateVCT3D);
				mRSMBuffersRTs_CopiesForAsync[0]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mRSMBuffersRTs_CopiesForAsync[1]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mRSMBuffersRTs_CopiesForAsync[2]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mVCTVoxelization3DRT_CopyForAsync->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandListGraphics);
			}
			PIXEndEvent(commandListGraphics);
		}
		RenderGbuffer(device, commandListGraphics, gpuDescriptorHeap);
		if (mTimer.GetFrameCount() > 1) {
			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mGbufferRTs[1]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mGbufferRTs[2]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mRSMBuffersRTs[0]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mRSMBuffersRTs[1]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mRSMBuffersRTs[2]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandListGraphics);
		}

		commandListGraphics->Close();
		mSandboxFramework->GetCommandQueueGraphics()->ExecuteCommandLists(1, ppCommandLists);
		mSandboxFramework->SignalGraphicsFence2();
	}

	//process graphics command list 2 - middle of the frame (Shadows, GI) 
	{
		if (mTimer.GetFrameCount() > 1) {

			commandListGraphics2->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

			RenderShadowMapping(device, commandListGraphics2, gpuDescriptorHeap);

			//copy depth-stencil to custom depth
			PIXBeginEvent(commandListGraphics2, 0, "Copy Depth-Stencil to texture");
			{
				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mSandboxFramework->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
				commandListGraphics2->ResourceBarrier(1, &barrier);
				commandListGraphics2->CopyResource(mDepthStencil->GetResource(), mSandboxFramework->GetDepthStencil());
				barrier = CD3DX12_RESOURCE_BARRIER::Transition(mSandboxFramework->GetDepthStencil(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				commandListGraphics2->ResourceBarrier(1, &barrier);
			}
			PIXEndEvent(commandListGraphics2);

			RenderReflectiveShadowMapping(device, commandListGraphics2, gpuDescriptorHeap, GRAPHICS_QUEUE, true); //only rsm textures generation which cant go to compute
			RenderLightPropagationVolume(device, commandListGraphics2, gpuDescriptorHeap);
			RenderVoxelConeTracing(device, commandListGraphics2, gpuDescriptorHeap, GRAPHICS_QUEUE, true);//only voxelization there which cant go to compute

			mSandboxFramework->WaitForGraphicsFence2ToFinish(mSandboxFramework->GetCommandQueueGraphics(), true);

			commandListGraphics2->Close();
			mSandboxFramework->GetCommandQueueGraphics()->ExecuteCommandLists(1, ppCommandLists2);
		}
	}

	//process graphics command list 1 - end of the frame (Lighting, Composite, UI)
	{
		// submit the rest of the frame to graphics queue
		commandListGraphics->Reset(mSandboxFramework->GetCommandAllocatorGraphics(), nullptr);
		commandListGraphics->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mRSMRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mRSMUpsampleAndBlurRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mVCTMainRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mVCTMainUpsampleAndBlurRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mGbufferRTs[1]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mGbufferRTs[2]->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandListGraphics);

		RenderLighting(device, commandListGraphics, gpuDescriptorHeap);
		RenderComposite(device, commandListGraphics, gpuDescriptorHeap);

		//draw imgui 
		PIXBeginEvent(commandListGraphics, 0, "ImGui");
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesFinal[] =
			{
				 mSandboxFramework->GetRenderTargetView()
			};
			commandListGraphics->OMSetRenderTargets(_countof(rtvHandlesFinal), rtvHandlesFinal, FALSE, nullptr);

			ID3D12DescriptorHeap* ppHeaps[] = { mUIDescriptorHeap.Get() };
			commandListGraphics->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandListGraphics);
		}
		PIXEndEvent(commandListGraphics);

		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mRSMRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COMMON);
		mRSMUpsampleAndBlurRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COMMON);
		mVCTMainRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COMMON);
		mVCTMainUpsampleAndBlurRT->TransitionTo(mBarriers, commandListGraphics, D3D12_RESOURCE_STATE_COMMON);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandListGraphics);

		mSandboxFramework->WaitForComputeToFinish(); //wait for compute queue to finish the current frame
		mSandboxFramework->Present();
		mGraphicsMemory->Commit(mSandboxFramework->GetCommandQueueGraphics());
	}
}

void DXRSExampleGIScene::RenderSync()
{
	if (mTimer.GetFrameCount() == 0)
		return;

	// Prepare the command list to render a new frame.
	mSandboxFramework->Prepare(D3D12_RESOURCE_STATE_PRESENT, true);

	auto commandListGraphics = mSandboxFramework->GetCommandListGraphics();

	Clear(commandListGraphics);

	auto device = mSandboxFramework->GetD3DDevice();
	auto descriptorHeapManager = mSandboxFramework->GetDescriptorHeapManager();

	DXRS::GPUDescriptorHeap* gpuDescriptorHeap = descriptorHeapManager->GetGPUHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gpuDescriptorHeap->Reset();

	ID3D12DescriptorHeap* ppHeaps[] = { gpuDescriptorHeap->GetHeap() };
	commandListGraphics->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	commandListGraphics->RSSetViewports(1, &viewport);
	commandListGraphics->RSSetScissorRects(1, &rect);

	RenderGbuffer(device, commandListGraphics, gpuDescriptorHeap);
	RenderShadowMapping(device, commandListGraphics, gpuDescriptorHeap);

	//copy depth-stencil to custom depth
	PIXBeginEvent(commandListGraphics, 0, "Copy Depth-Stencil to texture");
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mSandboxFramework->GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandListGraphics->ResourceBarrier(1, &barrier);
		commandListGraphics->CopyResource(mDepthStencil->GetResource(), mSandboxFramework->GetDepthStencil());
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(mSandboxFramework->GetDepthStencil(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandListGraphics->ResourceBarrier(1, &barrier);
	}
	PIXEndEvent(commandListGraphics);

	RenderReflectiveShadowMapping(device, commandListGraphics, gpuDescriptorHeap);
	RenderLightPropagationVolume(device, commandListGraphics, gpuDescriptorHeap);
	RenderVoxelConeTracing(device, commandListGraphics, gpuDescriptorHeap);
	RenderLighting(device, commandListGraphics, gpuDescriptorHeap);
	RenderComposite(device, commandListGraphics, gpuDescriptorHeap);

	//draw imgui 
	PIXBeginEvent(commandListGraphics, 0, "ImGui");
	{
		ID3D12DescriptorHeap* ppHeaps[] = { mUIDescriptorHeap.Get() };
		commandListGraphics->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandListGraphics);
	}
	PIXEndEvent(commandListGraphics);

	mSandboxFramework->Present();
	mGraphicsMemory->Commit(mSandboxFramework->GetCommandQueueGraphics());
}

void DXRSExampleGIScene::Update(DXRSTimer const& timer)
{
	UpdateCamera(timer);
	UpdateLights(timer);
	UpdateBuffers(timer);
	UpdateImGui();
	UpdateTransforms(timer);
}

void DXRSExampleGIScene::UpdateTransforms(DXRSTimer const& timer) 
{
	for (auto& model : mObjects) {
		if (mUseDynamicObjects && model->GetFileName().find("content\\models\\cube.fbx") != std::string::npos)
		{
			model->UpdateWorldMatrix(XMMatrixIdentity() * XMMatrixRotationX(-3.14f / 2.0f) * XMMatrixRotationY(-0.907571f) * XMMatrixTranslation(sin(timer.GetTotalSeconds() * mDynamicDirectionalLightSpeed) * 5.0f, 5.0f, -19.0f));
		}
		else 
			model->UpdateWorldMatrix(model->GetWorldMatrix()); //TODO unnecessary but can be used in the future for custom matrix update
	}
}

void DXRSExampleGIScene::UpdateBuffers(DXRSTimer const& timer)
{
	float width = mSandboxFramework->GetOutputSize().right;
	float height = mSandboxFramework->GetOutputSize().bottom;
	GBufferCBData gbufferPassData;
	gbufferPassData.ViewProjection = mCameraView * mCameraProjection;
	gbufferPassData.InvViewProjection = XMMatrixInverse(nullptr, gbufferPassData.ViewProjection);
	gbufferPassData.CameraPos = XMFLOAT4(mCameraEye.x, mCameraEye.y, mCameraEye.z, 1);
	gbufferPassData.ScreenSize = { width, height, 1.0f / width, 1.0f / height };
	gbufferPassData.LightColor = XMFLOAT4(mDirectionalLightColor[0], mDirectionalLightColor[1], mDirectionalLightColor[2], mDirectionalLightColor[3]);
	memcpy(mGbufferCB->Map(), &gbufferPassData, sizeof(gbufferPassData));

	XMFLOAT3 shadowCenter(0.0f, 0.0f, 0.0f);
	XMVECTOR eyePosition = XMLoadFloat3(&shadowCenter);
	XMVECTOR direction = XMVECTOR{ -mDirectionalLightDir[0], -mDirectionalLightDir[1], -mDirectionalLightDir[2], mDirectionalLightDir[3] };
	XMVECTOR upDirection = XMVECTOR{ 0.0f, 1.0f, 0.0f };

	XMMATRIX viewMatrix = XMMatrixLookToRH(eyePosition, direction, upDirection);
	XMMATRIX projectionMatrix = XMMatrixOrthographicRH(256.0f, 256.0f, -256.0f, 256.0f);
	mLightViewProjection = viewMatrix * projectionMatrix;
	mLightView = viewMatrix;
	mLightProj = projectionMatrix;

	ShadowMappingCBData shadowPassData = {};
	shadowPassData.LightViewProj = mLightViewProjection;
	shadowPassData.LightColor = XMFLOAT4(mDirectionalLightColor);
	shadowPassData.LightDir = XMFLOAT4(-mDirectionalLightDir[0], -mDirectionalLightDir[1], -mDirectionalLightDir[2], mDirectionalLightDir[3]);
	memcpy(mShadowMappingCB->Map(), &shadowPassData, sizeof(shadowPassData));

	LightingCBData lightPassData = {};
	lightPassData.InvViewProjection = XMMatrixInverse(nullptr, gbufferPassData.ViewProjection);
	lightPassData.ShadowViewProjection = mLightViewProjection;
	lightPassData.ShadowTexelSize = XMFLOAT2(1.0f / mShadowDepth->GetWidth(), 1.0f / mShadowDepth->GetHeight());
	lightPassData.ShadowIntensity = mShadowIntensity;
	lightPassData.CameraPos = XMFLOAT4(mCameraEye.x, mCameraEye.y, mCameraEye.z, 1);
	lightPassData.ScreenSize = { width, height, 1.0f / width, 1.0f / height };
	memcpy(mLightingCB->Map(), &lightPassData, sizeof(lightPassData));

	IlluminationFlagsCBData illumData = {};
	illumData.useDirect = mUseDirectLight ? 1 : 0;
	illumData.useShadows = mUseShadows ? 1 : 0;
	illumData.useRSM = mUseRSM ? 1 : 0;
	illumData.useLPV = mUseLPV ? 1 : 0;
	illumData.useVCT = mUseVCT ? 1 : 0;
	illumData.showOnlyAO = mShowOnlyAO ? 1 : 0;
	memcpy(mIlluminationFlagsCB->Map(), &illumData, sizeof(illumData));

	RSMCBData rsmPassData = {};
	rsmPassData.ShadowViewProjection = mLightViewProjection;
	rsmPassData.RSMIntensity = mRSMIntensity;
	rsmPassData.RSMRMax = mRSMRMax;
	rsmPassData.UpsampleRatio = XMFLOAT2(mGbufferRTs[0]->GetWidth() / mRSMRT->GetWidth(), mGbufferRTs[0]->GetHeight() / mRSMRT->GetHeight());
	memcpy(mRSMCB->Map(), &rsmPassData, sizeof(rsmPassData));

	RSMCBDataDownsample rsmDownsamplePassData = {};
	rsmDownsamplePassData.LightDir = XMFLOAT4(mDirectionalLightDir[0], mDirectionalLightDir[1], mDirectionalLightDir[2], mDirectionalLightDir[3]);
	rsmDownsamplePassData.ScaleSize = mRSMDownsampleScaleSize;
	memcpy(mRSMDownsampleCB->Map(), &rsmDownsamplePassData, sizeof(rsmDownsamplePassData));

	LPVCBData lpvData = {};
	lpvData.worldToLPV = mWorldToLPV;
	lpvData.LPVCutoff = mLPVCutoff;
	lpvData.LPVPower = mLPVPower;
	lpvData.LPVAttenuation = mLPVAttenuation;
	memcpy(mLPVCB->Map(), &lpvData, sizeof(lpvData));

	VCTVoxelizationCBData voxelData = {};
	float scale = 1.0f;// VCT_SCENE_VOLUME_SIZE / mWorldVoxelScale;
	voxelData.WorldVoxelCube = XMMatrixTranslation(-VCT_SCENE_VOLUME_SIZE * 0.25f, -VCT_SCENE_VOLUME_SIZE * 0.25f, -VCT_SCENE_VOLUME_SIZE * 0.25f) * XMMatrixScaling(scale, -scale, scale);
	voxelData.ViewProjection = mCameraView * mCameraProjection;
	voxelData.ShadowViewProjection = mLightViewProjection;
	voxelData.WorldVoxelScale = mWorldVoxelScale;
	memcpy(mVCTVoxelizationCB->Map(), &voxelData, sizeof(voxelData));

	VCTMainCBData vctMainData = {};
	vctMainData.CameraPos = XMFLOAT4(mCameraEye.x, mCameraEye.y, mCameraEye.z, 1);
	vctMainData.UpsampleRatio = XMFLOAT2(mGbufferRTs[0]->GetWidth() / mVCTMainRT->GetWidth(), mGbufferRTs[0]->GetHeight() / mVCTMainRT->GetHeight());
	vctMainData.IndirectDiffuseStrength = mVCTIndirectDiffuseStrength;
	vctMainData.IndirectSpecularStrength = mVCTIndirectSpecularStrength;
	vctMainData.MaxConeTraceDistance = mVCTMaxConeTraceDistance;
	vctMainData.AOFalloff = mVCTAoFalloff;
	vctMainData.SamplingFactor = mVCTSamplingFactor;
	vctMainData.VoxelSampleOffset = mVCTVoxelSampleOffset;
	memcpy(mVCTMainCB->Map(), &vctMainData, sizeof(vctMainData));
}

void DXRSExampleGIScene::UpdateImGui()
{
	//capture mouse clicks
	ImGui::GetIO().MouseDown[0] = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
	ImGui::GetIO().MouseDown[1] = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;
	ImGui::GetIO().MouseDown[2] = (GetKeyState(VK_MBUTTON) & 0x8000) != 0;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	{
		ImGui::Begin("DirectX GI Sandbox");
		ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.0f, 1), "FPS: (%.1f FPS), %.3f ms/frame", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("Camera pos: %f, %f, %f", mCameraEye.x, mCameraEye.y, mCameraEye.z);
		ImGui::Checkbox("Lock camera", &mLockCamera);
		mCamera->SetLock(mLockCamera);
		if (mLockCamera) {
			mCamera->SetLock(true);
			for (int i = 0; i < 3; i++)
			{
				std::string name = "Mode " + std::to_string(i);
				XMVECTOR upDirection = XMLoadFloat3(&mCamera->Up());
				if (ImGui::Button(name.c_str()))
					mCamera->SetViewMatrix(XMMatrixLookAtRH(mLockedCameraPositions[i], mLockedCameraTargets[i], upDirection));
				if (i < 2)
					ImGui::SameLine();
			}
		}

		ImGui::SliderFloat4("Light Color", mDirectionalLightColor, 0.0f, 1.0f);
		ImGui::SliderFloat4("Light Direction", mDirectionalLightDir, -1.0f, 1.0f);
		ImGui::SliderFloat("Light Intensity", &mDirectionalLightIntensity, 0.0f, 5.0f);
		ImGui::Checkbox("Dynamic Light", &mDynamicDirectionalLight);
		ImGui::SameLine();
		ImGui::SliderFloat("Speed", &mDynamicDirectionalLightSpeed, 0.0f, 5.0f);
		ImGui::Checkbox("Dynamic objects", &mUseDynamicObjects);

		ImGui::Separator();

		ImGui::Checkbox("Use Direct Light", &mUseDirectLight);
		ImGui::Checkbox("Use Direct Shadows", &mUseShadows);
		ImGui::Checkbox("Use RSM (Indirect Light)", &mUseRSM);
		ImGui::Checkbox("Use LPV (Indirect Light)", &mUseLPV);
		ImGui::Checkbox("Use VCT (Indirect Light)", &mUseVCT);
		ImGui::Checkbox("Show AO", &mShowOnlyAO);

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Global Illumination Config"))
		{
			if (ImGui::CollapsingHeader("Reflective Shadow Mapping")) {
				ImGui::Checkbox("Use CS for main RSM pass", &mRSMComputeVersion);
				ImGui::Checkbox("Upsample & blur RSM result in CS", &mRSMUseUpsampleAndBlur);
				ImGui::Separator();
				ImGui::SliderFloat("RSM Intensity", &mRSMIntensity, 0.0f, 15.0f);
				ImGui::SliderFloat("RSM Rmax", &mRSMRMax, 0.0f, 1.0f);
				//ImGui::SliderInt("RSM Samples Count", &mRSMSamplesCount, 1, 1000);
			}
			if (ImGui::CollapsingHeader("Light Propagation Volume")) {
				ImGui::Checkbox("Use downsampled RSM", &mRSMDownsampleForLPV);
				ImGui::SameLine();
				ImGui::Checkbox("in CS", &mRSMDownsampleUseCS);
				ImGui::Separator();
				ImGui::SliderInt("Propagation steps", &mLPVPropagationSteps, 0, 100);
				ImGui::SliderFloat("Cutoff", &mLPVCutoff, 0.0f, 1.0f);
				ImGui::SliderFloat("Power", &mLPVPower, 0.0f, 2.0f);
				ImGui::SliderFloat("Attenuation", &mLPVAttenuation, 0.0f, 5.0f);
				ImGui::Separator();
				ImGui::Checkbox("DX12 bundles for propagation", &mUseBundleForLPVPropagation);
				if (mUseBundleForLPVPropagation)
				{
					ImGui::SameLine();
					if (ImGui::Button("Update")) {
						mLPVPropagationBundle1Closed = false;
						mLPVPropagationBundle2Closed = false;
						mLPVPropagationBundlesClosed = false;
						mLPVPropagationBundle1->Reset(mLPVPropagationBundle1Allocator.Get(), nullptr);
						mLPVPropagationBundle2->Reset(mLPVPropagationBundle2Allocator.Get(), nullptr);
					}
				}
			}
			if (ImGui::CollapsingHeader("Voxel Cone Tracing")) {
				ImGui::Checkbox("Use CS for main VCT pass", &mVCTUseMainCompute);
				ImGui::Checkbox("Upsample & blur VCT result in CS", &mVCTMainRTUseUpsampleAndBlur);
				ImGui::Separator();
				ImGui::SliderFloat("Diffuse Strength", &mVCTIndirectDiffuseStrength, 0.0f, 1.0f);
				ImGui::SliderFloat("Specular Strength", &mVCTIndirectSpecularStrength, 0.0f, 1.0f);
				ImGui::SliderFloat("Max Cone Trace Dist", &mVCTMaxConeTraceDistance, 0.0f, 2500.0f);
				ImGui::SliderFloat("AO Falloff", &mVCTAoFalloff, 0.0f, 2.0f);
				ImGui::SliderFloat("Sampling Factor", &mVCTSamplingFactor, 0.01f, 3.0f);
				ImGui::SliderFloat("Sample Offset", &mVCTVoxelSampleOffset, -0.1f, 0.1f);
				ImGui::Separator();
				ImGui::Checkbox("Render voxels for debug", &mVCTRenderDebug);
			}
		}
		ImGui::Separator();

		ImGui::Checkbox("DX12 Asynchronous Compute", &mUseAsyncCompute);

		ImGui::End();
	}
}

void DXRSExampleGIScene::UpdateLights(DXRSTimer const& timer)
{
	if (mDynamicDirectionalLight)
		mDirectionalLightDir[0] = sin(timer.GetTotalSeconds() * mDynamicDirectionalLightSpeed);

	LightsInfoCBData lightData = {};
	lightData.LightColor = XMFLOAT4(mDirectionalLightColor[0], mDirectionalLightColor[1], mDirectionalLightColor[2], mDirectionalLightColor[3]);
	lightData.LightDirection = XMFLOAT4(mDirectionalLightDir[0], mDirectionalLightDir[1], mDirectionalLightDir[2], mDirectionalLightDir[3]);
	lightData.LightIntensity = mDirectionalLightIntensity;

	memcpy(mLightsInfoCB->Map(), &lightData, sizeof(lightData));
}

void DXRSExampleGIScene::UpdateCamera(DXRSTimer const& timer)
{
	mCamera->Update(timer, mMouse, mKeyboard);

	mCameraView = mCamera->ViewMatrix();
	mCameraProjection = mCamera->ProjectionMatrix();
	mCameraEye = mCamera->Position();
}

void DXRSExampleGIScene::OnWindowSizeChanged(int width, int height)
{
	if (!mSandboxFramework->WindowSizeChanged(width, height))
		return;

	auto size = mSandboxFramework->GetOutputSize();
	mCamera->SetAspectRatio(float(size.right) / float(size.bottom));
}

void DXRSExampleGIScene::ThrowFailedErrorBlob(ID3DBlob* blob)
{
	std::string message = "";
	if (blob)
		message.append((char*)blob->GetBufferPointer());

	throw std::runtime_error(message.c_str());
}

void DXRSExampleGIScene::RenderObject(U_PTR<DXRSModel>& aModel, std::function<void(U_PTR<DXRSModel>&)> aCallback)
{
	if (aCallback) {
		aCallback(aModel);
	}
}

void DXRSExampleGIScene::InitGbuffer(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
{
	D3D12_SAMPLER_DESC sampler;
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;

	DXGI_FORMAT rtFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, rtFormat, flags, L"Albedo"));

	rtFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
	mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, rtFormat, flags, L"Normals"));

	rtFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	mGbufferRTs.push_back(new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, rtFormat, flags, L"World Positions"));

	// root signature
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	mGbufferRS.Reset(1, 1);
	mGbufferRS.InitStaticSampler(0, sampler, D3D12_SHADER_VISIBILITY_PIXEL);
	mGbufferRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
	mGbufferRS.Finalize(device, L"GPrepassRS", rootSignatureFlags);

	//Create Pipeline State Object
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ID3DBlob* errorBlob = nullptr;

	ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\GBuffer.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));

	compileFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

	ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\GBuffer.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	DXGI_FORMAT formats[3];
	formats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	formats[1] = DXGI_FORMAT_R16G16B16A16_SNORM;
	formats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;

	mGbufferPSO.SetRootSignature(mGbufferRS);
	mGbufferPSO.SetRasterizerState(mRasterizerState);
	mGbufferPSO.SetBlendState(mBlendState);
	mGbufferPSO.SetDepthStencilState(mDepthStateRW);
	mGbufferPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
	mGbufferPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	mGbufferPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
	mGbufferPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
	mGbufferPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
	mGbufferPSO.Finalize(device);

	//create constant buffer for pass
	DXRSBuffer::Description cbDesc;
	cbDesc.mElementSize = sizeof(GBufferCBData);
	cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
	cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

	mGbufferCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"GBuffer CB");
}
void DXRSExampleGIScene::RenderGbuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap)
{
	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);

	PIXBeginEvent(commandList, 0, "GBuffer");
	{
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &rect);

		commandList->SetPipelineState(mGbufferPSO.GetPipelineStateObject());
		commandList->SetGraphicsRootSignature(mGbufferRS.GetSignature());

		//transition buffers to rendertarget outputs
		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mGbufferRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mGbufferRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mGbufferRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] =
		{
			mGbufferRTs[0]->GetRTV().GetCPUHandle(),
			mGbufferRTs[1]->GetRTV().GetCPUHandle(),
			mGbufferRTs[2]->GetRTV().GetCPUHandle()
		};

		commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &mSandboxFramework->GetDepthStencilView());
		commandList->ClearRenderTargetView(rtvHandles[0], clearColorBlack, 0, nullptr);
		commandList->ClearRenderTargetView(rtvHandles[1], clearColorBlack, 0, nullptr);
		commandList->ClearRenderTargetView(rtvHandles[2], clearColorBlack, 0, nullptr);

		DXRS::DescriptorHandle cbvHandle;

		for (auto& model : mObjects) {
			RenderObject(model, [this, gpuDescriptorHeap, commandList, &cbvHandle, device](U_PTR<DXRSModel>& anObject) {
				cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mGbufferCB->GetCBV());
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, anObject->GetCB()->GetCBV());

				commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
				anObject->Render(commandList);
			});
		}
	}
	PIXEndEvent(commandList);
}

void DXRSExampleGIScene::InitShadowMapping(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
// create resources for shadow mapping
{
	mShadowDepth = new DXRSDepthBuffer(device, descriptorManager, SHADOWMAP_SIZE, SHADOWMAP_SIZE, DXGI_FORMAT_D32_FLOAT);

	//create root signature
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	mShadowMappingRS.Reset(1, 0);
	mShadowMappingRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_VERTEX);
	mShadowMappingRS.Finalize(device, L"Shadow Mapping pass RS", rootSignatureFlags);

	ComPtr<ID3DBlob> vertexShader;
	//ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ID3DBlob* errorBlob = nullptr;

	HRESULT res = D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\ShadowMapping.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSOnlyMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob);

	//if (errorBlob) {
	//	std::string resultMessasge;
	//	resultMessasge.append((char*)errorBlob->GetBufferPointer());
	//}

	ThrowIfFailed(res);

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mShadowMappingPSO.SetRootSignature(mShadowMappingRS);
	mShadowMappingPSO.SetRasterizerState(mRasterizerStateShadow);
	mShadowMappingPSO.SetRenderTargetFormats(0, nullptr, mShadowDepth->GetFormat());
	mShadowMappingPSO.SetBlendState(mBlendState);
	mShadowMappingPSO.SetDepthStencilState(mDepthStateRW);
	mShadowMappingPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
	mShadowMappingPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	mShadowMappingPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
	//mShadowMappingPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
	mShadowMappingPSO.Finalize(device);

	//create constant buffer for pass
	DXRSBuffer::Description cbDesc;
	cbDesc.mElementSize = sizeof(ShadowMappingCBData);
	cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
	cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

	mShadowMappingCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"Shadow Mapping CB");
}
void DXRSExampleGIScene::RenderShadowMapping(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap)
{
	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);

	PIXBeginEvent(commandList, 0, "Shadows");
	{
		CD3DX12_VIEWPORT shadowMapViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mShadowDepth->GetWidth(), mShadowDepth->GetHeight());
		CD3DX12_RECT shadowRect = CD3DX12_RECT(0.0f, 0.0f, mShadowDepth->GetWidth(), mShadowDepth->GetHeight());
		commandList->RSSetViewports(1, &shadowMapViewport);
		commandList->RSSetScissorRects(1, &shadowRect);

		commandList->SetPipelineState(mShadowMappingPSO.GetPipelineStateObject());
		commandList->SetGraphicsRootSignature(mShadowMappingRS.GetSignature());

		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mShadowDepth->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

		commandList->OMSetRenderTargets(0, nullptr, FALSE, &mShadowDepth->GetDSV().GetCPUHandle());
		commandList->ClearDepthStencilView(mShadowDepth->GetDSV().GetCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		DXRS::DescriptorHandle cbvHandle;

		for (auto& model : mObjects) {
			RenderObject(model, [this, gpuDescriptorHeap, commandList, &cbvHandle, device](U_PTR<DXRSModel>& anObject) {
				cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mShadowMappingCB->GetCBV());
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, anObject->GetCB()->GetCBV());

				commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
				anObject->Render(commandList);
			});
		}

		//reset back
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &rect);
	}
	PIXEndEvent(commandList);
}

void DXRSExampleGIScene::InitReflectiveShadowMapping(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
{
	//generation
	{
		DXGI_FORMAT rtFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		mRSMBuffersRTs.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE, RSM_SIZE, rtFormat, flags, L"RSM World Pos"));
		mRSMBuffersRTs.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE, RSM_SIZE, rtFormat, flags, L"RSM Normals"));
		mRSMBuffersRTs.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE, RSM_SIZE, rtFormat, flags, L"RSM Flux"));
		
		mRSMBuffersRTs_CopiesForAsync.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE, RSM_SIZE, rtFormat, flags, L"C RSM World Pos"));
		mRSMBuffersRTs_CopiesForAsync.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE, RSM_SIZE, rtFormat, flags, L"C RSM Normals"));
		mRSMBuffersRTs_CopiesForAsync.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE, RSM_SIZE, rtFormat, flags, L"C RSM Flux"));

		// root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		mRSMBuffersRS.Reset(1, 0);
		mRSMBuffersRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
		mRSMBuffersRS.Finalize(device, L"RSM Buffers RS", rootSignatureFlags);

		//Create Pipeline State Object
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\ShadowMapping.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));

		compileFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\ShadowMapping.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSRSM", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		DXGI_FORMAT formats[3];
		formats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;

		mRSMBuffersPSO.SetRootSignature(mRSMBuffersRS);
		mRSMBuffersPSO.SetRasterizerState(mRasterizerState);
		mRSMBuffersPSO.SetBlendState(mBlendState);
		mRSMBuffersPSO.SetDepthStencilState(mDepthStateRead);
		mRSMBuffersPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		mRSMBuffersPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		mRSMBuffersPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
		mRSMBuffersPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		mRSMBuffersPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
		mRSMBuffersPSO.Finalize(device);
	}

	//calculation
	{
		//RTs
		mRSMRT = new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH * mRSMRTRatio, MAX_SCREEN_HEIGHT * mRSMRTRatio,
			DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"RSM Indirect Illumination");

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		D3D12_SAMPLER_DESC rsmSampler;
		rsmSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		rsmSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		rsmSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		rsmSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		//rsmSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		rsmSampler.MipLODBias = 0;
		rsmSampler.MaxAnisotropy = 16;
		rsmSampler.MinLOD = 0.0f;
		rsmSampler.MaxLOD = D3D12_FLOAT32_MAX;
		rsmSampler.BorderColor[0] = 0.0f;
		rsmSampler.BorderColor[1] = 0.0f;
		rsmSampler.BorderColor[2] = 0.0f;
		rsmSampler.BorderColor[3] = 0.0f;

		// pixel version
		{
			mRSMRS.Reset(2, 1);
			mRSMRS.InitStaticSampler(0, rsmSampler, D3D12_SHADER_VISIBILITY_PIXEL);
			mRSMRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
			mRSMRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5, D3D12_SHADER_VISIBILITY_PIXEL);
			mRSMRS.Finalize(device, L"RSM pixel shader pass RS", rootSignatureFlags);

			//PSO
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif
			ID3DBlob* errorBlob = nullptr;

			ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\ReflectiveShadowMappingPS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob));
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\ReflectiveShadowMappingPS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob));
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			DXGI_FORMAT m_rtFormats[1];
			m_rtFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			mRSMPSO.SetRootSignature(mRSMRS);
			mRSMPSO.SetRasterizerState(mRasterizerState);
			mRSMPSO.SetBlendState(mBlendState);
			mRSMPSO.SetDepthStencilState(mDepthStateDisabled);
			mRSMPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
			mRSMPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			mRSMPSO.SetRenderTargetFormats(_countof(m_rtFormats), m_rtFormats, DXGI_FORMAT_D32_FLOAT);
			mRSMPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
			mRSMPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
			mRSMPSO.Finalize(device);
		}

		// compute version
		{
			mRSMRS_Compute.Reset(3, 1);
			mRSMRS_Compute.InitStaticSampler(0, rsmSampler, D3D12_SHADER_VISIBILITY_ALL);
			mRSMRS_Compute[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
			mRSMRS_Compute[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5, D3D12_SHADER_VISIBILITY_ALL);
			mRSMRS_Compute[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
			mRSMRS_Compute.Finalize(device, L"RSM compute shader pass RS", rootSignatureFlags);

			ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif
			ID3DBlob* errorBlob = nullptr;

			ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\ReflectiveShadowMappingCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			mRSMPSO_Compute.SetRootSignature(mRSMRS_Compute);
			mRSMPSO_Compute.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
			mRSMPSO_Compute.Finalize(device);
		}

		//CB
		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(RSMCBData);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

		mRSMCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"RSM Pass CB");

		cbDesc.mElementSize = sizeof(RSMCBDataRandomValues);
		mRSMCB2 = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"RSM Pass CB 2");

		RSMCBDataRandomValues rsmPassData2 = {};
		for (int i = 0; i < RSM_SAMPLES_COUNT; i++)
		{
			rsmPassData2.xi[i].x = RandomFloat(0.0f, 1.0f);
			rsmPassData2.xi[i].y = RandomFloat(0.0f, 1.0f);
			rsmPassData2.xi[i].z = 0.0f;
			rsmPassData2.xi[i].w = 0.0f;
		}
		memcpy(mRSMCB2->Map(), &rsmPassData2, sizeof(rsmPassData2));
	}

	//blur
	{
		//RTs
		mRSMUpsampleAndBlurRT = new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"RSM Main RT Upsampled & Blurred", -1, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		mRSMUpsampleAndBlurRS.Reset(2, 1);
		mRSMUpsampleAndBlurRS.InitStaticSampler(0, mBilinearSampler, D3D12_SHADER_VISIBILITY_ALL);
		mRSMUpsampleAndBlurRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mRSMUpsampleAndBlurRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mRSMUpsampleAndBlurRS.Finalize(device, L"RSM Main RT Upsample & Blur pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\UpsampleBlurCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		mRSMUpsampleAndBlurPSO.SetRootSignature(mRSMUpsampleAndBlurRS);
		mRSMUpsampleAndBlurPSO.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
		mRSMUpsampleAndBlurPSO.Finalize(device);
	}

	//downsampling for LPV - PS
	{
		DXGI_FORMAT rtFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		mRSMDownsampledBuffersRTs.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE / mRSMDownsampleScaleSize, RSM_SIZE / mRSMDownsampleScaleSize, rtFormat, flags, L"RSM Downsampled World Pos", -1, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mRSMDownsampledBuffersRTs.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE / mRSMDownsampleScaleSize, RSM_SIZE / mRSMDownsampleScaleSize, rtFormat, flags, L"RSM Downsampled Normals", -1, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mRSMDownsampledBuffersRTs.push_back(new DXRSRenderTarget(device, descriptorManager, RSM_SIZE / mRSMDownsampleScaleSize, RSM_SIZE / mRSMDownsampleScaleSize, rtFormat, flags, L"RSM Downsampled Flux", -1, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		// root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		mRSMDownsampleRS.Reset(2, 0);
		mRSMDownsampleRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		mRSMDownsampleRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_PIXEL);
		mRSMDownsampleRS.Finalize(device, L"RSM Downsample RS", rootSignatureFlags);

		//Create Pipeline State Object
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\RSMDownsamplePS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));

		compileFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\RSMDownsamplePS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		DXGI_FORMAT formats[3];
		formats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;

		mRSMDownsamplePSO.SetRootSignature(mRSMDownsampleRS);
		mRSMDownsamplePSO.SetRasterizerState(mRasterizerState);
		mRSMDownsamplePSO.SetBlendState(mBlendState);
		mRSMDownsamplePSO.SetDepthStencilState(mDepthStateDisabled);
		mRSMDownsamplePSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		mRSMDownsamplePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		mRSMDownsamplePSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
		mRSMDownsamplePSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		mRSMDownsamplePSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
		mRSMDownsamplePSO.Finalize(device);

		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(RSMCBDataDownsample);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

		mRSMDownsampleCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"RSM Downsample CB");
	}

	//downsampling for LPV - CS
	{
		// root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		mRSMDownsampleRS_Compute.Reset(3, 0);
		mRSMDownsampleRS_Compute[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mRSMDownsampleRS_Compute[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
		mRSMDownsampleRS_Compute[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
		mRSMDownsampleRS_Compute.Finalize(device, L"RSM Downsample compute pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\RSMDownsampleCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		mRSMDownsamplePSO_Compute.SetRootSignature(mRSMDownsampleRS_Compute);
		mRSMDownsamplePSO_Compute.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
		mRSMDownsamplePSO_Compute.Finalize(device);
	}
}
void DXRSExampleGIScene::RenderReflectiveShadowMapping(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap, RenderQueue aQueue, bool useAsyncCompute)
{
	if (!mUseRSM && !mUseLPV)
		return;

	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);

	auto clearRSMRT = [this, commandList]() {
		commandList->SetPipelineState(mRSMPSO.GetPipelineStateObject());
		commandList->SetGraphicsRootSignature(mRSMRS.GetSignature());

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesRSM[] = { mRSMRT->GetRTV().GetCPUHandle() };
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandlesRSM[] = { mRSMUpsampleAndBlurRT->GetUAV().GetCPUHandle() };

		//transition buffers to rendertarget outputs
		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mRSMRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

		commandList->OMSetRenderTargets(_countof(rtvHandlesRSM), rtvHandlesRSM, FALSE, nullptr);
		commandList->ClearRenderTargetView(rtvHandlesRSM[0], clearColorBlack, 0, nullptr);

		//TODO fix null GPU Handle in UAV
		//commandList->ClearUnorderedAccessViewFloat(mRSMUpsampleAndBlurRT->GetUAV().GetGPUHandle(), mRSMUpsampleAndBlurRT->GetUAV().GetCPUHandle(),
		//	mRSMUpsampleAndBlurRT->GetResource(), clearColor, 0, nullptr);
	};

	if (mUseRSM || mUseLPV) {
		if (!useAsyncCompute || (useAsyncCompute && aQueue == GRAPHICS_QUEUE)) {
			// buffers generation (pos, normals, flux)
			PIXBeginEvent(commandList, 0, "RSM buffers generation");
			{
				CD3DX12_VIEWPORT rsmBuffersViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mRSMBuffersRTs[0]->GetWidth(), mRSMBuffersRTs[0]->GetHeight());
				CD3DX12_RECT rsmRect = CD3DX12_RECT(0.0f, 0.0f, mRSMBuffersRTs[0]->GetWidth(), mRSMBuffersRTs[0]->GetHeight());
				commandList->RSSetViewports(1, &rsmBuffersViewport);
				commandList->RSSetScissorRects(1, &rsmRect);

				commandList->SetPipelineState(mRSMBuffersPSO.GetPipelineStateObject());
				commandList->SetGraphicsRootSignature(mRSMBuffersRS.GetSignature());

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mRSMBuffersRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				mRSMBuffersRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				mRSMBuffersRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				mShadowDepth->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_DEPTH_READ);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] =
				{
					mRSMBuffersRTs[0]->GetRTV().GetCPUHandle(),
					mRSMBuffersRTs[1]->GetRTV().GetCPUHandle(),
					mRSMBuffersRTs[2]->GetRTV().GetCPUHandle()
				};

				commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &mShadowDepth->GetDSV().GetCPUHandle());
				commandList->ClearRenderTargetView(rtvHandles[0], clearColorBlack, 0, nullptr);
				commandList->ClearRenderTargetView(rtvHandles[1], clearColorBlack, 0, nullptr);
				commandList->ClearRenderTargetView(rtvHandles[2], clearColorBlack, 0, nullptr);

				DXRS::DescriptorHandle cbvHandle;

				for (auto& model : mObjects) {
					RenderObject(model, [this, gpuDescriptorHeap, commandList, &cbvHandle, device](U_PTR<DXRSModel>& anObject) {
						cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
						gpuDescriptorHeap->AddToHandle(device, cbvHandle, mShadowMappingCB->GetCBV());
						gpuDescriptorHeap->AddToHandle(device, cbvHandle, anObject->GetCB()->GetCBV());

						commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
						anObject->Render(commandList);
					});
				}

				//reset back
				commandList->RSSetViewports(1, &viewport);
				commandList->RSSetScissorRects(1, &rect);
			}
			PIXEndEvent(commandList);
		}
		// downsample for LPV
		if (mRSMDownsampleForLPV) {
			if ((!useAsyncCompute || (useAsyncCompute && aQueue == GRAPHICS_QUEUE)) && !mRSMDownsampleUseCS) {
				PIXBeginEvent(commandList, 0, "RSM downsample PS for LPV");
				{
					CD3DX12_VIEWPORT rsmDownsampleResViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, RSM_SIZE / mRSMDownsampleScaleSize, RSM_SIZE / mRSMDownsampleScaleSize);
					CD3DX12_RECT rsmDownsampleRect = CD3DX12_RECT(0.0f, 0.0f, RSM_SIZE / mRSMDownsampleScaleSize, RSM_SIZE / mRSMDownsampleScaleSize);
					commandList->RSSetViewports(1, &rsmDownsampleResViewport);
					commandList->RSSetScissorRects(1, &rsmDownsampleRect);

					commandList->SetPipelineState(mRSMDownsamplePSO.GetPipelineStateObject());
					commandList->SetGraphicsRootSignature(mRSMDownsampleRS.GetSignature());

					D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesRSM[] = {
						mRSMDownsampledBuffersRTs[0]->GetRTV().GetCPUHandle(),
						mRSMDownsampledBuffersRTs[1]->GetRTV().GetCPUHandle(),
						mRSMDownsampledBuffersRTs[2]->GetRTV().GetCPUHandle()
					};

					mSandboxFramework->ResourceBarriersBegin(mBarriers);
					mRSMDownsampledBuffersRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
					mRSMDownsampledBuffersRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
					mRSMDownsampledBuffersRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
					mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

					commandList->OMSetRenderTargets(_countof(rtvHandlesRSM), rtvHandlesRSM, FALSE, nullptr);
					commandList->ClearRenderTargetView(rtvHandlesRSM[0], clearColorBlack, 0, nullptr);
					commandList->ClearRenderTargetView(rtvHandlesRSM[1], clearColorBlack, 0, nullptr);
					commandList->ClearRenderTargetView(rtvHandlesRSM[2], clearColorBlack, 0, nullptr);

					DXRS::DescriptorHandle cbvHandleRSM = gpuDescriptorHeap->GetHandleBlock(1);
					gpuDescriptorHeap->AddToHandle(device, cbvHandleRSM, mRSMDownsampleCB->GetCBV());

					DXRS::DescriptorHandle srvHandleRSM = gpuDescriptorHeap->GetHandleBlock(3);
					gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[0]->GetSRV());
					gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[1]->GetSRV());
					gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[2]->GetSRV());

					commandList->SetGraphicsRootDescriptorTable(0, cbvHandleRSM.GetGPUHandle());
					commandList->SetGraphicsRootDescriptorTable(1, srvHandleRSM.GetGPUHandle());

					commandList->IASetVertexBuffers(0, 1, &mSandboxFramework->GetFullscreenQuadBufferView());
					commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
					commandList->DrawInstanced(4, 1, 0, 0);

					//reset back
					commandList->RSSetViewports(1, &viewport);
					commandList->RSSetScissorRects(1, &rect);
				}
				PIXEndEvent(commandList);
			}
			else if ((!useAsyncCompute || (useAsyncCompute && aQueue == GRAPHICS_QUEUE)) && mRSMDownsampleUseCS) {
				PIXBeginEvent(commandList, 0, "RSM downsample CS for LPV");
				{
					commandList->SetPipelineState(mRSMDownsamplePSO_Compute.GetPipelineStateObject());
					commandList->SetComputeRootSignature(mRSMDownsampleRS_Compute.GetSignature());

					mSandboxFramework->ResourceBarriersBegin(mBarriers);
					mRSMDownsampledBuffersRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					mRSMDownsampledBuffersRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					mRSMDownsampledBuffersRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

					DXRS::DescriptorHandle cbvHandleRSM = gpuDescriptorHeap->GetHandleBlock(1);
					gpuDescriptorHeap->AddToHandle(device, cbvHandleRSM, mRSMDownsampleCB->GetCBV());

					DXRS::DescriptorHandle srvHandleRSM = gpuDescriptorHeap->GetHandleBlock(3);
					gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[0]->GetSRV());
					gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[1]->GetSRV());
					gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[2]->GetSRV());

					DXRS::DescriptorHandle uavHandleRSM = gpuDescriptorHeap->GetHandleBlock(3);
					gpuDescriptorHeap->AddToHandle(device, uavHandleRSM, mRSMDownsampledBuffersRTs[0]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandleRSM, mRSMDownsampledBuffersRTs[1]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandleRSM, mRSMDownsampledBuffersRTs[2]->GetUAV());

					commandList->SetComputeRootDescriptorTable(0, cbvHandleRSM.GetGPUHandle());
					commandList->SetComputeRootDescriptorTable(1, srvHandleRSM.GetGPUHandle());
					commandList->SetComputeRootDescriptorTable(2, uavHandleRSM.GetGPUHandle());

					commandList->Dispatch(DivideByMultiple(static_cast<UINT>(RSM_SIZE / mRSMDownsampleScaleSize), 8u), DivideByMultiple(static_cast<UINT>(RSM_SIZE / mRSMDownsampleScaleSize), 8u), 1u);
				}
				PIXEndEvent(commandList);
			}
		}
	}

	if (mUseRSM) {
		// calculation
		if ((!useAsyncCompute || (useAsyncCompute && aQueue == GRAPHICS_QUEUE)) && !mRSMComputeVersion) {
			PIXBeginEvent(commandList, 0, "RSM main calculation PS");
			{
				CD3DX12_VIEWPORT rsmResViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mRSMRT->GetWidth(), mRSMRT->GetHeight());
				CD3DX12_RECT rsmRect = CD3DX12_RECT(0.0f, 0.0f, mRSMRT->GetWidth(), mRSMRT->GetHeight());
				commandList->RSSetViewports(1, &rsmResViewport);
				commandList->RSSetScissorRects(1, &rsmRect);

				clearRSMRT();

				DXRS::DescriptorHandle cbvHandleRSM = gpuDescriptorHeap->GetHandleBlock(2);
				gpuDescriptorHeap->AddToHandle(device, cbvHandleRSM, mRSMCB->GetCBV());
				gpuDescriptorHeap->AddToHandle(device, cbvHandleRSM, mRSMCB2->GetCBV());

				DXRS::DescriptorHandle srvHandleRSM = gpuDescriptorHeap->GetHandleBlock(5);
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[0]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[1]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mRSMBuffersRTs[2]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mGbufferRTs[2]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mGbufferRTs[1]->GetSRV());

				commandList->SetGraphicsRootDescriptorTable(0, cbvHandleRSM.GetGPUHandle());
				commandList->SetGraphicsRootDescriptorTable(1, srvHandleRSM.GetGPUHandle());

				commandList->IASetVertexBuffers(0, 1, &mSandboxFramework->GetFullscreenQuadBufferView());
				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				commandList->DrawInstanced(4, 1, 0, 0);

				//reset back
				commandList->RSSetViewports(1, &viewport);
				commandList->RSSetScissorRects(1, &rect);
			}
			PIXEndEvent(commandList);
		}
		else if ((!useAsyncCompute || (useAsyncCompute && aQueue == COMPUTE_QUEUE)) && mRSMComputeVersion) {
			PIXBeginEvent(commandList, 0, "RSM main calculation CS");
			{
				commandList->SetPipelineState(mRSMPSO_Compute.GetPipelineStateObject());
				commandList->SetComputeRootSignature(mRSMRS_Compute.GetSignature());

				DXRS::DescriptorHandle cbvHandleRSM = gpuDescriptorHeap->GetHandleBlock(2);
				gpuDescriptorHeap->AddToHandle(device, cbvHandleRSM, mRSMCB->GetCBV());
				gpuDescriptorHeap->AddToHandle(device, cbvHandleRSM, mRSMCB2->GetCBV());

				DXRS::DescriptorHandle srvHandleRSM = gpuDescriptorHeap->GetHandleBlock(5);
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, useAsyncCompute ? mRSMBuffersRTs_CopiesForAsync[0]->GetSRV() : mRSMBuffersRTs[0]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, useAsyncCompute ? mRSMBuffersRTs_CopiesForAsync[1]->GetSRV() : mRSMBuffersRTs[1]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, useAsyncCompute ? mRSMBuffersRTs_CopiesForAsync[2]->GetSRV() : mRSMBuffersRTs[2]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mGbufferRTs[2]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandleRSM, mGbufferRTs[1]->GetSRV());

				DXRS::DescriptorHandle uavHandleRSM = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, uavHandleRSM, mRSMRT->GetUAV());

				commandList->SetComputeRootDescriptorTable(0, cbvHandleRSM.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(1, srvHandleRSM.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(2, uavHandleRSM.GetGPUHandle());

				commandList->Dispatch(DivideByMultiple(static_cast<UINT>(mRSMRT->GetWidth()), 8u), DivideByMultiple(static_cast<UINT>(mRSMRT->GetHeight()), 8u), 1u);
			}
			PIXEndEvent(commandList);
		}

		if ((!useAsyncCompute || (useAsyncCompute && aQueue == COMPUTE_QUEUE)) && mRSMUseUpsampleAndBlur) {
			// upsample & blur
			PIXBeginEvent(commandList, 0, "RSM upsample & blur CS");
			{
				commandList->SetPipelineState(mRSMUpsampleAndBlurPSO.GetPipelineStateObject());
				commandList->SetComputeRootSignature(mRSMUpsampleAndBlurRS.GetSignature());

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mRSMUpsampleAndBlurRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

				DXRS::DescriptorHandle srvHandleBlurRSM = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, srvHandleBlurRSM, mRSMRT->GetSRV());

				DXRS::DescriptorHandle uavHandleBlurRSM = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, uavHandleBlurRSM, mRSMUpsampleAndBlurRT->GetUAV());

				commandList->SetComputeRootDescriptorTable(0, srvHandleBlurRSM.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(1, uavHandleBlurRSM.GetGPUHandle());

				commandList->Dispatch(DivideByMultiple(static_cast<UINT>(mRSMUpsampleAndBlurRT->GetWidth()), 8u), DivideByMultiple(static_cast<UINT>(mRSMUpsampleAndBlurRT->GetHeight()), 8u), 1u);
			}
			PIXEndEvent(commandList);
		}
	}
	//else if (!useAsyncCompute && !computeOnly) //TODO fix for UAV clear
	//	clearRSMRT();
}

void DXRSExampleGIScene::InitLightPropagationVolume(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
{
	// injection
	{
		DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		//red
		mLPVSHColorsRTs.push_back(new DXRSRenderTarget(device, descriptorManager, LPV_DIM, LPV_DIM, format, flags, L"Red SH LPV", LPV_DIM));
		mLPVSHColorsRTs.push_back(new DXRSRenderTarget(device, descriptorManager, LPV_DIM, LPV_DIM, format, flags, L"Green SH LPV", LPV_DIM));
		mLPVSHColorsRTs.push_back(new DXRSRenderTarget(device, descriptorManager, LPV_DIM, LPV_DIM, format, flags, L"Blue SH LPV", LPV_DIM));

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		mLPVInjectionRS.Reset(2, 0);
		mLPVInjectionRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
		mLPVInjectionRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
		mLPVInjectionRS.Finalize(device, L"LPV Injection pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> geometryShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\LPVInjection.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\LPVInjection.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GSMain", "gs_5_1", compileFlags, 0, &geometryShader, &errorBlob));
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\LPVInjection.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

		DXGI_FORMAT formats[3];
		formats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;

		mLPVInjectionPSO.SetRootSignature(mLPVInjectionRS);
		mLPVInjectionPSO.SetRasterizerState(mRasterizerState);
		mLPVInjectionPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
		mLPVInjectionPSO.SetBlendState(mBlendState);
		mLPVInjectionPSO.SetDepthStencilState(mDepthStateDisabled);
		mLPVInjectionPSO.SetInputLayout(0, nullptr);
		mLPVInjectionPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		mLPVInjectionPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		mLPVInjectionPSO.SetGeometryShader(geometryShader->GetBufferPointer(), geometryShader->GetBufferSize());
		mLPVInjectionPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
		mLPVInjectionPSO.Finalize(device);

		//CB
		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(LPVCBData);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

		mLPVCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"LPV Injection Pass CB");

		LPVCBData data = {};

		XMFLOAT3 lpv_max = { LPV_DIM / 2,LPV_DIM / 2,LPV_DIM / 2 };
		XMFLOAT3 lpv_min = { -LPV_DIM / 2,-LPV_DIM / 2,-LPV_DIM / 2 };
		XMVECTOR diag = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&lpv_max), DirectX::XMLoadFloat3(&lpv_min));

		XMFLOAT3 d;
		XMStoreFloat3(&d, diag);
		XMMATRIX scale = DirectX::XMMatrixScaling(1.f / d.x, 1.f / d.y, 1.f / d.z);
		XMMATRIX trans = DirectX::XMMatrixTranslation(-lpv_min.x, -lpv_min.y, -lpv_min.z);

		data.worldToLPV = trans * scale;
		mWorldToLPV = data.worldToLPV;
		memcpy(mLPVCB->Map(), &data, sizeof(data));
	}

	// propagation
	{
		// bundle data
		{
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mLPVPropagationBundle1Allocator)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mLPVPropagationBundle1Allocator.Get(), nullptr, IID_PPV_ARGS(&mLPVPropagationBundle1)));
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&mLPVPropagationBundle2Allocator)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, mLPVPropagationBundle2Allocator.Get(), nullptr, IID_PPV_ARGS(&mLPVPropagationBundle2)));
		}

		DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		//red
		mLPVAccumulationSHColorsRTs.push_back(new DXRSRenderTarget(device, descriptorManager, LPV_DIM, LPV_DIM, format, flags, L"Accumulation Red SH LPV", LPV_DIM));
		mLPVAccumulationSHColorsRTs.push_back(new DXRSRenderTarget(device, descriptorManager, LPV_DIM, LPV_DIM, format, flags, L"Accumulation Green SH LPV", LPV_DIM));
		mLPVAccumulationSHColorsRTs.push_back(new DXRSRenderTarget(device, descriptorManager, LPV_DIM, LPV_DIM, format, flags, L"Accumulation Blue SH LPV", LPV_DIM));

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		mLPVPropagationRS.Reset(1, 0);
		mLPVPropagationRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_ALL);
		mLPVPropagationRS.Finalize(device, L"LPV Propagation pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> geometryShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\LPVPropagation.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\LPVPropagation.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GSMain", "gs_5_1", compileFlags, 0, &geometryShader, &errorBlob));
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\LPVPropagation.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

		DXGI_FORMAT formats[6];
		formats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[3] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[4] = DXGI_FORMAT_R32G32B32A32_FLOAT;
		formats[5] = DXGI_FORMAT_R32G32B32A32_FLOAT;

		mLPVPropagationPSO.SetRootSignature(mLPVPropagationRS);
		mLPVPropagationPSO.SetRasterizerState(mRasterizerState);
		mLPVPropagationPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
		mLPVPropagationPSO.SetBlendState(mBlendStateLPVPropagation); //TODO add additive blending - fix 
		mLPVPropagationPSO.SetDepthStencilState(mDepthStateDisabled);
		mLPVPropagationPSO.SetInputLayout(0, nullptr);
		mLPVPropagationPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		mLPVPropagationPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		mLPVPropagationPSO.SetGeometryShader(geometryShader->GetBufferPointer(), geometryShader->GetBufferSize());
		mLPVPropagationPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
		mLPVPropagationPSO.Finalize(device);
	}
}
void DXRSExampleGIScene::RenderLightPropagationVolume(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap, RenderQueue aQueue, bool useAsyncCompute)
{
	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);

	if (mUseLPV) {
		PIXBeginEvent(commandList, 0, "LPV Injection");
		{
			CD3DX12_VIEWPORT lpvBuffersViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, LPV_DIM, LPV_DIM);
			CD3DX12_RECT lpvRect = CD3DX12_RECT(0.0f, 0.0f, LPV_DIM, LPV_DIM);
			commandList->RSSetViewports(1, &lpvBuffersViewport);
			commandList->RSSetScissorRects(1, &lpvRect);

			commandList->SetPipelineState(mLPVInjectionPSO.GetPipelineStateObject());
			commandList->SetGraphicsRootSignature(mLPVInjectionRS.GetSignature());

			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mLPVSHColorsRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVSHColorsRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVSHColorsRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesLPVInjection[] =
			{
				mLPVSHColorsRTs[0]->GetRTV().GetCPUHandle(),
				mLPVSHColorsRTs[1]->GetRTV().GetCPUHandle(),
				mLPVSHColorsRTs[2]->GetRTV().GetCPUHandle()
			};

			commandList->OMSetRenderTargets(_countof(rtvHandlesLPVInjection), rtvHandlesLPVInjection, FALSE, nullptr);
			commandList->ClearRenderTargetView(rtvHandlesLPVInjection[0], clearColorBlack, 0, nullptr);
			commandList->ClearRenderTargetView(rtvHandlesLPVInjection[1], clearColorBlack, 0, nullptr);
			commandList->ClearRenderTargetView(rtvHandlesLPVInjection[2], clearColorBlack, 0, nullptr);

			DXRS::DescriptorHandle cbvHandleLPVInjection = gpuDescriptorHeap->GetHandleBlock(2);
			gpuDescriptorHeap->AddToHandle(device, cbvHandleLPVInjection, mLPVCB->GetCBV());
			gpuDescriptorHeap->AddToHandle(device, cbvHandleLPVInjection, mRSMDownsampleCB->GetCBV());

			DXRS::DescriptorHandle srvHandleLPVInjection = gpuDescriptorHeap->GetHandleBlock(3);
			gpuDescriptorHeap->AddToHandle(device, srvHandleLPVInjection, (mRSMDownsampleForLPV) ? mRSMDownsampledBuffersRTs[0]->GetSRV() : mRSMBuffersRTs[0]->GetSRV());
			gpuDescriptorHeap->AddToHandle(device, srvHandleLPVInjection, (mRSMDownsampleForLPV) ? mRSMDownsampledBuffersRTs[1]->GetSRV() : mRSMBuffersRTs[1]->GetSRV());
			gpuDescriptorHeap->AddToHandle(device, srvHandleLPVInjection, (mRSMDownsampleForLPV) ? mRSMDownsampledBuffersRTs[2]->GetSRV() : mRSMBuffersRTs[2]->GetSRV());

			commandList->SetGraphicsRootDescriptorTable(0, cbvHandleLPVInjection.GetGPUHandle());
			commandList->SetGraphicsRootDescriptorTable(1, srvHandleLPVInjection.GetGPUHandle());

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

			if (!mRSMDownsampleForLPV)
				commandList->DrawInstanced(RSM_SIZE * RSM_SIZE, 1, 0, 0);
			else
				commandList->DrawInstanced(RSM_SIZE * RSM_SIZE / (mRSMDownsampleScaleSize * mRSMDownsampleScaleSize), 1, 0, 0);

			//reset back
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &rect);
		}
		PIXEndEvent(commandList);

		PIXBeginEvent(commandList, 0, "LPV Propagation");
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesLPVPropagation[] =
			{
				mLPVSHColorsRTs[0]->GetRTV().GetCPUHandle(),
				mLPVSHColorsRTs[1]->GetRTV().GetCPUHandle(),
				mLPVSHColorsRTs[2]->GetRTV().GetCPUHandle(),
				mLPVAccumulationSHColorsRTs[0]->GetRTV().GetCPUHandle(),
				mLPVAccumulationSHColorsRTs[1]->GetRTV().GetCPUHandle(),
				mLPVAccumulationSHColorsRTs[2]->GetRTV().GetCPUHandle()
			};
			CD3DX12_VIEWPORT lpvBuffersViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, LPV_DIM, LPV_DIM);
			CD3DX12_RECT lpvRect = CD3DX12_RECT(0.0f, 0.0f, LPV_DIM, LPV_DIM);
			commandList->RSSetViewports(1, &lpvBuffersViewport);
			commandList->RSSetScissorRects(1, &lpvRect);

			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mLPVSHColorsRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVSHColorsRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVSHColorsRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVAccumulationSHColorsRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVAccumulationSHColorsRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mLPVAccumulationSHColorsRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

			commandList->OMSetRenderTargets(_countof(rtvHandlesLPVPropagation), rtvHandlesLPVPropagation, FALSE, nullptr);
			commandList->OMSetBlendFactor(clearColorWhite);
			commandList->ClearRenderTargetView(rtvHandlesLPVPropagation[3], clearColorBlack, 0, nullptr);
			commandList->ClearRenderTargetView(rtvHandlesLPVPropagation[4], clearColorBlack, 0, nullptr);
			commandList->ClearRenderTargetView(rtvHandlesLPVPropagation[5], clearColorBlack, 0, nullptr);

			// if bundles are used, record 1, then record 2, then use 1, then use 2, etc.
			// we need 2 bundles since GPU descriptor heap is double buffered and a bundle has to share the same GPU descriptor heap with a parent command list
			ID3D12GraphicsCommandList* commandListPropagation = commandList;
			if (mUseBundleForLPVPropagation && !mLPVPropagationBundlesClosed) {
				if (!mLPVPropagationBundle1Closed) 
					commandListPropagation = mLPVPropagationBundle1.Get();
				else if (!mLPVPropagationBundle2Closed) 
					commandListPropagation = mLPVPropagationBundle2.Get();
				else 
					mLPVPropagationBundlesClosed = true;
			}

			ID3D12DescriptorHeap* ppHeaps[] = { gpuDescriptorHeap->GetHeap() };
			commandListPropagation->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			commandListPropagation->SetPipelineState(mLPVPropagationPSO.GetPipelineStateObject());
			commandListPropagation->SetGraphicsRootSignature(mLPVPropagationRS.GetSignature());

			auto srvHandleLPVInjection = gpuDescriptorHeap->GetHandleBlock(3);
			gpuDescriptorHeap->AddToHandle(device, srvHandleLPVInjection, mLPVSHColorsRTs[0]->GetSRV());
			gpuDescriptorHeap->AddToHandle(device, srvHandleLPVInjection, mLPVSHColorsRTs[1]->GetSRV());
			gpuDescriptorHeap->AddToHandle(device, srvHandleLPVInjection, mLPVSHColorsRTs[2]->GetSRV());
			commandListPropagation->SetGraphicsRootDescriptorTable(0, srvHandleLPVInjection.GetGPUHandle());

			//recording a bundle (or just normal command list)
			if (!mUseBundleForLPVPropagation || (mUseBundleForLPVPropagation && !mLPVPropagationBundlesClosed)) {
				for (int step = 0; step < mLPVPropagationSteps; step++) {
					commandListPropagation->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					commandListPropagation->DrawInstanced(3, LPV_DIM, 0, 0);
				}
			}

			//executing a bundle depending on the frame index
			if (mUseBundleForLPVPropagation) {
				if (!mLPVPropagationBundlesClosed && !mLPVPropagationBundle1Closed) {
					mLPVPropagationBundle1.Get()->Close();
					mLPVPropagationBundle1Closed = true;
					mLPVPropagationBundle1UsedGPUHeap = gpuDescriptorHeap;
				}
				else if (!mLPVPropagationBundlesClosed && !mLPVPropagationBundle2Closed) {
					mLPVPropagationBundle2.Get()->Close();
					mLPVPropagationBundle2Closed = true;
					mLPVPropagationBundle2UsedGPUHeap = gpuDescriptorHeap;
				}

				if (mLPVPropagationBundle1UsedGPUHeap == gpuDescriptorHeap)
					commandList->ExecuteBundle(mLPVPropagationBundle1.Get());
				else if (mLPVPropagationBundle2UsedGPUHeap == gpuDescriptorHeap) 
					commandList->ExecuteBundle(mLPVPropagationBundle2.Get());
			}
		}
		PIXEndEvent(commandList);

		//reset back
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &rect);
	}
}

void DXRSExampleGIScene::InitVoxelConeTracing(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
{
	// voxelization
	{
		D3D12_SAMPLER_DESC shadowSampler;
		shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		shadowSampler.MipLODBias = 0;
		shadowSampler.MaxAnisotropy = 16;
		shadowSampler.MinLOD = 0.0f;
		shadowSampler.MaxLOD = D3D12_FLOAT32_MAX;
		shadowSampler.BorderColor[0] = 1.0f;
		shadowSampler.BorderColor[1] = 1.0f;
		shadowSampler.BorderColor[2] = 1.0f;
		shadowSampler.BorderColor[3] = 1.0f;

		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		mVCTVoxelization3DRT = new DXRSRenderTarget(device, descriptorManager, VCT_SCENE_VOLUME_SIZE, VCT_SCENE_VOLUME_SIZE, format, flags, L"Voxelization Scene Data 3D", VCT_SCENE_VOLUME_SIZE);
		mVCTVoxelization3DRT_CopyForAsync = new DXRSRenderTarget(device, descriptorManager, VCT_SCENE_VOLUME_SIZE, VCT_SCENE_VOLUME_SIZE, format, flags, L"Voxelization Scene Data 3D Copy", VCT_SCENE_VOLUME_SIZE);

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		mVCTVoxelizationRS.Reset(3, 1);
		mVCTVoxelizationRS.InitStaticSampler(0, shadowSampler, D3D12_SHADER_VISIBILITY_PIXEL);
		mVCTVoxelizationRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
		mVCTVoxelizationRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTVoxelizationRS[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTVoxelizationRS.Finalize(device, L"VCT voxelization pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> geometryShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingVoxelization.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingVoxelization.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GSMain", "gs_5_1", compileFlags, 0, &geometryShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingVoxelization.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		mVCTVoxelizationPSO.SetRootSignature(mVCTVoxelizationRS);
		mVCTVoxelizationPSO.SetRasterizerState(mRasterizerState);
		mVCTVoxelizationPSO.SetRenderTargetFormats(0, nullptr, DXGI_FORMAT_D32_FLOAT);
		mVCTVoxelizationPSO.SetBlendState(mBlendState);
		mVCTVoxelizationPSO.SetDepthStencilState(mDepthStateDisabled);
		mVCTVoxelizationPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		mVCTVoxelizationPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		mVCTVoxelizationPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		mVCTVoxelizationPSO.SetGeometryShader(geometryShader->GetBufferPointer(), geometryShader->GetBufferSize());
		mVCTVoxelizationPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
		mVCTVoxelizationPSO.Finalize(device);

		//create constant buffer for pass
		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(VCTVoxelizationCBData);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;
		mVCTVoxelizationCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT Voxelization Pass CB");

		VCTVoxelizationCBData data = {};
		float scale = 1.0f;
		data.WorldVoxelCube = XMMatrixScaling(scale, scale, scale);
		data.ViewProjection = mCameraView * mCameraProjection;
		data.ShadowViewProjection = mLightViewProjection;
		data.WorldVoxelScale = mWorldVoxelScale;
		memcpy(mVCTVoxelizationCB->Map(), &data, sizeof(data));
	}

	//debug 
	{
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		mVCTVoxelizationDebugRT = new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT, format, flags, L"Voxelization Debug RT");

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		mVCTVoxelizationDebugRS.Reset(2, 0);
		mVCTVoxelizationDebugRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTVoxelizationDebugRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTVoxelizationDebugRS.Finalize(device, L"VCT voxelization debug pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> geometryShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingVoxelizationDebug.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingVoxelizationDebug.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GSMain", "gs_5_1", compileFlags, 0, &geometryShader, &errorBlob));
		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingVoxelizationDebug.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));

		DXGI_FORMAT formats[1];
		formats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		mVCTVoxelizationDebugPSO.SetRootSignature(mVCTVoxelizationDebugRS);
		mVCTVoxelizationDebugPSO.SetRasterizerState(mRasterizerState);
		mVCTVoxelizationDebugPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
		mVCTVoxelizationDebugPSO.SetBlendState(mBlendState);
		mVCTVoxelizationDebugPSO.SetDepthStencilState(mDepthStateRW);
		mVCTVoxelizationDebugPSO.SetInputLayout(0, nullptr);
		mVCTVoxelizationDebugPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		mVCTVoxelizationDebugPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		mVCTVoxelizationDebugPSO.SetGeometryShader(geometryShader->GetBufferPointer(), geometryShader->GetBufferSize());
		mVCTVoxelizationDebugPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
		mVCTVoxelizationDebugPSO.Finalize(device);
	}

	// aniso mipmapping prepare
	{
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		int size = VCT_SCENE_VOLUME_SIZE >> 1;
		mVCTAnisoMipmappinPrepare3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Prepare X+ 3D", size, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinPrepare3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Prepare X- 3D", size, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinPrepare3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Prepare Y+ 3D", size, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinPrepare3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Prepare Y- 3D", size, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinPrepare3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Prepare Z+ 3D", size, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinPrepare3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Prepare Z- 3D", size, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		mVCTAnisoMipmappingPrepareRS.Reset(3, 0);
		mVCTAnisoMipmappingPrepareRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTAnisoMipmappingPrepareRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTAnisoMipmappingPrepareRS[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 6, D3D12_SHADER_VISIBILITY_ALL);
		mVCTAnisoMipmappingPrepareRS.Finalize(device, L"VCT aniso mipmapping prepare pass compute version RS", rootSignatureFlags);

		ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingAnisoMipmapPrepareCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		mVCTAnisoMipmappingPreparePSO.SetRootSignature(mVCTAnisoMipmappingPrepareRS);
		mVCTAnisoMipmappingPreparePSO.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
		mVCTAnisoMipmappingPreparePSO.Finalize(device);

		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(VCTAnisoMipmappingCBData);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

		mVCTAnisoMipmappingCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping CB");
	}

	// aniso mipmapping main
	{
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		int size = VCT_SCENE_VOLUME_SIZE >> 1;
		mVCTAnisoMipmappinMain3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Main X+ 3D", size, VCT_MIPS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinMain3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Main X- 3D", size, VCT_MIPS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinMain3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Main Y+ 3D", size, VCT_MIPS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinMain3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Main Y- 3D", size, VCT_MIPS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinMain3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Main Z+ 3D", size, VCT_MIPS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		mVCTAnisoMipmappinMain3DRTs.push_back(new DXRSRenderTarget(device, descriptorManager, size, size, format, flags, L"Voxelization Scene Mip Main Z- 3D", size, VCT_MIPS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		mVCTAnisoMipmappingMainRS.Reset(2, 0);
		mVCTAnisoMipmappingMainRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTAnisoMipmappingMainRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 12, D3D12_SHADER_VISIBILITY_ALL);
		mVCTAnisoMipmappingMainRS.Finalize(device, L"VCT aniso mipmapping main pass comptue version RS", rootSignatureFlags);

		ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingAnisoMipmapMainCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		mVCTAnisoMipmappingMainPSO.SetRootSignature(mVCTAnisoMipmappingMainRS);
		mVCTAnisoMipmappingMainPSO.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
		mVCTAnisoMipmappingMainPSO.Finalize(device);

		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(VCTAnisoMipmappingCBData);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

		mVCTAnisoMipmappingMainCB.push_back(new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping main mip 0 CB"));
		mVCTAnisoMipmappingMainCB.push_back(new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping main mip 1 CB"));
		mVCTAnisoMipmappingMainCB.push_back(new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping main mip 2 CB"));
		mVCTAnisoMipmappingMainCB.push_back(new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping main mip 3 CB"));
		mVCTAnisoMipmappingMainCB.push_back(new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping main mip 4 CB"));
		mVCTAnisoMipmappingMainCB.push_back(new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT aniso mip mapping main mip 5 CB"));

	}

	// main 
	{
		DXRSBuffer::Description cbDesc;
		cbDesc.mElementSize = sizeof(VCTMainCBData);
		cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
		cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

		mVCTMainCB = new DXRSBuffer(mSandboxFramework->GetD3DDevice(), descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"VCT main CB");
		
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		mVCTMainRT = new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH * mVCTRTRatio, MAX_SCREEN_HEIGHT * mVCTRTRatio, format, flags, L"VCT Final Output", -1, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		// PS
		{
			mVCTMainRS.Reset(2, 1);
			mVCTMainRS.InitStaticSampler(0, mBilinearSampler, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS.Finalize(device, L"VCT main pass pixel version RS", rootSignatureFlags);

			ComPtr<ID3DBlob> vertexShader;
			ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif

			ID3DBlob* errorBlob = nullptr;

			ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingPS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob));
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}
			ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingPS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob));
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			DXGI_FORMAT formats[1];
			formats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

			mVCTMainPSO.SetRootSignature(mVCTMainRS);
			mVCTMainPSO.SetRasterizerState(mRasterizerState);
			mVCTMainPSO.SetBlendState(mBlendState);
			mVCTMainPSO.SetDepthStencilState(mDepthStateDisabled);
			mVCTMainPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
			mVCTMainPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			mVCTMainPSO.SetRenderTargetFormats(_countof(formats), formats, DXGI_FORMAT_D32_FLOAT);
			mVCTMainPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
			mVCTMainPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
			mVCTMainPSO.Finalize(device);
		}

		// CS
		{
			mVCTMainRS_Compute.Reset(3, 1);
			mVCTMainRS_Compute.InitStaticSampler(0, mBilinearSampler, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS_Compute[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS_Compute[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS_Compute[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
			mVCTMainRS_Compute.Finalize(device, L"VCT main pass compute version RS", rootSignatureFlags);

			ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif
			ID3DBlob* errorBlob = nullptr;

			ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\VoxelConeTracingCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
			if (errorBlob)
			{
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
				errorBlob->Release();
			}

			mVCTMainPSO_Compute.SetRootSignature(mVCTMainRS_Compute);
			mVCTMainPSO_Compute.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
			mVCTMainPSO_Compute.Finalize(device);
		}
	}

	// upsample and blur
	{
		//RTs
		mVCTMainUpsampleAndBlurRT = new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"VCT Main RT Upsampled & Blurred", -1, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//create root signature
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		mVCTMainUpsampleAndBlurRS.Reset(2, 1);
		mVCTMainUpsampleAndBlurRS.InitStaticSampler(0, mBilinearSampler, D3D12_SHADER_VISIBILITY_ALL);
		mVCTMainUpsampleAndBlurRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTMainUpsampleAndBlurRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
		mVCTMainUpsampleAndBlurRS.Finalize(device, L"VCT Main RT Upsample & Blur pass RS", rootSignatureFlags);

		ComPtr<ID3DBlob> computeShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		ID3DBlob* errorBlob = nullptr;

		ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\UpsampleBlurCS.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &errorBlob));
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}

		mVCTMainUpsampleAndBlurPSO.SetRootSignature(mVCTMainUpsampleAndBlurRS);
		mVCTMainUpsampleAndBlurPSO.SetComputeShader(computeShader->GetBufferPointer(), computeShader->GetBufferSize());
		mVCTMainUpsampleAndBlurPSO.Finalize(device);
	}
}
void DXRSExampleGIScene::RenderVoxelConeTracing(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap, RenderQueue aQueue, bool useAsyncCompute)
{
	if (!mUseVCT)
		return;

	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);

	auto clearVCTMainRT = [this, commandList]() {
		commandList->SetPipelineState(mVCTMainPSO.GetPipelineStateObject());
		commandList->SetGraphicsRootSignature(mVCTMainRS.GetSignature());

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesRSM[] = { mVCTMainRT->GetRTV().GetCPUHandle() };
		D3D12_CPU_DESCRIPTOR_HANDLE uavHandlesRSM[] = { mVCTMainUpsampleAndBlurRT->GetUAV().GetCPUHandle() };

		//transition buffers to rendertarget outputs
		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mVCTMainRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

		commandList->OMSetRenderTargets(_countof(rtvHandlesRSM), rtvHandlesRSM, FALSE, nullptr);
		commandList->ClearRenderTargetView(rtvHandlesRSM[0], clearColorBlack, 0, nullptr);

		//TODO fix null GPU Handle in UAV
		//commandList->ClearUnorderedAccessViewFloat(mVCTMainUpsampleAndBlurRT->GetUAV().GetGPUHandle(), mVCTMainUpsampleAndBlurRT->GetUAV().GetCPUHandle(),
		//	mVCTMainUpsampleAndBlurRT->GetResource(), clearColor, 0, nullptr);
	};

	//if (!mVCTEnabled) {
	//	clearVCTMainRT();
	//	return;
	//}

	if (!useAsyncCompute || (useAsyncCompute && aQueue == GRAPHICS_QUEUE)) {
		PIXBeginEvent(commandList, 0, "VCT Voxelization");
		{
			CD3DX12_VIEWPORT vctViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, VCT_SCENE_VOLUME_SIZE, VCT_SCENE_VOLUME_SIZE);
			CD3DX12_RECT vctRect = CD3DX12_RECT(0.0f, 0.0f, VCT_SCENE_VOLUME_SIZE, VCT_SCENE_VOLUME_SIZE);
			commandList->RSSetViewports(1, &vctViewport);
			commandList->RSSetScissorRects(1, &vctRect);
			commandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);

			commandList->SetPipelineState(mVCTVoxelizationPSO.GetPipelineStateObject());
			commandList->SetGraphicsRootSignature(mVCTVoxelizationRS.GetSignature());

			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mVCTVoxelization3DRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

			DXRS::DescriptorHandle cbvHandle;
			DXRS::DescriptorHandle uavHandle;
			DXRS::DescriptorHandle srvHandle;

			for (auto& model : mObjects) {
				RenderObject(model, [this, gpuDescriptorHeap, commandList, &cbvHandle, &uavHandle, &srvHandle, device](U_PTR<DXRSModel>& anObject) {
					cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
					gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTVoxelizationCB->GetCBV());
					gpuDescriptorHeap->AddToHandle(device, cbvHandle, anObject->GetCB()->GetCBV());

					srvHandle = gpuDescriptorHeap->GetHandleBlock(1);
					gpuDescriptorHeap->AddToHandle(device, srvHandle, mShadowDepth->GetSRV());

					uavHandle = gpuDescriptorHeap->GetHandleBlock(1);
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTVoxelization3DRT->GetUAV());

					commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
					commandList->SetGraphicsRootDescriptorTable(1, srvHandle.GetGPUHandle());
					commandList->SetGraphicsRootDescriptorTable(2, uavHandle.GetGPUHandle());

					anObject->Render(commandList);
				});
			}

			//reset back
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &rect);
		}
		PIXEndEvent(commandList);

		if (mVCTRenderDebug) {
			PIXBeginEvent(commandList, 0, "VCT Voxelization Debug");
			{
				commandList->SetPipelineState(mVCTVoxelizationDebugPSO.GetPipelineStateObject());
				commandList->SetGraphicsRootSignature(mVCTVoxelizationDebugRS.GetSignature());

				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesFinal[] =
				{
					 mVCTVoxelizationDebugRT->GetRTV().GetCPUHandle()
				};

				commandList->OMSetRenderTargets(_countof(rtvHandlesFinal), rtvHandlesFinal, FALSE, &mSandboxFramework->GetDepthStencilView());
				commandList->ClearRenderTargetView(rtvHandlesFinal[0], clearColorBlack, 0, nullptr);
				commandList->ClearDepthStencilView(mSandboxFramework->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mVCTVoxelization3DRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

				DXRS::DescriptorHandle cbvHandle;
				DXRS::DescriptorHandle uavHandle;

				cbvHandle = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTVoxelizationCB->GetCBV());

				uavHandle = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTVoxelization3DRT->GetUAV());

				commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
				commandList->SetGraphicsRootDescriptorTable(1, uavHandle.GetGPUHandle());

				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
				commandList->DrawInstanced(VCT_SCENE_VOLUME_SIZE * VCT_SCENE_VOLUME_SIZE * VCT_SCENE_VOLUME_SIZE, 1, 0, 0);
			}
			PIXEndEvent(commandList);
		}
	
	}

	if (!useAsyncCompute || (useAsyncCompute && aQueue == COMPUTE_QUEUE)) {
		PIXBeginEvent(commandList, 0, "VCT Mipmapping prepare CS");
		{
			commandList->SetPipelineState(mVCTAnisoMipmappingPreparePSO.GetPipelineStateObject());
			commandList->SetComputeRootSignature(mVCTAnisoMipmappingPrepareRS.GetSignature());
		
			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mVCTAnisoMipmappinPrepare3DRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[3]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[4]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[5]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);
		
			VCTAnisoMipmappingCBData cbData = {};
			cbData.MipDimension = VCT_SCENE_VOLUME_SIZE >> 1;
			cbData.MipLevel = 0;
			memcpy(mVCTAnisoMipmappingCB->Map(), &cbData, sizeof(cbData));
			DXRS::DescriptorHandle cbvHandle = gpuDescriptorHeap->GetHandleBlock(1);
			gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTAnisoMipmappingCB->GetCBV());
		
			DXRS::DescriptorHandle srvHandle = gpuDescriptorHeap->GetHandleBlock(1);
			gpuDescriptorHeap->AddToHandle(device, srvHandle, useAsyncCompute ? mVCTVoxelization3DRT_CopyForAsync->GetSRV() : mVCTVoxelization3DRT->GetSRV());
		
			DXRS::DescriptorHandle uavHandle = gpuDescriptorHeap->GetHandleBlock(6);
			gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[0]->GetUAV());
			gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[1]->GetUAV());
			gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[2]->GetUAV());
			gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[3]->GetUAV());
			gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[4]->GetUAV());
			gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[5]->GetUAV());
		
			commandList->SetComputeRootDescriptorTable(0, cbvHandle.GetGPUHandle());
			commandList->SetComputeRootDescriptorTable(1, srvHandle.GetGPUHandle());
			commandList->SetComputeRootDescriptorTable(2, uavHandle.GetGPUHandle());
		
			commandList->Dispatch(DivideByMultiple(static_cast<UINT>(cbData.MipDimension), 8u), DivideByMultiple(static_cast<UINT>(cbData.MipDimension), 8u), DivideByMultiple(static_cast<UINT>(cbData.MipDimension), 8u));
		}
		PIXEndEvent(commandList);

		PIXBeginEvent(commandList, 0, "VCT Mipmapping main CS");
		{
			commandList->SetPipelineState(mVCTAnisoMipmappingMainPSO.GetPipelineStateObject());
			commandList->SetComputeRootSignature(mVCTAnisoMipmappingMainRS.GetSignature());

			mSandboxFramework->ResourceBarriersBegin(mBarriers);
			mVCTAnisoMipmappinPrepare3DRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[3]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[4]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinPrepare3DRTs[5]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			mVCTAnisoMipmappinMain3DRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinMain3DRTs[1]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinMain3DRTs[2]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinMain3DRTs[3]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinMain3DRTs[4]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			mVCTAnisoMipmappinMain3DRTs[5]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

			int mipDimension = VCT_SCENE_VOLUME_SIZE >> 1;
			for (int mip = 0; mip < VCT_MIPS; mip++)
			{
				DXRS::DescriptorHandle cbvHandle = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTAnisoMipmappingMainCB[mip]->GetCBV());

				DXRS::DescriptorHandle uavHandle = gpuDescriptorHeap->GetHandleBlock(12);
				if (mip == 0) {
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[0]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[1]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[2]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[3]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[4]->GetUAV());
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinPrepare3DRTs[5]->GetUAV());
				}
				else {
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[0]->GetUAV(mip - 1));
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[1]->GetUAV(mip - 1));
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[2]->GetUAV(mip - 1));
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[3]->GetUAV(mip - 1));
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[4]->GetUAV(mip - 1));
					gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[5]->GetUAV(mip - 1));
				}
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[0]->GetUAV(mip));
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[1]->GetUAV(mip));
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[2]->GetUAV(mip));
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[3]->GetUAV(mip));
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[4]->GetUAV(mip));
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTAnisoMipmappinMain3DRTs[5]->GetUAV(mip));

				VCTAnisoMipmappingCBData cbData = {};
				cbData.MipDimension = mipDimension;
				cbData.MipLevel = mip;
				memcpy(mVCTAnisoMipmappingMainCB[mip]->Map(), &cbData, sizeof(cbData));
				commandList->SetComputeRootDescriptorTable(0, cbvHandle.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(1, uavHandle.GetGPUHandle());

				commandList->Dispatch(DivideByMultiple(static_cast<UINT>(cbData.MipDimension), 8u), DivideByMultiple(static_cast<UINT>(cbData.MipDimension), 8u), DivideByMultiple(static_cast<UINT>(cbData.MipDimension), 8u));
				mipDimension >>= 1;
			}
		}
		PIXEndEvent(commandList);

		if (!mVCTUseMainCompute && !useAsyncCompute) {
			PIXBeginEvent(commandList, 0, "VCT Main PS");
			{
				CD3DX12_VIEWPORT vctResViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mVCTMainRT->GetWidth(), mVCTMainRT->GetHeight());
				CD3DX12_RECT vctRect = CD3DX12_RECT(0.0f, 0.0f, mVCTMainRT->GetWidth(), mVCTMainRT->GetHeight());
				commandList->RSSetViewports(1, &vctResViewport);
				commandList->RSSetScissorRects(1, &vctRect);

				commandList->SetPipelineState(mVCTMainPSO.GetPipelineStateObject());
				commandList->SetGraphicsRootSignature(mVCTMainRS.GetSignature());

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mVCTMainRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesFinal[] =
				{
					 mVCTMainRT->GetRTV().GetCPUHandle()
				};

				commandList->OMSetRenderTargets(_countof(rtvHandlesFinal), rtvHandlesFinal, FALSE, nullptr);
				commandList->ClearRenderTargetView(rtvHandlesFinal[0], clearColorBlack, 0, nullptr);

				DXRS::DescriptorHandle srvHandle = gpuDescriptorHeap->GetHandleBlock(10);
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mGbufferRTs[0]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mGbufferRTs[1]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mGbufferRTs[2]->GetSRV());

				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[0]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[1]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[2]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[3]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[4]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[5]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTVoxelization3DRT->GetSRV());

				DXRS::DescriptorHandle cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTVoxelizationCB->GetCBV());
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTMainCB->GetCBV());

				commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.GetGPUHandle());
				commandList->SetGraphicsRootDescriptorTable(1, srvHandle.GetGPUHandle());

				commandList->IASetVertexBuffers(0, 1, &mSandboxFramework->GetFullscreenQuadBufferView());
				commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				commandList->DrawInstanced(4, 1, 0, 0);

				//reset back
				commandList->RSSetViewports(1, &viewport);
				commandList->RSSetScissorRects(1, &rect);
			}
			PIXEndEvent(commandList);
		}
		else {
			PIXBeginEvent(commandList, 0, "VCT Main CS");
			{
				commandList->SetPipelineState(mVCTMainPSO_Compute.GetPipelineStateObject());
				commandList->SetComputeRootSignature(mVCTMainRS_Compute.GetSignature());

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mVCTMainRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

				DXRS::DescriptorHandle uavHandle = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, uavHandle, mVCTMainRT->GetUAV());

				DXRS::DescriptorHandle srvHandle = gpuDescriptorHeap->GetHandleBlock(10);
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mGbufferRTs[0]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mGbufferRTs[1]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mGbufferRTs[2]->GetSRV());

				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[0]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[1]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[2]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[3]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[4]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, mVCTAnisoMipmappinMain3DRTs[5]->GetSRV());
				gpuDescriptorHeap->AddToHandle(device, srvHandle, useAsyncCompute ? mVCTVoxelization3DRT_CopyForAsync->GetSRV() : mVCTVoxelization3DRT->GetSRV());

				DXRS::DescriptorHandle cbvHandle = gpuDescriptorHeap->GetHandleBlock(2);
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTVoxelizationCB->GetCBV());
				gpuDescriptorHeap->AddToHandle(device, cbvHandle, mVCTMainCB->GetCBV());

				commandList->SetComputeRootDescriptorTable(0, cbvHandle.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(1, srvHandle.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(2, uavHandle.GetGPUHandle());

				commandList->Dispatch(DivideByMultiple(static_cast<UINT>(mVCTMainRT->GetWidth()), 8u), DivideByMultiple(static_cast<UINT>(mVCTMainRT->GetHeight()), 8u), 1u);
			}
			PIXEndEvent(commandList);
		}

		// upsample and blur
		if (mVCTMainRTUseUpsampleAndBlur) {
			PIXBeginEvent(commandList, 0, "VCT Main RT upsample & blur CS");
			{
				commandList->SetPipelineState(mVCTMainUpsampleAndBlurPSO.GetPipelineStateObject());
				commandList->SetComputeRootSignature(mVCTMainUpsampleAndBlurRS.GetSignature());

				mSandboxFramework->ResourceBarriersBegin(mBarriers);
				mVCTMainUpsampleAndBlurRT->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

				DXRS::DescriptorHandle srvHandleBlur = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, srvHandleBlur, mVCTMainRT->GetSRV());

				DXRS::DescriptorHandle uavHandleBlur = gpuDescriptorHeap->GetHandleBlock(1);
				gpuDescriptorHeap->AddToHandle(device, uavHandleBlur, mVCTMainUpsampleAndBlurRT->GetUAV());

				commandList->SetComputeRootDescriptorTable(0, srvHandleBlur.GetGPUHandle());
				commandList->SetComputeRootDescriptorTable(1, uavHandleBlur.GetGPUHandle());

				commandList->Dispatch(DivideByMultiple(static_cast<UINT>(mVCTMainUpsampleAndBlurRT->GetWidth()), 8u), DivideByMultiple(static_cast<UINT>(mVCTMainUpsampleAndBlurRT->GetHeight()), 8u), 1u);
			}
			PIXEndEvent(commandList);
		}
	}
}

void DXRSExampleGIScene::InitLighting(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
{
	//RTs
	mLightingRTs.push_back(new DXRSRenderTarget(device, descriptorManager, MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, L"Lighting"));

	//create root signature
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_SAMPLER_DESC shadowSampler;
	shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	shadowSampler.MipLODBias = 0;
	shadowSampler.MaxAnisotropy = 16;
	shadowSampler.MinLOD = 0.0f;
	shadowSampler.MaxLOD = D3D12_FLOAT32_MAX;
	shadowSampler.BorderColor[0] = 1.0f;
	shadowSampler.BorderColor[1] = 1.0f;
	shadowSampler.BorderColor[2] = 1.0f;
	shadowSampler.BorderColor[3] = 1.0f;

	D3D12_SAMPLER_DESC lpvSampler;
	lpvSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	lpvSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	lpvSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	lpvSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	//lpvSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	lpvSampler.MipLODBias = 0;
	lpvSampler.MaxAnisotropy = 16;
	lpvSampler.MinLOD = 0.0f;
	lpvSampler.MaxLOD = D3D12_FLOAT32_MAX;
	lpvSampler.BorderColor[0] = 0.0f;
	lpvSampler.BorderColor[1] = 0.0f;
	lpvSampler.BorderColor[2] = 0.0f;
	lpvSampler.BorderColor[3] = 0.0f;

	mLightingRS.Reset(2, 3);
	mLightingRS.InitStaticSampler(0, mBilinearSampler, D3D12_SHADER_VISIBILITY_PIXEL);
	mLightingRS.InitStaticSampler(1, shadowSampler, D3D12_SHADER_VISIBILITY_PIXEL);
	mLightingRS.InitStaticSampler(2, lpvSampler, D3D12_SHADER_VISIBILITY_PIXEL);
	mLightingRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 0, 4, D3D12_SHADER_VISIBILITY_ALL);
	mLightingRS[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, D3D12_SHADER_VISIBILITY_PIXEL);
	mLightingRS.Finalize(device, L"Lighting pass RS", rootSignatureFlags);

	//PSO
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	ID3DBlob* errorBlob = nullptr;

	ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\Lighting.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob));
	if (errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
	}

	ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\Lighting.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob));
	if (errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
	}

	DXGI_FORMAT m_rtFormats[1];
	m_rtFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	mLightingPSO.SetRootSignature(mLightingRS);
	mLightingPSO.SetRasterizerState(mRasterizerState);
	mLightingPSO.SetBlendState(mBlendState);
	mLightingPSO.SetDepthStencilState(mDepthStateDisabled);
	mLightingPSO.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
	mLightingPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	mLightingPSO.SetRenderTargetFormats(_countof(m_rtFormats), m_rtFormats, DXGI_FORMAT_D32_FLOAT);
	mLightingPSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
	mLightingPSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
	mLightingPSO.Finalize(device);

	//CB
	DXRSBuffer::Description cbDesc;
	cbDesc.mElementSize = sizeof(LightingCBData);
	cbDesc.mState = D3D12_RESOURCE_STATE_GENERIC_READ;
	cbDesc.mDescriptorType = DXRSBuffer::DescriptorType::CBV;

	mLightingCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"Lighting Pass CB");

	cbDesc.mElementSize = sizeof(LightsInfoCBData);
	mLightsInfoCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"Lights Info CB");

	cbDesc.mElementSize = sizeof(IlluminationFlagsCBData);
	mIlluminationFlagsCB = new DXRSBuffer(device, descriptorManager, mSandboxFramework->GetCommandListGraphics(), cbDesc, L"Illumination Flags CB");
}
void DXRSExampleGIScene::RenderLighting(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap)
{
	CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);
	CD3DX12_RECT rect = CD3DX12_RECT(0.0f, 0.0f, mSandboxFramework->GetOutputSize().right, mSandboxFramework->GetOutputSize().bottom);

	PIXBeginEvent(commandList, 0, "Lighting");
	{
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &rect);

		commandList->SetPipelineState(mLightingPSO.GetPipelineStateObject());
		commandList->SetGraphicsRootSignature(mLightingRS.GetSignature());

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesLighting[] =
		{
			mLightingRTs[0]->GetRTV().GetCPUHandle()
		};

		mSandboxFramework->ResourceBarriersBegin(mBarriers);
		mLightingRTs[0]->TransitionTo(mBarriers, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mSandboxFramework->ResourceBarriersEnd(mBarriers, commandList);

		commandList->OMSetRenderTargets(_countof(rtvHandlesLighting), rtvHandlesLighting, FALSE, nullptr);
		commandList->ClearRenderTargetView(rtvHandlesLighting[0], clearColorBlack, 0, nullptr);

		DXRS::DescriptorHandle cbvHandleLighting = gpuDescriptorHeap->GetHandleBlock(4);
		gpuDescriptorHeap->AddToHandle(device, cbvHandleLighting, mLightingCB->GetCBV());
		gpuDescriptorHeap->AddToHandle(device, cbvHandleLighting, mLightsInfoCB->GetCBV());
		gpuDescriptorHeap->AddToHandle(device, cbvHandleLighting, mLPVCB->GetCBV());
		gpuDescriptorHeap->AddToHandle(device, cbvHandleLighting, mIlluminationFlagsCB->GetCBV());

		DXRS::DescriptorHandle srvHandleLighting = gpuDescriptorHeap->GetHandleBlock(10);
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mGbufferRTs[0]->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mGbufferRTs[1]->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mGbufferRTs[2]->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mDepthStencil->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mShadowDepth->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, (mRSMUseUpsampleAndBlur) ? mRSMUpsampleAndBlurRT->GetSRV() : mRSMRT->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mLPVAccumulationSHColorsRTs[0]->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mLPVAccumulationSHColorsRTs[1]->GetSRV());
		gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mLPVAccumulationSHColorsRTs[2]->GetSRV());

		if (mVCTRenderDebug)
			gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mVCTVoxelizationDebugRT->GetSRV());
		else if (mVCTMainRTUseUpsampleAndBlur)
			gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mVCTMainUpsampleAndBlurRT->GetSRV());
		else
			gpuDescriptorHeap->AddToHandle(device, srvHandleLighting, mVCTMainRT->GetSRV());

		commandList->SetGraphicsRootDescriptorTable(0, cbvHandleLighting.GetGPUHandle());
		commandList->SetGraphicsRootDescriptorTable(1, srvHandleLighting.GetGPUHandle());

		commandList->IASetVertexBuffers(0, 1, &mSandboxFramework->GetFullscreenQuadBufferView());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 1, 0, 0);
	}
	PIXEndEvent(commandList);
}

void DXRSExampleGIScene::InitComposite(ID3D12Device* device, DXRS::DescriptorHeapManager* descriptorManager)
{
	//create root signature
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	mCompositeRS.Reset(1, 0);
	//mCompositeRS.InitStaticSampler(0, SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	mCompositeRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	mCompositeRS.Finalize(device, L"Composite RS", rootSignatureFlags);

	//create pipeline state object
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	ID3DBlob* errorBlob = nullptr;

	ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\Composite.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob));
	if (errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
	}

	ThrowIfFailed(D3DCompileFromFile(mSandboxFramework->GetFilePath(L"content\\shaders\\Composite.hlsl").c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob));

	mCompositePSO = mLightingPSO;

	mCompositePSO.SetRootSignature(mCompositeRS);
	mCompositePSO.SetRenderTargetFormat(mSandboxFramework->GetBackBufferFormat(), DXGI_FORMAT_D32_FLOAT);
	mCompositePSO.SetVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
	mCompositePSO.SetPixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
	mCompositePSO.Finalize(device);
}
void DXRSExampleGIScene::RenderComposite(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXRS::GPUDescriptorHeap* gpuDescriptorHeap)
{
	PIXBeginEvent(commandList, 0, "Composite");
	{
		commandList->SetPipelineState(mCompositePSO.GetPipelineStateObject());
		commandList->SetGraphicsRootSignature(mCompositeRS.GetSignature());

		mSandboxFramework->TransitionMainRT(commandList, D3D12_RESOURCE_STATE_PRESENT);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandlesFinal[] =
		{
			 mSandboxFramework->GetRenderTargetView()
		};
		commandList->OMSetRenderTargets(_countof(rtvHandlesFinal), rtvHandlesFinal, FALSE, nullptr);
		commandList->ClearRenderTargetView(mSandboxFramework->GetRenderTargetView(), Colors::Green, 0, nullptr);
		
		DXRS::DescriptorHandle srvHandleComposite = gpuDescriptorHeap->GetHandleBlock(1);
		gpuDescriptorHeap->AddToHandle(device, srvHandleComposite, mLightingRTs[0]->GetSRV());
		
		commandList->SetGraphicsRootDescriptorTable(0, srvHandleComposite.GetGPUHandle());
		commandList->IASetVertexBuffers(0, 1, &mSandboxFramework->GetFullscreenQuadBufferView());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 1, 0, 0);
	}
	PIXEndEvent(commandList);
}