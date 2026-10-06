//
//	DirectX12 > Generating Sphere [Templete]	
//

#include <windows.h>
#include <WindowsX.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <wrl.h>
#include "resource.h"
using namespace std;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

bool pauseLight = false;
bool renderWireFrame = false;

int g_thetaStep = 18; // Başlangıç dikey adım
int g_phiStep = 36;   // Başlangıç yatay adım
UINT g_activeVertexCount = 0; // Draw komutuna gidecek olan güncel köşe sayısı

// Sınırlar 
const int MIN_STEP = 5;  // En yüksek çözünürlük 
const int MAX_STEP = 90; // En düşük çözünürlük 
const UINT MAX_ALLOWED_VERTICES = 50000; //  maksimum boyut

enum ShapeMode {
	MODE_SPHERE, // 1
	MODE_CYLINDER, // 2
	MODE_CUBE,     // 3
	MODE_PLANE, // 4
	MODE_CONE,// 5
	MODE_TORUS,//6
	MODE_TOPAC// 7 
	

};
ShapeMode g_currentMode = MODE_SPHERE; //default

vector <XMFLOAT3> Ring;
vector <vector<XMFLOAT3>> RINGS;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
};

struct SceneConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
	XMFLOAT4 mLightPos;
	XMFLOAT4 mLightColor;
	XMFLOAT4 mEyePos;
	XMFLOAT4 mMeshColor;
};

XMMATRIX g_World;
XMMATRIX g_View;
XMMATRIX g_Projection;

HINSTANCE m_hinst			= NULL;
HWND m_hwnd					= NULL;
UINT m_width				= 1600;
UINT m_height				= 900;
UINT m_rtvDescriptorSize	= 0;
bool m_useWarpDevice		= false;	// Adapter info.
float rotation				= 0.0;
const UINT FrameCount		= 2;

// Pipeline objects.
D3D12_VIEWPORT						m_viewport;
D3D12_RECT							m_scissorRect;
ComPtr<IDXGISwapChain3>				m_swapChain;
ComPtr<ID3D12Device>				m_device;
ComPtr<ID3D12Resource>				m_renderTargets[FrameCount];
ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
ComPtr<ID3D12CommandQueue>			m_commandQueue;
ComPtr<ID3D12RootSignature>			m_rootSignature;
ComPtr<ID3D12DescriptorHeap>		m_rtvHeap;

ComPtr<ID3D12PipelineState>			m_pipelineStatePhong;
ComPtr<ID3D12PipelineState>			m_pipelineStateSolid;
ComPtr<ID3D12PipelineState>			m_pipelineStateWire;

ComPtr<ID3D12GraphicsCommandList>	m_commandList;
ComPtr<ID3D12Resource>				m_constantBuffer;
SceneConstantBuffer					m_constantBufferData;
UINT8*								m_pCbvDataBegin = NULL;

// Depth/Stencil
ComPtr<ID3D12DescriptorHeap>		m_dsvHeap;
ComPtr<ID3D12Resource>				m_depthStencil;


vector<Vertex> generated_Faces;
ComPtr<ID3D12Resource>				m_vertexBuffer_Faces;
D3D12_VERTEX_BUFFER_VIEW			m_vertexBufferView_Faces;
BYTE* m_MappedData_for_m_vertexBufferView_Faces = nullptr;

// Line (Rotating in Z-axes 180 degrees)
float phi_line = 0.0;
float theta_line = 0.0;
vector<Vertex> rotating_Line;
ComPtr<ID3D12Resource>				m_vertexBuffer_Rotating_Line;
D3D12_VERTEX_BUFFER_VIEW			m_vertexBufferView_Rotating_Line;
BYTE* m_MappedData_for_m_vertexBufferView_Rotating_Line = nullptr;


// Synchronization objects.
UINT								m_frameIndex;
HANDLE								m_fenceEvent;
ComPtr<ID3D12Fence>					m_fence;
UINT64								m_fenceValue;

void OnInit();
void OnUpdate();
void OnRender();
void OnDestroy();
void WaitForPreviousFrame();
void UpdateResolutionInput();


void ThrowIfFailed(HRESULT hr);
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);

float		Distance(XMFLOAT3 V0, XMFLOAT3 V1);
XMFLOAT3	Cross(XMFLOAT3 V0, XMFLOAT3 V1);
float		Dot(XMFLOAT3 V0, XMFLOAT3 V1);
XMFLOAT3	DotS(float f, XMFLOAT3 V1);
XMFLOAT3	Subtract(XMFLOAT3 V1, XMFLOAT3 V0);
XMFLOAT3	Normalize(XMFLOAT3 V0);
XMFLOAT3	Add(XMFLOAT3 V1, XMFLOAT3 V0);


void OnMouseDown(WPARAM btnState, int x, int y);
void OnMouseUp(WPARAM btnState, int x, int y);
void OnMouseMove(WPARAM btnState, int x, int y);
float Clamp(float x, float low, float high);


int		m_nMouseWheelDelta = 0;							// Amount of middle wheel scroll (+/-)

#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_MIDDLE_BUTTON 0x02
#define MOUSE_RIGHT_BUTTON  0x04
#define MOUSE_WHEEL         0x08

float Pi		= 3.1415926535f;
float mTheta	= 3 * XM_PI / 2;
float mPhi		= XM_PI / 2;
float mRadius	= 3.0f;
POINT mLastMousePos;

bool generate = true;
void generateSphere();
void generateCylinder();


void OnInit()
{
	#if defined(_DEBUG)
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
	#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);
		ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc	= {};
	queueDesc.Flags						= D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type						= D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount			= FrameCount;
	swapChainDesc.Width					= m_width;
	swapChainDesc.Height				= m_height;
	swapChainDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count		= 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc	= {};
		rtvHeapDesc.NumDescriptors				= FrameCount;
		rtvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a depth stencil view (DSV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc	= {};
		dsvHeapDesc.NumDescriptors				= 1;
		dsvHeapDesc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
	}

	// Create frame resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineStatePhong.Get(), IID_PPV_ARGS(&m_commandList)));

	// Now we execute the command list to upload the initial assets (triangle data)
	m_commandList->Close();

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForPreviousFrame();
	}

	// Graphics root signature.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		ComPtr<ID3DBlob> pixelShaderSolid;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PS_Solid", "ps_5_0", compileFlags, 0, &pixelShaderSolid, nullptr));

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc	= {};
		psoDesc.InputLayout							= { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature						= m_rootSignature.Get();
		psoDesc.VS									= CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS									= CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState						= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState							= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState					= CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask							= UINT_MAX;
		psoDesc.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets					= 1;
		psoDesc.RTVFormats[0]						= DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat							= DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count					= 1;
		psoDesc.RasterizerState.CullMode			= D3D12_CULL_MODE_NONE;

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStatePhong)));

		// Describe and create the graphics pipeline state object PSO for pixelShaderSolid.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescSolid = psoDesc;
		psoDescSolid.PS = CD3DX12_SHADER_BYTECODE(pixelShaderSolid.Get());
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDescSolid, IID_PPV_ARGS(&m_pipelineStateSolid)));

		// Describe and create the graphics pipeline state object PSO for pixelShaderSolid.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescWire = psoDesc;
		psoDescWire.PS = CD3DX12_SHADER_BYTECODE(pixelShaderSolid.Get());
		psoDescWire.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDescWire, IID_PPV_ARGS(&m_pipelineStateWire)));

	}

	// Create the depth stencil view.
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc	= {};
		depthStencilDesc.Format							= DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags							= D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue		= {};
		depthOptimizedClearValue.Format					= DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth		= 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil	= 0;

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_depthStencil)
		));

		m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create the Vertex Buffer for Generated Sphere
	{
		// Küredeki tesselation'a göre 360000 yeterli olmayabilir....
		const UINT VertexDataSize = 360000 * sizeof(Vertex);
		Vertex V; V.position = XMFLOAT3(0, 0, 0);
		
		for (int i = 0; i < 360000; i++)
		{
			generated_Faces.push_back(V);
		}

		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(VertexDataSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer_Faces)));

		ID3D12Resource* m_vertexBufferUploadHeap;
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,	&CD3DX12_RESOURCE_DESC::Buffer(VertexDataSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,	IID_PPV_ARGS(&m_vertexBufferUploadHeap)));
	
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData		= &generated_Faces[0];
		vertexData.RowPitch		= VertexDataSize;
		vertexData.SlicePitch	= VertexDataSize;

		UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer_Faces.Get(), m_vertexBufferUploadHeap, 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer_Faces.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer view
		m_vertexBufferView_Faces.BufferLocation		= m_vertexBufferUploadHeap->GetGPUVirtualAddress();
		m_vertexBufferView_Faces.StrideInBytes		= sizeof(Vertex);
		m_vertexBufferView_Faces.SizeInBytes		= VertexDataSize;

		m_vertexBufferUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData_for_m_vertexBufferView_Faces));
	}


	// Create the Vertex Buffer for BOX TO BOX INTERSECTIONS
	{
		const UINT VertexDataSize = 6 * sizeof(Vertex);
		Vertex V; V.position = XMFLOAT3(0, 0, 0);
		for (int i = 0; i < 6; i++)
		{
			rotating_Line.push_back(V);
		}

		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(VertexDataSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer_Rotating_Line)));

		ID3D12Resource* m_vertexBufferUploadHeap;
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,	&CD3DX12_RESOURCE_DESC::Buffer(VertexDataSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,	IID_PPV_ARGS(&m_vertexBufferUploadHeap)));
	
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData		= &rotating_Line[0];
		vertexData.RowPitch		= VertexDataSize;
		vertexData.SlicePitch	= VertexDataSize;

		UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer_Rotating_Line.Get(), m_vertexBufferUploadHeap, 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer_Rotating_Line.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer view
		m_vertexBufferView_Rotating_Line.BufferLocation		= m_vertexBufferUploadHeap->GetGPUVirtualAddress();
		m_vertexBufferView_Rotating_Line.StrideInBytes		= sizeof(Vertex);
		m_vertexBufferView_Rotating_Line.SizeInBytes		= VertexDataSize;

		m_vertexBufferUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData_for_m_vertexBufferView_Rotating_Line));
	}


	// Create the constant buffer.
	{
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));

		// Initialize and map the constant buffers. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));

		CD3DX12_RANGE readRange(0, 0);				// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
	}

	m_viewport.Width		= static_cast<float>(m_width);
	m_viewport.Height		= static_cast<float>(m_height);
	m_viewport.MaxDepth		= 1.0f;

	m_scissorRect.right		= static_cast<float>(m_width);
	m_scissorRect.bottom	= static_cast<float>(m_height);

	// Initialize the world matrix
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -5.0f, 0.0f );
	XMVECTOR At  = XMVectorSet( 0.0f, 0.0f,  1.0f, 0.0f );
	XMVECTOR Up  = XMVectorSet( 0.0f, 1.0f,  0.0f, 0.0f );
	g_View		 = XMMatrixLookAtLH(Eye, At, Up);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1600 / (FLOAT)900, 0.01f, 100.0f);

	m_constantBufferData.mWorld			= XMMatrixTranspose(g_World);
	m_constantBufferData.mView			= XMMatrixTranspose(g_View);
	m_constantBufferData.mProjection	= XMMatrixTranspose(g_Projection);
	m_constantBufferData.mEyePos		= XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);
	m_constantBufferData.mLightPos		= XMFLOAT4(3.0f, 3.0f, -3.0f, 0.0f);

	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}


// Update frame-based values.
void OnUpdate()
{
	// Build the view matrix and update contant buffer.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);
	XMVECTOR pos	= XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(pos, target, up);
	m_constantBufferData.mView = XMMatrixTranspose(g_View);
	XMStoreFloat4(&m_constantBufferData.mEyePos, pos);

	// Constant Buffer Settings for Light Source
	if (!pauseLight) rotation += 0.005;
	XMMATRIX mRotate = XMMatrixRotationY(rotation);
	XMMATRIX mTranslate1 = XMMatrixTranslation(-2.0f, 0.0f, 0.0f);
	XMMATRIX mTranslate2 = XMMatrixTranslation(0.0f, 2.0f, -4.0f);

	XMVECTOR xmvLightPos = XMVectorSet(0, 0, 0, 0);
	xmvLightPos = XMVector3Transform(xmvLightPos, mTranslate1);
	xmvLightPos = XMVector3Transform(xmvLightPos, mRotate);
	xmvLightPos = XMVector3Transform(xmvLightPos, mTranslate2);
	XMStoreFloat4(&m_constantBufferData.mLightPos, xmvLightPos);


	if (generate) generateSphere(); generate = false;


	g_World = XMMatrixIdentity();

	// Constant Buffer Settings for Sphere
	m_constantBufferData.mMeshColor  = XMFLOAT4(1, 0, 0, 1); // Red
	m_constantBufferData.mLightColor = XMFLOAT4(1, 1, 1, 1);
	m_constantBufferData.mWorld = XMMatrixTranspose(g_World);
	memcpy(m_pCbvDataBegin + 0 * 256, &m_constantBufferData, sizeof(m_constantBufferData));


	// Constant Buffer Settings for Rotating Line
	
	//theta_line += 1;
	//phi_line += 1;

	//theta_line = 36;
	//phi_line += 0.5;

	XMVECTOR Up_Vector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_World = XMMatrixRotationZ(theta_line * (XM_PI/180.0)) * XMMatrixRotationY(phi_line * (XM_PI/180.0));
	Up_Vector = XMVector3Normalize(XMVector3TransformNormal(Up_Vector, g_World));
	XMFLOAT4 Up_Vector_F4; XMStoreFloat4(&Up_Vector_F4, Up_Vector);
	XMFLOAT3 Up_Vector_F3 = XMFLOAT3(Up_Vector_F4.x, Up_Vector_F4.y, Up_Vector_F4.z);
	rotating_Line[1].position = Up_Vector_F3;
	memcpy(&m_MappedData_for_m_vertexBufferView_Rotating_Line[0], &rotating_Line[0], (rotating_Line.size()) * sizeof(Vertex)); // Bunu yapıyosan World Matrisi XMMatrixIdentity() olsun !

	m_constantBufferData.mMeshColor  = XMFLOAT4(1, 1, 1, 1); // White
	memcpy(m_pCbvDataBegin + 1 * 256, &m_constantBufferData, sizeof(m_constantBufferData));

	UpdateResolutionInput(); // dinamik ayarlama

}


// Render the scene.
void OnRender()
{
	// Command list allocators can only be reset when the associated command lists have finished execution on the GPU; apps should use fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command list, that command list can then be reset at any time and must be before re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineStatePhong.Get()));

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.1f, 0.3f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (renderWireFrame)
		m_commandList->SetPipelineState(m_pipelineStateWire.Get());
	else
		m_commandList->SetPipelineState(m_pipelineStatePhong.Get());

	// Draw Sphere [as Triangulated Faces]
	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + 0 * 256);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView_Faces);
	m_commandList->DrawInstanced(generated_Faces.size(), 1, 0, 0);


	/*m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	m_commandList->SetPipelineState(m_pipelineStateSolid.Get());

	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + 1 * 256);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView_Rotating_Line);
	m_commandList->DrawInstanced(rotating_Line.size(), 1, 0, 0);*/
	
	
	m_commandList->DrawInstanced(g_activeVertexCount, 1, 0, 0); // cizilmesi gereken vertex sayısı 


	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());

	// Execute the command list. 
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}


void WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}


void OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}


_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	InitWindow(hInstance, nCmdShow);

	OnInit();

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			OnUpdate();
			OnRender();
		}
	}

	OnDestroy();

	return (int)msg.wParam;
}


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= L"D3D12Transformations";
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);

	if (!RegisterClassEx(&wcex)) return E_FAIL;

	// Create window
	m_hinst = hInstance;
	RECT rc = { 0, 0, 1600, 900 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateWindow(
		L"D3D12Transformations", L"DirectX12 > Generating Sphere",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, hInstance, NULL);

	if (!m_hwnd) return E_FAIL;

	ShowWindow(m_hwnd, nCmdShow);

	return S_OK;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:

	case WM_MBUTTONDOWN:

	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:

	case WM_MBUTTONUP:

	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEWHEEL:
	{
		m_nMouseWheelDelta += (short)HIWORD(wParam);
		mRadius -= m_nMouseWheelDelta * mRadius * 0.1f / 120.0f; // 120.0'ı arttırınca Zoom'u yavaşlatıyor.

		// Restrict the radius.
		mRadius = Clamp(mRadius, 0.1f, 10.0f);
		m_nMouseWheelDelta = 0;
	}

	break;

	case WM_KEYDOWN:
		switch (wParam)
		{
			case 'P':
				if (pauseLight)
					pauseLight = false;
				else
					pauseLight = true;
				return 0;

			case 'R':
				if (renderWireFrame)
					renderWireFrame = false;
				else
					renderWireFrame = true;
				return 0;

		}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


_Use_decl_annotations_
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}


void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

float Distance(XMFLOAT3 V0, XMFLOAT3 V1)
{
	return sqrt((V0.x - V1.x)*(V0.x - V1.x) + (V0.y - V1.y)*(V0.y - V1.y) + (V0.z - V1.z)*(V0.z - V1.z));
}

XMFLOAT3 Cross(XMFLOAT3 V0, XMFLOAT3 V1)
{
	return
		XMFLOAT3
		(
			V0.y * V1.z - V0.z * V1.y,
			V0.z * V1.x - V0.x * V1.z,
			V0.x * V1.y - V0.y * V1.x
		);
}

float Dot(XMFLOAT3 V0, XMFLOAT3 V1)
{
	return V0.x * V1.x + V0.y * V1.y + V0.z * V1.z;
}

XMFLOAT3 DotS(float f, XMFLOAT3 V1)
{
	return XMFLOAT3( f * V1.x , f * V1.y , f * V1.z );
}

XMFLOAT3 Subtract(XMFLOAT3 V1, XMFLOAT3 V0)
{
	return
		XMFLOAT3
		(
			V1.x - V0.x,
			V1.y - V0.y,
			V1.z - V0.z
		);
}

XMFLOAT3 Add(XMFLOAT3 V1, XMFLOAT3 V0)
{
	return
		XMFLOAT3
		(
			V1.x + V0.x,
			V1.y + V0.y,
			V1.z + V0.z
		);
}

XMFLOAT3 Normalize(XMFLOAT3 V0)
{
	float L = sqrt(V0.x*V0.x + V0.y*V0.y + V0.z*V0.z);
	return
		XMFLOAT3
		(
			V0.x / L,
			V0.y / L,
			V0.z / L
		);
}


void OnMouseDown(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(m_hwnd);
	}
}

void OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(mLastMousePos.x - x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(mLastMousePos.y - y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = Clamp(mPhi, 0.1f, Pi - 0.1f);
	}

	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(mLastMousePos.x - x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(mLastMousePos.y - y));
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float Clamp(float x, float low, float high)
{
	return x < low ? low : (x > high ? high : x);
}


void generateSphere()
{
	RINGS.clear();
	generated_Faces.clear();

	
	int numRings = 180 / g_thetaStep - 1; //enlem
	int numSlices = 360/ g_phiStep; //boylam

	// 1. ÜST KUTUP NOKTASI
	vector<XMFLOAT3> topPole;
	topPole.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
	RINGS.push_back(topPole);

	// 2. ORTA HALKALAR
	for (int t = 1; t <= numRings; ++t)
	{
		float theta = t * g_thetaStep * (XM_PI / 180.0f);
		vector<XMFLOAT3> currentRing;

		for (int p = 0; p < numSlices; ++p)
		{
			float phi = p * g_phiStep * (XM_PI / 180.0f);

			float x = sin(theta) * cos(phi);
			float y = cos(theta);
			float z = sin(theta) * sin(phi);

			currentRing.push_back(XMFLOAT3(x, y, z));
		}
		RINGS.push_back(currentRing);
	}

	// 3. ALT KUTUP NOKTASI
	vector<XMFLOAT3> bottomPole;
	bottomPole.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));
	RINGS.push_back(bottomPole);

	// ------------------ ÜÇGENLERİN OLUŞTURULMASI ------------------
	Vertex V0, V1, V2;

	// A. Üst Kutup
	const vector<XMFLOAT3>& ring1 = RINGS[1];
	for (int i = 0; i < numSlices; i++)
	{
		V0.position = RINGS[0][0];
		V1.position = ring1[(i + 0) % numSlices];
		V2.position = ring1[(i + 1) % numSlices];

		V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;

		generated_Faces.push_back(V0);
		generated_Faces.push_back(V1);
		generated_Faces.push_back(V2);
	}

	// B. Orta Gövde
	for (int j = 1; j < RINGS.size() - 2; j++)
	{
		const vector<XMFLOAT3>& topRing = RINGS[j];
		const vector<XMFLOAT3>& bottomRing = RINGS[j + 1];

		for (int i = 0; i < numSlices; i++)
		{
			//sol ucgen
			V0.position = bottomRing[(i + 0) % numSlices];
			V1.position = topRing[(i + 1) % numSlices];
			V2.position = bottomRing[(i + 1) % numSlices];

			V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;
			generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);

			//sağ ucgen
			V0.position = topRing[(i + 0) % numSlices];
			V1.position = topRing[(i + 1) % numSlices];
			V2.position = bottomRing[(i + 0) % numSlices];

			V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;
			generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);
		}
	}

	// C. Alt Kutup
	const vector<XMFLOAT3>& lastRing = RINGS[RINGS.size() - 2];
	const XMFLOAT3& bottomVertex = RINGS[RINGS.size() - 1][0];
	for (int i = 0; i < numSlices; i++)
	{
		V0.position = lastRing[(i + 0) % numSlices];
		V1.position = bottomVertex;
		V2.position = lastRing[(i + 1) % numSlices];

		V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;

		generated_Faces.push_back(V0);
		generated_Faces.push_back(V1);
		generated_Faces.push_back(V2);
	}

	g_activeVertexCount = generated_Faces.size();

	// Boyut kontrolü
	if (g_activeVertexCount > MAX_ALLOWED_VERTICES)
	{
		OutputDebugStringA("KRITIK HATA: Maksimum vertex limiti asildi!\n");
		__debugbreak();
		return;
	}

	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
} 
void generateCylinder()
{
	generated_Faces.clear();

	// Çözünürlük ve boyut ayarları
	int numSlices = 360 / g_phiStep;
	float radius = 1.0f;       // yarıçap
	float halfHeight = 1.0f;   //  yüksekliğinin yarısı 
	Vertex V0, V1, V2;

	
	// 1. Ust Kutup 
	
	Vertex topCenter;
	topCenter.position = XMFLOAT3(0.0f, halfHeight, 0.0f);
	topCenter.normal = XMFLOAT3(0.0f, 1.0f, 0.0f); 

	for (int i = 0; i < numSlices; i++)
	{
		float phi1 = i * g_phiStep * (XM_PI / 180.0f);
		float phi2 = ((i + 1) % numSlices) * g_phiStep * (XM_PI / 180.0f);

		V0 = topCenter;

		V1.position = XMFLOAT3(radius * cos(phi1), halfHeight, radius * sin(phi1));
		V1.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

		V2.position = XMFLOAT3(radius * cos(phi2), halfHeight, radius * sin(phi2));
		V2.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

		generated_Faces.push_back(V0);
		generated_Faces.push_back(V1);
		generated_Faces.push_back(V2);
	}

	
	// 2.Govde

	for (int i = 0; i < numSlices; i++)
	{
		float phi1 = i * g_phiStep * (XM_PI / 180.0f);
		float phi2 = ((i + 1) % numSlices) * g_phiStep * (XM_PI / 180.0f);

		// X ve Z koordinatları
		float x1 = radius * cos(phi1); float z1 = radius * sin(phi1);
		float x2 = radius * cos(phi2); float z2 = radius * sin(phi2);

		// Gövde Normalleri (Y ekseni sıfır, sadece dışa doğru)
		XMFLOAT3 n1(x1, 0.0f, z1);
		XMFLOAT3 n2(x2, 0.0f, z2);

		// 1. Üçgen (Sol üstten başlar)
		V0.position = XMFLOAT3(x1, halfHeight, z1);  V0.normal = n1;
		V1.position = XMFLOAT3(x2, halfHeight, z2);  V1.normal = n2;
		V2.position = XMFLOAT3(x1, -halfHeight, z1); V2.normal = n1;

		generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);

		// 2. Üçgen (Sağ altı tamamlar)
		V0.position = XMFLOAT3(x1, -halfHeight, z1); V0.normal = n1;
		V1.position = XMFLOAT3(x2, halfHeight, z2);  V1.normal = n2;
		V2.position = XMFLOAT3(x2, -halfHeight, z2); V2.normal = n2;

		generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);
	}

	
	// 3.Alt Kutup
	
	Vertex bottomCenter;
	bottomCenter.position = XMFLOAT3(0.0f, -halfHeight, 0.0f);
	bottomCenter.normal = XMFLOAT3(0.0f, -1.0f, 0.0f); // Dümdüz aşağı bakar

	for (int i = 0; i < numSlices; i++)
	{
		float phi1 = i * g_phiStep * (XM_PI / 180.0f);
		float phi2 = ((i + 1) % numSlices) * g_phiStep * (XM_PI / 180.0f);

		V0 = bottomCenter;

		// DİKKAT: V1 ve V2'nin yerini bilerek değiştirdik! (Winding Order - Culling için)
		// Eğer üst kapaktaki gibi çizersek, alt kapağın yüzü silindirin "içine" bakar.
		V1.position = XMFLOAT3(radius * cos(phi2), -halfHeight, radius * sin(phi2));
		V1.normal = XMFLOAT3(0.0f, -1.0f, 0.0f);

		V2.position = XMFLOAT3(radius * cos(phi1), -halfHeight, radius * sin(phi1));
		V2.normal = XMFLOAT3(0.0f, -1.0f, 0.0f);

		generated_Faces.push_back(V0);
		generated_Faces.push_back(V1);
		generated_Faces.push_back(V2);
	}

	g_activeVertexCount = generated_Faces.size();

	if (g_activeVertexCount > MAX_ALLOWED_VERTICES)
	{
		OutputDebugStringA("KRITIK HATA: Maksimum vertex limiti asildi!\n");
		__debugbreak();
		return;
	}

	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
}

void generateCube()
{
	generated_Faces.clear();

	float w = 1.0f; // Yarı genişlik, yükseklik ve derinlik (Toplam boyut 2x2x2)

	// Yüzey (Face) eklemeyi otomatikleştiren Lambda Fonksiyonu
	// (4 köşe ve o yüzeyin baktığı yönü belirten 1 normal vektörü alır)
	auto addFace = [&](XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, XMFLOAT3 p4, XMFLOAT3 normal)
		{
			Vertex v1, v2, v3, v4;
			v1.position = p1; v1.normal = normal;
			v2.position = p2; v2.normal = normal;
			v3.position = p3; v3.normal = normal;
			v4.position = p4; v4.normal = normal;

			// 1. Üçgen (Sol Üst -> Sağ Üst -> Sol Alt) // Saat Yönü (Clockwise)
			generated_Faces.push_back(v1);
			generated_Faces.push_back(v2);
			generated_Faces.push_back(v3);

			// 2. Üçgen (Sol Alt -> Sağ Üst -> Sağ Alt)
			generated_Faces.push_back(v3);
			generated_Faces.push_back(v2);
			generated_Faces.push_back(v4);
		};

	// DirectX (Left-Handed) Koordinat Sistemine Göre 6 Yüzeyin Tanımlanması

	// 1. Ön Yüz (Front) - Normal kameraya (bize) doğru bakar: -Z
	addFace(XMFLOAT3(-w, w, -w), XMFLOAT3(w, w, -w),
		XMFLOAT3(-w, -w, -w), XMFLOAT3(w, -w, -w), XMFLOAT3(0.0f, 0.0f, -1.0f));

	// 2. Arka Yüz (Back) - Normal ileriye bakar: +Z
	addFace(XMFLOAT3(w, w, w), XMFLOAT3(-w, w, w),
		XMFLOAT3(w, -w, w), XMFLOAT3(-w, -w, w), XMFLOAT3(0.0f, 0.0f, 1.0f));

	// 3. Üst Yüz (Top) - Normal yukarı bakar: +Y
	addFace(XMFLOAT3(-w, w, w), XMFLOAT3(w, w, w),
		XMFLOAT3(-w, w, -w), XMFLOAT3(w, w, -w), XMFLOAT3(0.0f, 1.0f, 0.0f));

	// 4. Alt Yüz (Bottom) - Normal aşağı bakar: -Y
	addFace(XMFLOAT3(-w, -w, -w), XMFLOAT3(w, -w, -w),
		XMFLOAT3(-w, -w, w), XMFLOAT3(w, -w, w), XMFLOAT3(0.0f, -1.0f, 0.0f));

	// 5. Sol Yüz (Left) - Normal sola bakar: -X
	addFace(XMFLOAT3(-w, w, w), XMFLOAT3(-w, w, -w),
		XMFLOAT3(-w, -w, w), XMFLOAT3(-w, -w, -w), XMFLOAT3(-1.0f, 0.0f, 0.0f));

	// 6. Sağ Yüz (Right) - Normal sağa bakar: +X
	addFace(XMFLOAT3(w, w, -w), XMFLOAT3(w, w, w),
		XMFLOAT3(w, -w, -w), XMFLOAT3(w, -w, w), XMFLOAT3(1.0f, 0.0f, 0.0f));


	
	g_activeVertexCount = generated_Faces.size();

	if (g_activeVertexCount > MAX_ALLOWED_VERTICES)
	{
		OutputDebugStringA("KRITIK HATA: Maksimum vertex limiti asildi!\n");
		__debugbreak();
		return;
	}

	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
}

void generatePlane()
{
	generated_Faces.clear();

	float width = 2.0f; // X eksenindeki genişlik
	float depth = 2.0f; // Z eksenindeki derinlik

	int rows = 180 / g_thetaStep;
	int cols = 360 / g_phiStep;

	float dx = width / cols;
	float dz = depth / rows;

	float startX = -width / 2.0f;
	float startZ = depth / 2.0f; // Z ekseninde derinlik (Bize yakın olan taraf pozitiftir)

	Vertex V0, V1, V2;
	XMFLOAT3 normal = XMFLOAT3(0.0f, 1.0f, 0.0f); // Tüm zemin yukarı bakar

	// Grid (Izgara) oluşturma döngüsü
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			// Bir karenin 4 köşesinin koordinatları
			XMFLOAT3 p1(startX + j * dx, 0.0f, startZ - i * dz);
			XMFLOAT3 p2(startX + (j + 1) * dx, 0.0f, startZ - i * dz);
			XMFLOAT3 p3(startX + j * dx, 0.0f, startZ - (i + 1) * dz);
			XMFLOAT3 p4(startX + (j + 1) * dx, 0.0f, startZ - (i + 1) * dz);

			// 1. Üçgen (Sol Üst, Sağ Üst, Sol Alt)
			V0.position = p1; V0.normal = normal;
			V1.position = p2; V1.normal = normal;
			V2.position = p3; V2.normal = normal;
			generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);

			// 2. Üçgen (Sol Alt, Sağ Üst, Sağ Alt)
			V0.position = p3; V0.normal = normal;
			V1.position = p2; V1.normal = normal;
			V2.position = p4; V2.normal = normal;
			generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);
		}
	}

	g_activeVertexCount = generated_Faces.size();
	if (g_activeVertexCount > MAX_ALLOWED_VERTICES)
	{
		OutputDebugStringA("KRITIK HATA: Plane uretiminde Maksimum vertex limiti asildi!\n");
		__debugbreak(); return;
	}
	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
}

void generateCone()
{
	generated_Faces.clear();

	int numSlices = 360 / g_phiStep;
	float radius = 1.0f;
	float halfHeight = 1.0f;

	Vertex V0, V1, V2;

	// Tepe Noktası
	XMFLOAT3 tipPosition(0.0f, halfHeight, 0.0f);

	// Gövdenin Eğimli Normallerini hesaplamak için Y ekseni bileşeni
	float normalY = radius / (halfHeight * 2.0f);

	// 1. GÖVDE (Tepe noktasından taban çemberine inen üçgenler)
	for (int i = 0; i < numSlices; i++)
	{
		float phi1 = i * g_phiStep * (XM_PI / 180.0f);
		float phi2 = ((i + 1) % numSlices) * g_phiStep * (XM_PI / 180.0f);

		// Alt çemberdeki iki nokta
		XMFLOAT3 p1(radius * cos(phi1), -halfHeight, radius * sin(phi1));
		XMFLOAT3 p2(radius * cos(phi2), -halfHeight, radius * sin(phi2));

		// Normaller
		XMVECTOR n1Vec = XMVector3Normalize(XMVectorSet(p1.x, normalY, p1.z, 0.0f));
		XMVECTOR n2Vec = XMVector3Normalize(XMVectorSet(p2.x, normalY, p2.z, 0.0f));

		XMFLOAT3 n1, n2;
		XMStoreFloat3(&n1, n1Vec);
		XMStoreFloat3(&n2, n2Vec);

		// Üçgen (Tepe, Sağ Alt, Sol Alt) - Saat yönü
		V0.position = tipPosition; V0.normal = n1; // Tepe normali alt noktanın normaliyle yumuşatılır
		V1.position = p2;          V1.normal = n2;
		V2.position = p1;          V2.normal = n1;

		generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);
	}

	// 2. ALT KAPAK (Silindirdeki alt kapağın birebir aynısı)
	Vertex bottomCenter;
	bottomCenter.position = XMFLOAT3(0.0f, -halfHeight, 0.0f);
	bottomCenter.normal = XMFLOAT3(0.0f, -1.0f, 0.0f);

	for (int i = 0; i < numSlices; i++)
	{
		float phi1 = i * g_phiStep * (XM_PI / 180.0f);
		float phi2 = ((i + 1) % numSlices) * g_phiStep * (XM_PI / 180.0f);

		V0 = bottomCenter;
		V1.position = XMFLOAT3(radius * cos(phi2), -halfHeight, radius * sin(phi2));
		V1.normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
		V2.position = XMFLOAT3(radius * cos(phi1), -halfHeight, radius * sin(phi1));
		V2.normal = XMFLOAT3(0.0f, -1.0f, 0.0f);

		generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);
	}

	
	g_activeVertexCount = generated_Faces.size();
	if (g_activeVertexCount > MAX_ALLOWED_VERTICES) {
		OutputDebugStringA("KRITIK HATA: Koni uretiminde Maksimum vertex limiti asildi!\n");
		__debugbreak(); return;
	}
	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
}

void generateTorus()
{
	generated_Faces.clear();

	float R = 1.5f; // Halkanın ana yarıçapı (Genişliği)
	float r = 0.5f; // Tüpün kendi kalınlığı

	int ringCount = 360 / g_phiStep;   // Halka etrafındaki dilimler
	int tubeCount = 360 / g_thetaStep; // Tüpün etrafındaki dilimler

	Vertex V0, V1, V2;

	// Yüzey Bölme (Tessellation) mantığıyla ızgara gibi örüyoruz
	for (int i = 0; i < ringCount; i++)
	{
		for (int j = 0; j < tubeCount; j++)
		{
			// Açılar (Radyan cinsinden)
			float phi1 = i * g_phiStep * (XM_PI / 180.0f);
			float phi2 = ((i + 1) % ringCount) * g_phiStep * (XM_PI / 180.0f);

			float theta1 = j * g_thetaStep * (XM_PI / 180.0f);
			float theta2 = ((j + 1) % tubeCount) * g_thetaStep * (XM_PI / 180.0f);

			// --- KÖŞE (VERTEX) POZİSYON VE NORMALLERİNİ HESAPLAYAN LAMBDA ---
			auto getVertex = [&](float phi, float theta) -> Vertex
				{
					Vertex v;
					// Pozisyon Formülü
					v.position.x = (R + r * cos(theta)) * cos(phi);
					v.position.y = r * sin(theta);
					v.position.z = (R + r * cos(theta)) * sin(phi);

					// Normal Formülü (Sadece tüpün merkezinden yüzeye olan yön)
					v.normal.x = cos(theta) * cos(phi);
					v.normal.y = sin(theta);
					v.normal.z = cos(theta) * sin(phi);
					return v;
				};

			// Bir dörtgeni (Quad) oluşturan 4 köşeyi al
			Vertex p1 = getVertex(phi1, theta1); // Sol Alt
			Vertex p2 = getVertex(phi2, theta1); // Sağ Alt
			Vertex p3 = getVertex(phi1, theta2); // Sol Üst
			Vertex p4 = getVertex(phi2, theta2); // Sağ Üst

			// 1. Üçgen (Sol Alt -> Sol Üst -> Sağ Alt)
			generated_Faces.push_back(p1); generated_Faces.push_back(p3); generated_Faces.push_back(p2);

			// 2. Üçgen (Sağ Alt -> Sol Üst -> Sağ Üst)
			generated_Faces.push_back(p2); generated_Faces.push_back(p3); generated_Faces.push_back(p4);
		}
	}

	
	g_activeVertexCount = generated_Faces.size();
	if (g_activeVertexCount > MAX_ALLOWED_VERTICES)
	{
		OutputDebugStringA("KRITIK HATA: Torus uretiminde Maksimum vertex limiti asildi!\n");
		__debugbreak(); return;
	}
	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
}

void generateTopac()
{
	RINGS.clear();
	generated_Faces.clear();


	int numRings = 90 / g_thetaStep - 1; //enlem
	int numSlices = 360 / g_phiStep; //boylam

	// 1. ÜST KUTUP NOKTASI
	vector<XMFLOAT3> topPole;
	topPole.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
	RINGS.push_back(topPole);

	// 2. ORTA HALKALAR
	for (int t = 1; t <= numRings; ++t)
	{
		float theta = t * g_thetaStep * (XM_PI / 180.0f);
		vector<XMFLOAT3> currentRing;

		for (int p = 0; p < numSlices; ++p)
		{
			float phi = p * g_phiStep * (XM_PI / 180.0f);

			float x = sin(theta) * cos(phi);
			float y = cos(theta);
			float z = sin(theta) * sin(phi);

			currentRing.push_back(XMFLOAT3(x, y, z));
		}
		RINGS.push_back(currentRing);
	}

	// 3. ALT KUTUP NOKTASI
	vector<XMFLOAT3> bottomPole;
	bottomPole.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));
	RINGS.push_back(bottomPole);

	// ------------------ ÜÇGENLERİN OLUŞTURULMASI ------------------
	Vertex V0, V1, V2;

	// A. Üst Kutup
	const vector<XMFLOAT3>& ring1 = RINGS[1];
	for (int i = 0; i < numSlices; i++)
	{
		V0.position = RINGS[0][0];
		V1.position = ring1[(i + 0) % numSlices];
		V2.position = ring1[(i + 1) % numSlices];

		V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;

		generated_Faces.push_back(V0);
		generated_Faces.push_back(V1);
		generated_Faces.push_back(V2);
	}

	// B. Orta Gövde
	for (int j = 1; j < RINGS.size() - 2; j++)
	{
		const vector<XMFLOAT3>& topRing = RINGS[j];
		const vector<XMFLOAT3>& bottomRing = RINGS[j + 1];

		for (int i = 0; i < numSlices; i++)
		{
			//sol ucgen
			V0.position = bottomRing[(i + 0) % numSlices];
			V1.position = topRing[(i + 1) % numSlices];
			V2.position = bottomRing[(i + 1) % numSlices];

			V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;
			generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);

			//sağ ucgen
			V0.position = topRing[(i + 0) % numSlices];
			V1.position = topRing[(i + 1) % numSlices];
			V2.position = bottomRing[(i + 0) % numSlices];

			V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;
			generated_Faces.push_back(V0); generated_Faces.push_back(V1); generated_Faces.push_back(V2);
		}
	}

	// C. Alt Kutup
	const vector<XMFLOAT3>& lastRing = RINGS[RINGS.size() - 2];
	const XMFLOAT3& bottomVertex = RINGS[RINGS.size() - 1][0];
	for (int i = 0; i < numSlices; i++)
	{
		V0.position = lastRing[(i + 0) % numSlices];
		V1.position = bottomVertex;
		V2.position = lastRing[(i + 1) % numSlices];

		V0.normal = V0.position; V1.normal = V1.position; V2.normal = V2.position;

		generated_Faces.push_back(V0);
		generated_Faces.push_back(V1);
		generated_Faces.push_back(V2);
	}

	g_activeVertexCount = generated_Faces.size();

	// Boyut kontrolü
	if (g_activeVertexCount > MAX_ALLOWED_VERTICES)
	{
		OutputDebugStringA("KRITIK HATA: Maksimum vertex limiti asildi!\n");
		__debugbreak();
		return;
	}

	memcpy(&m_MappedData_for_m_vertexBufferView_Faces[0], &generated_Faces[0], g_activeVertexCount * sizeof(Vertex));
}



void UpdateResolutionInput()
{
	bool needsUpdate = false;

	if (GetAsyncKeyState('1') & 0x0001)
	{
		if (g_currentMode != MODE_SPHERE)
		{
			g_currentMode = MODE_SPHERE;
			needsUpdate = true;
		}
	}
	else if (GetAsyncKeyState('2') & 0x0001)
	{
		if (g_currentMode != MODE_CYLINDER)
		{
			g_currentMode = MODE_CYLINDER;
			needsUpdate = true;
		}
	}
	else if (GetAsyncKeyState('3') & 0x0001)
	{
		if (g_currentMode != MODE_CUBE)
		{
			g_currentMode = MODE_CUBE;
			needsUpdate = true;
		}
	}

	else if (GetAsyncKeyState('4') & 0x0001)
	{
		if (g_currentMode != MODE_PLANE)
		{
			g_currentMode = MODE_PLANE;
			needsUpdate = true;
		}
	}

	else if (GetAsyncKeyState('5') & 0x0001)
	{
		if (g_currentMode != MODE_CONE)
		{
			g_currentMode = MODE_CONE;
			needsUpdate = true;
		}
	}

	else if (GetAsyncKeyState('6') & 0x0001)
	{
		if (g_currentMode != MODE_TORUS)
		{
			g_currentMode = MODE_TORUS;
			needsUpdate = true;
		}
	}
	else if (GetAsyncKeyState('7') & 0x0001)
	{
		if (g_currentMode != MODE_TOPAC)
		{
			g_currentMode = MODE_TOPAC;
			needsUpdate = true;
		}
	}

	

	if (GetAsyncKeyState('K') & 0x0001)
	{
		if (g_thetaStep > MIN_STEP) { g_thetaStep -= 2; needsUpdate = true; }
		if (g_phiStep > MIN_STEP * 2) { g_phiStep -= 4; needsUpdate = true; }
	}
	else if (GetAsyncKeyState('L') & 0x0001)
	{
		if (g_thetaStep < MAX_STEP) { g_thetaStep += 2; needsUpdate = true; }
		if (g_phiStep < MAX_STEP * 2) { g_phiStep += 4; needsUpdate = true; }
	}

	if (needsUpdate)
	{
		if (g_currentMode == MODE_CYLINDER){generateCylinder();}
		else if (g_currentMode == MODE_SPHERE){generateSphere();}
		else if (g_currentMode == MODE_CUBE){generateCube();}
		else if (g_currentMode == MODE_PLANE) { generatePlane(); }
		else if (g_currentMode == MODE_CONE) { generateCone(); }
		else if (g_currentMode == MODE_TORUS) { generateTorus(); }
		else if (g_currentMode == MODE_TOPAC) { generateTopac(); }


	}
}

