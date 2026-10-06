#include <windows.h>
#include <WindowsX.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include "resource.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 texture;
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

typedef struct { float x, y, z; } VertexType;
typedef struct { int vIndex1, vIndex2, vIndex3; int tIndex1, tIndex2, tIndex3; int nIndex1, nIndex2, nIndex3; } FaceType;

XMMATRIX g_World;
XMMATRIX g_View;
XMMATRIX g_Projection;

HINSTANCE m_hinst = NULL;
HWND m_hwnd = NULL;
UINT m_width = 1600;
UINT m_height = 900;
UINT m_rtvDescriptorSize = 0;
bool m_useWarpDevice = false;
float rotation = 0.0;
const UINT FrameCount = 2;

float translation = 0.2;
XMFLOAT4 Translation1 = { 0.0f, 0.0f, 0.0f, 0.0f };
XMFLOAT4 Translation2 = { 0.0f, 0.0f, 0.0f, 0.0f };
float Pi = 3.1415926535f;

int vertexCount_Box1 = 0;
int vertexCount_Box2 = 0;

bool pauseLight = false;
bool renderWireFrame = true; 

XMMATRIX g_IntersectionSolidMatrix;
bool g_bHasIntersection = false;

D3D12_VIEWPORT m_viewport;
D3D12_RECT m_scissorRect;
ComPtr<IDXGISwapChain3> m_swapChain;
ComPtr<ID3D12Device> m_device;
ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
ComPtr<ID3D12CommandAllocator> m_commandAllocator;
ComPtr<ID3D12CommandQueue> m_commandQueue;
ComPtr<ID3D12RootSignature> m_rootSignature;
ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

ComPtr<ID3D12PipelineState> m_pipelineState_Default;
ComPtr<ID3D12PipelineState> m_pipelineState_Phong;
ComPtr<ID3D12PipelineState> m_pipelineState_WireFrame;
ComPtr<ID3D12GraphicsCommandList> m_commandList;

ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
ComPtr<ID3D12Resource> m_depthStencil;

Vertex* vertices_Model_Box1;
Vertex* vertices_Model_Box2;
Vertex* vertices_Model_Box1_Temp;
Vertex* vertices_Model_Box2_Temp;
DWORD Indices_Box1[36];
DWORD Indices_Box2[36];

XMMATRIX mTranslation_1;
XMMATRIX mTranslation_2;

ComPtr<ID3D12Resource> m_vertexBuffer_Cube_1;
D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView_Cube_1;
BYTE* m_MappedData_for_m_vertexBufferView_Cube_1 = nullptr;
ComPtr<ID3D12Resource> m_indexBuffer_Cube_1;
D3D12_INDEX_BUFFER_VIEW m_indexBufferView_Cube_1;

ComPtr<ID3D12Resource> m_vertexBuffer_Cube_2;
D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView_Cube_2;
BYTE* m_MappedData_for_m_vertexBufferView_Cube_2 = nullptr;
ComPtr<ID3D12Resource> m_indexBuffer_Cube2;
D3D12_INDEX_BUFFER_VIEW m_indexBufferView_Cube2;

ComPtr<ID3D12Resource> m_vertexBuffer_Ground;
D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView_Ground;

ComPtr<ID3D12Resource> m_constantBuffer;
SceneConstantBuffer m_constantBufferData;
UINT8* m_pCbvDataBegin = NULL;

UINT m_frameIndex;
HANDLE m_fenceEvent;
ComPtr<ID3D12Fence> m_fence;
UINT64 m_fenceValue;

float mTheta = 1.5f * XM_PI;
float mPhi = XM_PIDIV4;
float mRadius = 100.0f;
POINT mLastMousePos;

// --- GEOMETRI VE KESISIM DEGISKENLERI ---
vector<XMFLOAT3> g_IntersectionPoints;
XMFLOAT3 Box1_Min, Box1_Max;
XMFLOAT3 Box2_Min, Box2_Max;

void OnInit();
void OnUpdate();
void OnRender();
void OnDestroy();
void WaitForPreviousFrame();
void ThrowIfFailed(HRESULT hr);
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
Vertex* Obj_Loader(char* filename, int* verticesCount, DWORD(&Indices)[36]);
void OnMouseDown(WPARAM btnState, int x, int y);
void OnMouseUp(WPARAM btnState, int x, int y);
void OnMouseMove(WPARAM btnState, int x, int y);
float Clamp(float x, float low, float high);

XMFLOAT3 Cross(XMFLOAT3 V0, XMFLOAT3 V1);
float Dot(XMFLOAT3 V0, XMFLOAT3 V1);
float Distance(XMFLOAT3 V0, XMFLOAT3 V1);
XMFLOAT3 fXMFLOAT3(float f, XMFLOAT3 V);
XMFLOAT3 Subtract(XMFLOAT3 V0, XMFLOAT3 V1);
XMFLOAT3 Sum(XMFLOAT3 V0, XMFLOAT3 V1);
XMFLOAT3 Div(float f, XMFLOAT3 V1);
XMFLOAT3 Mul(float f, XMFLOAT3 V1);
XMFLOAT3 Normalize(XMFLOAT3 V0);
float Length(XMFLOAT3 V0);

// --- KESISIM MATEMATIGI FONKSIYONLARI ---

void AddUniquePoint(XMFLOAT3 p)
{
	for (auto& ep : g_IntersectionPoints) {
		if (Distance(ep, p) < 0.1f) return;
	}
	g_IntersectionPoints.push_back(p);
}

bool IntersectSegmentTriangle(XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 v0, XMFLOAT3 v1, XMFLOAT3 v2, XMFLOAT3& outPoint)
{
	XMVECTOR dir = XMLoadFloat3(&p2) - XMLoadFloat3(&p1);
	float length = XMVectorGetX(XMVector3Length(dir));
	XMVECTOR dirNorm = XMVector3Normalize(dir);

	XMVECTOR vertex0 = XMLoadFloat3(&v0);
	XMVECTOR vertex1 = XMLoadFloat3(&v1);
	XMVECTOR vertex2 = XMLoadFloat3(&v2);

	XMVECTOR edge1 = vertex1 - vertex0;
	XMVECTOR edge2 = vertex2 - vertex0;
	XMVECTOR h = XMVector3Cross(dirNorm, edge2);

	float a = XMVectorGetX(XMVector3Dot(edge1, h));
	if (a > -0.00001f && a < 0.00001f) return false;

	float f = 1.0f / a;
	XMVECTOR s = XMLoadFloat3(&p1) - vertex0;

	// AABB kenar kesişimleri için sınır toleransı eklendi
	float eps = 0.001f;
	float u = f * XMVectorGetX(XMVector3Dot(s, h));
	if (u < -eps || u > 1.0f + eps) return false;

	XMVECTOR q = XMVector3Cross(s, edge1);
	float v = f * XMVectorGetX(XMVector3Dot(dirNorm, q));
	if (v < -eps || u + v > 1.0f + eps) return false;

	float t = f * XMVectorGetX(XMVector3Dot(edge2, q));
	if (t > -eps && t <= length + eps)
	{
		XMVECTOR intersectPoint = XMLoadFloat3(&p1) + dirNorm * t;
		XMStoreFloat3(&outPoint, intersectPoint);
		return true;
	}
	return false;
}

bool IsPointInOBB(XMFLOAT3 pointWorld, XMMATRIX boxWorld, XMFLOAT3 boxMin, XMFLOAT3 boxMax)
{
	XMVECTOR det;
	XMMATRIX invWorld = XMMatrixInverse(&det, boxWorld);
	XMVECTOR localP = XMVector3TransformCoord(XMLoadFloat3(&pointWorld), invWorld);
	XMFLOAT3 p; XMStoreFloat3(&p, localP);

	float eps = 0.01f;
	return (p.x >= boxMin.x - eps && p.x <= boxMax.x + eps &&
		p.y >= boxMin.y - eps && p.y <= boxMax.y + eps &&
		p.z >= boxMin.z - eps && p.z <= boxMax.z + eps);
}

void CalculateIntersections()
{
	g_IntersectionPoints.clear();

	// 1. Point-in-OBB (İçeride Kalan Noktalar)
	for (int i = 0; i < 8; i++) {
		if (IsPointInOBB(vertices_Model_Box1[i].position, mTranslation_2, Box2_Min, Box2_Max))
			AddUniquePoint(vertices_Model_Box1[i].position);

		if (IsPointInOBB(vertices_Model_Box2[i].position, mTranslation_1, Box1_Min, Box1_Max))
			AddUniquePoint(vertices_Model_Box2[i].position);
	}

	// 2. Edge-Face (Kenar-Yüzey Kesişimleri)
	for (int i = 0; i < 36; i += 3) {
		int iA0 = Indices_Box1[i], iA1 = Indices_Box1[i + 1], iA2 = Indices_Box1[i + 2];
		XMFLOAT3 edgesA[3][2] = {
			{vertices_Model_Box1[iA0].position, vertices_Model_Box1[iA1].position},
			{vertices_Model_Box1[iA1].position, vertices_Model_Box1[iA2].position},
			{vertices_Model_Box1[iA2].position, vertices_Model_Box1[iA0].position}
		};
		for (int e = 0; e < 3; e++) {
			for (int j = 0; j < 36; j += 3) {
				XMFLOAT3 v0 = vertices_Model_Box2[Indices_Box2[j]].position;
				XMFLOAT3 v1 = vertices_Model_Box2[Indices_Box2[j + 1]].position;
				XMFLOAT3 v2 = vertices_Model_Box2[Indices_Box2[j + 2]].position;
				XMFLOAT3 outPoint;
				if (IntersectSegmentTriangle(edgesA[e][0], edgesA[e][1], v0, v1, v2, outPoint))
					AddUniquePoint(outPoint);
			}
		}
	}

	for (int i = 0; i < 36; i += 3) {
		int iB0 = Indices_Box2[i], iB1 = Indices_Box2[i + 1], iB2 = Indices_Box2[i + 2];
		XMFLOAT3 edgesB[3][2] = {
			{vertices_Model_Box2[iB0].position, vertices_Model_Box2[iB1].position},
			{vertices_Model_Box2[iB1].position, vertices_Model_Box2[iB2].position},
			{vertices_Model_Box2[iB2].position, vertices_Model_Box2[iB0].position}
		};
		for (int e = 0; e < 3; e++) {
			for (int j = 0; j < 36; j += 3) {
				XMFLOAT3 v0 = vertices_Model_Box1[Indices_Box1[j]].position;
				XMFLOAT3 v1 = vertices_Model_Box1[Indices_Box1[j + 1]].position;
				XMFLOAT3 v2 = vertices_Model_Box1[Indices_Box1[j + 2]].position;
				XMFLOAT3 outPoint;
				if (IntersectSegmentTriangle(edgesB[e][0], edgesB[e][1], v0, v1, v2, outPoint))
					AddUniquePoint(outPoint);
			}
		}
	}
}

void OnInit()
{
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter);
	ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain));
	ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_descriptorHeap)));
	}

	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < FrameCount; n++) {
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState_Phong.Get(), IID_PPV_ARGS(&m_commandList)));

	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		WaitForPreviousFrame();
	}

	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader_Phong;
		ComPtr<ID3DBlob> pixelShader_Solid;
		UINT compileFlags = 0;

		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PS_Phong", "ps_5_0", compileFlags, 0, &pixelShader_Phong, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PS_Solid", "ps_5_0", compileFlags, 0, &pixelShader_Solid, nullptr));

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc_Default = {};
		psoDesc_Default.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc_Default.pRootSignature = m_rootSignature.Get();
		psoDesc_Default.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc_Default.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc_Default.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc_Default.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc_Default.SampleMask = UINT_MAX;
		psoDesc_Default.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc_Default.NumRenderTargets = 1;
		psoDesc_Default.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc_Default.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc_Default.SampleDesc.Count = 1;
		psoDesc_Default.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc_Default, IID_PPV_ARGS(&m_pipelineState_Default)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc_Phong = psoDesc_Default;
		psoDesc_Phong.PS = CD3DX12_SHADER_BYTECODE(pixelShader_Phong.Get());
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc_Phong, IID_PPV_ARGS(&m_pipelineState_Phong)));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc_WireFrame = psoDesc_Default;
		psoDesc_WireFrame.PS = CD3DX12_SHADER_BYTECODE(pixelShader_Phong.Get());
		psoDesc_WireFrame.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc_WireFrame, IID_PPV_ARGS(&m_pipelineState_WireFrame)));
	}

	{
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = 1600;
		depthStencilDesc.Height = 900;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(m_depthStencil.GetAddressOf())));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Texture2D.MipSlice = 0;
		m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Box 1
	{
		vertices_Model_Box1 = Obj_Loader((char*)"Media/Box1.obj", &vertexCount_Box1, Indices_Box1);
		vertices_Model_Box1_Temp = new Vertex[8];
		for (int i = 0; i < 8; i++) vertices_Model_Box1_Temp[i] = vertices_Model_Box1[i];

		const UINT vertexBufferSize = vertexCount_Box1 * sizeof(Vertex);
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer_Cube_1)));
		ID3D12Resource* m_vertexBufferUploadHeap;
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBufferUploadHeap)));

		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = &vertices_Model_Box1[0];
		vertexData.RowPitch = vertexBufferSize;
		vertexData.SlicePitch = vertexBufferSize;

		UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer_Cube_1.Get(), m_vertexBufferUploadHeap, 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer_Cube_1.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		m_vertexBufferView_Cube_1.BufferLocation = m_vertexBufferUploadHeap->GetGPUVirtualAddress();
		m_vertexBufferView_Cube_1.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView_Cube_1.SizeInBytes = vertexBufferSize;
		m_vertexBufferUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData_for_m_vertexBufferView_Cube_1));

		int IndexBufferSize = sizeof(Indices_Box1);
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer_Cube_1)));
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_indexBuffer_Cube_1->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, Indices_Box1, sizeof(Indices_Box1));
		m_indexBuffer_Cube_1->Unmap(0, nullptr);

		m_indexBufferView_Cube_1.BufferLocation = m_indexBuffer_Cube_1->GetGPUVirtualAddress();
		m_indexBufferView_Cube_1.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView_Cube_1.SizeInBytes = IndexBufferSize;
	}

	// Box 2
	{
		vertices_Model_Box2 = Obj_Loader((char*)"Media/Box2.obj", &vertexCount_Box2, Indices_Box2);
		vertices_Model_Box2_Temp = new Vertex[8];
		for (int i = 0; i < 8; i++) vertices_Model_Box2_Temp[i] = vertices_Model_Box2[i];

		const UINT vertexBufferSize = vertexCount_Box2 * sizeof(Vertex);
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer_Cube_2)));
		ID3D12Resource* m_vertexBufferUploadHeap;
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBufferUploadHeap)));

		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = &vertices_Model_Box2[0];
		vertexData.RowPitch = vertexBufferSize;
		vertexData.SlicePitch = vertexBufferSize;

		UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer_Cube_2.Get(), m_vertexBufferUploadHeap, 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer_Cube_2.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		m_vertexBufferView_Cube_2.BufferLocation = m_vertexBufferUploadHeap->GetGPUVirtualAddress();
		m_vertexBufferView_Cube_2.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView_Cube_2.SizeInBytes = vertexBufferSize;
		m_vertexBufferUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData_for_m_vertexBufferView_Cube_2));

		int IndexBufferSize = sizeof(Indices_Box2);
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer_Cube2)));
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_indexBuffer_Cube2->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, Indices_Box2, sizeof(Indices_Box2));
		m_indexBuffer_Cube2->Unmap(0, nullptr);

		m_indexBufferView_Cube2.BufferLocation = m_indexBuffer_Cube2->GetGPUVirtualAddress();
		m_indexBufferView_Cube2.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView_Cube2.SizeInBytes = IndexBufferSize;
	}

	// Ground
	{
		Vertex triangleVertices[] =
		{
			{ XMFLOAT3(48.0, 0.0,  48.0), XMFLOAT3(0.0, 1.0f, 0.0), XMFLOAT2(0.0, 0.0) },
			{ XMFLOAT3(48.0, 0.0, -48.0), XMFLOAT3(0.0, 1.0f, 0.0), XMFLOAT2(0.0, 0.0) },
			{ XMFLOAT3(-48.0, 0.0, -48.0), XMFLOAT3(0.0, 1.0f, 0.0), XMFLOAT2(0.0, 0.0) },
			{ XMFLOAT3(-48.0, 0.0,  48.0), XMFLOAT3(0.0, 1.0f, 0.0), XMFLOAT2(0.0, 0.0) },
			{ XMFLOAT3(48.0, 0.0,  48.0), XMFLOAT3(0.0, 1.0f, 0.0), XMFLOAT2(0.0, 0.0) },
			{ XMFLOAT3(-48.0, 0.0, -48.0), XMFLOAT3(0.0, 1.0f, 0.0), XMFLOAT2(0.0, 0.0) },
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer_Ground)));
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_vertexBuffer_Ground->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer_Ground->Unmap(0, nullptr);

		m_vertexBufferView_Ground.BufferLocation = m_vertexBuffer_Ground->GetGPUVirtualAddress();
		m_vertexBufferView_Ground.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView_Ground.SizeInBytes = vertexBufferSize;
	}

	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	{
		ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));
		ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
	}

	m_viewport.Width = static_cast<float>(m_width);
	m_viewport.Height = static_cast<float>(m_height);
	m_viewport.MaxDepth = 1.0f;
	m_scissorRect.right = static_cast<float>(m_width);
	m_scissorRect.bottom = static_cast<float>(m_height);

	XMVECTOR Eye = XMVectorSet(0.0, 20.0, -30.0, 0.0);
	XMVECTOR At = XMVectorSet(0.0, 0.0, 1.0, 0.0);
	XMVECTOR Up = XMVectorSet(0.0, 1.0, 0.0, 0.0);
	g_View = XMMatrixLookAtLH(Eye, At, Up);
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1600 / (FLOAT)900, 0.01f, 1000.0f);

	m_constantBufferData.mWorld = XMMatrixTranspose(XMMatrixIdentity());
	m_constantBufferData.mView = XMMatrixTranspose(g_View);
	m_constantBufferData.mProjection = XMMatrixTranspose(g_Projection);

	// KUTULARIN MIN/MAX DEGERLERINI HESAPLA (Local Space)
	Box1_Min = XMFLOAT3(9999, 9999, 9999); Box1_Max = XMFLOAT3(-9999, -9999, -9999);
	Box2_Min = XMFLOAT3(9999, 9999, 9999); Box2_Max = XMFLOAT3(-9999, -9999, -9999);

	for (int i = 0; i < 8; i++) {
		Box1_Min.x = min(Box1_Min.x, vertices_Model_Box1_Temp[i].position.x);
		Box1_Min.y = min(Box1_Min.y, vertices_Model_Box1_Temp[i].position.y);
		Box1_Min.z = min(Box1_Min.z, vertices_Model_Box1_Temp[i].position.z);
		Box1_Max.x = max(Box1_Max.x, vertices_Model_Box1_Temp[i].position.x);
		Box1_Max.y = max(Box1_Max.y, vertices_Model_Box1_Temp[i].position.y);
		Box1_Max.z = max(Box1_Max.z, vertices_Model_Box1_Temp[i].position.z);

		Box2_Min.x = min(Box2_Min.x, vertices_Model_Box2_Temp[i].position.x);
		Box2_Min.y = min(Box2_Min.y, vertices_Model_Box2_Temp[i].position.y);
		Box2_Min.z = min(Box2_Min.z, vertices_Model_Box2_Temp[i].position.z);
		Box2_Max.x = max(Box2_Max.x, vertices_Model_Box2_Temp[i].position.x);
		Box2_Max.y = max(Box2_Max.y, vertices_Model_Box2_Temp[i].position.y);
		Box2_Max.z = max(Box2_Max.z, vertices_Model_Box2_Temp[i].position.z);
	}
}

void OnUpdate()
{
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	m_constantBufferData.mView = XMMatrixTranspose(view);
	m_constantBufferData.mEyePos = XMFLOAT4(x, y, z, 0);

	if (!pauseLight) rotation += 0.01;
	XMMATRIX mRotate = XMMatrixRotationY(rotation);
	XMMATRIX mTranslate = XMMatrixTranslation(-35.0f, 60.0f, 0.0f);
	XMVECTOR xmvLightPos = XMVectorSet(0, 0, 0, 0);
	xmvLightPos = XMVector3Transform(xmvLightPos, mTranslate);
	xmvLightPos = XMVector3Transform(xmvLightPos, mRotate);
	XMStoreFloat4(&m_constantBufferData.mLightPos, xmvLightPos);

	m_constantBufferData.mLightColor = XMFLOAT4(1, 1, 1, 1);
	m_constantBufferData.mMeshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_constantBufferData.mWorld = XMMatrixTranspose(XMMatrixIdentity());
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	// Box 1 Kontrolleri
	if (GetAsyncKeyState('A') & 0x8000) Translation1.x -= translation;
	if (GetAsyncKeyState('D') & 0x8000) Translation1.x += translation;
	if (GetAsyncKeyState('W') & 0x8000) Translation1.z += translation;
	if (GetAsyncKeyState('S') & 0x8000) Translation1.z -= translation;
	if (GetAsyncKeyState('Z') & 0x8000) Translation1.y -= translation;
	if (GetAsyncKeyState('X') & 0x8000) Translation1.y += translation;

	mTranslation_1 = XMMatrixTranslation(Translation1.x, Translation1.y, Translation1.z);
	for (int i = 0; i < 8; i++) {
		// W=1 kuralını otomatik uygulayan TransformCoord kullanıldı
		XMVECTOR V = XMLoadFloat3(&vertices_Model_Box1_Temp[i].position);
		V = XMVector3TransformCoord(V, mTranslation_1);
		XMStoreFloat3(&vertices_Model_Box1[i].position, V);
	}
	m_constantBufferData.mWorld = XMMatrixTranspose(mTranslation_1);
	memcpy(m_pCbvDataBegin + 0 * 256, &m_constantBufferData, sizeof(m_constantBufferData));

	// Box 2 Kontrolleri
	if (GetAsyncKeyState('J') & 0x8000) Translation2.x -= translation;
	if (GetAsyncKeyState('L') & 0x8000) Translation2.x += translation;
	if (GetAsyncKeyState('I') & 0x8000) Translation2.z += translation;
	if (GetAsyncKeyState('K') & 0x8000) Translation2.z -= translation;
	if (GetAsyncKeyState('N') & 0x8000) Translation2.y -= translation;
	if (GetAsyncKeyState('M') & 0x8000) Translation2.y += translation;

	mTranslation_2 = XMMatrixTranslation(Translation2.x, Translation2.y, Translation2.z);
	for (int i = 0; i < 8; i++) {
		XMVECTOR V = XMLoadFloat3(&vertices_Model_Box2_Temp[i].position);
		V = XMVector3TransformCoord(V, mTranslation_2);
		XMStoreFloat3(&vertices_Model_Box2[i].position, V);
	}
	m_constantBufferData.mWorld = XMMatrixTranspose(mTranslation_2);
	memcpy(m_pCbvDataBegin + 1 * 256, &m_constantBufferData, sizeof(m_constantBufferData));

	// Zemin
	m_constantBufferData.mWorld = XMMatrixTranspose(XMMatrixIdentity());
	m_constantBufferData.mMeshColor = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	memcpy(m_pCbvDataBegin + 2 * 256, &m_constantBufferData, sizeof(m_constantBufferData));

	// HER KAREDE KESİŞİMLERİ HESAPLA
	CalculateIntersections();

	g_bHasIntersection = false;
	if (g_IntersectionPoints.size() > 0)
	{
		g_bHasIntersection = true;

		// 1. Bulunan tüm noktaların Minimum ve Maksimum sınırlarını bul
		XMFLOAT3 minP = { 99999.0f, 99999.0f, 99999.0f };
		XMFLOAT3 maxP = { -99999.0f, -99999.0f, -99999.0f };

		for (auto& p : g_IntersectionPoints) {
			minP.x = min(minP.x, p.x); minP.y = min(minP.y, p.y); minP.z = min(minP.z, p.z);
			maxP.x = max(maxP.x, p.x); maxP.y = max(maxP.y, p.y); maxP.z = max(maxP.z, p.z);
		}

		// 2. Kesişim Hacminin Boyutunu ve Merkezini hesapla
		XMFLOAT3 intSize = { maxP.x - minP.x, maxP.y - minP.y, maxP.z - minP.z };
		XMFLOAT3 intCenter = { (maxP.x + minP.x) / 2.0f, (maxP.y + minP.y) / 2.0f, (maxP.z + minP.z) / 2.0f };

		// 3. Çizim için kullanacağımız temel modelin (Box 1) orijinal lokal boyutlarını hesapla
		XMFLOAT3 baseSize = { Box1_Max.x - Box1_Min.x, Box1_Max.y - Box1_Min.y, Box1_Max.z - Box1_Min.z };
		XMFLOAT3 baseCenter = { (Box1_Max.x + Box1_Min.x) / 2.0f, (Box1_Max.y + Box1_Min.y) / 2.0f, (Box1_Max.z + Box1_Min.z) / 2.0f };

		// 4. Modele uygulanacak Ölçek (Scale) oranlarını bul
		float sX = (baseSize.x > 0.001f) ? (intSize.x / baseSize.x) : 0.0f;
		float sY = (baseSize.y > 0.001f) ? (intSize.y / baseSize.y) : 0.0f;
		float sZ = (baseSize.z > 0.001f) ? (intSize.z / baseSize.z) : 0.0f;

		// 5. Modelin lokal merkezini, kesişim merkezine tam oturtacak Öteleme (Translation) miktarını hesapla
		float tX = intCenter.x - baseCenter.x * sX;
		float tY = intCenter.y - baseCenter.y * sY;
		float tZ = intCenter.z - baseCenter.z * sZ;

		// 6. Nihai matrisi oluştur ve bellekte tut
		g_IntersectionSolidMatrix = XMMatrixScaling(sX, sY, sZ) * XMMatrixTranslation(tX, tY, tZ);
	}
}

void OnRender()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState_Phong.Get()));
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_descriptorHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->SetGraphicsRootDescriptorTable(1, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());

	if (renderWireFrame) m_commandList->SetPipelineState(m_pipelineState_WireFrame.Get());
	else m_commandList->SetPipelineState(m_pipelineState_Phong.Get());

	// ZEMİN ÇİZİMİ (KATI / SOLID)
	// Zemin için normal, içi dolu render modunu aktif ediyoruz
	m_commandList->SetPipelineState(m_pipelineState_Phong.Get());

	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + 2 * 256);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView_Ground);
	m_commandList->DrawInstanced(6, 1, 0, 0);


	// KUTU 1 ÇİZİMİ (TELKAFES / WIREFRAME)
	// Kutuların içini görebilmek için telkafes moduna geçiyoruz
	m_commandList->SetPipelineState(m_pipelineState_WireFrame.Get());

	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + 0 * 256);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView_Cube_1);
	m_commandList->IASetIndexBuffer(&m_indexBufferView_Cube_1);
	m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);


	// KUTU 2 ÇİZİMİ (TELKAFES / WIREFRAME)
	// PSO zaten WireFrame modunda olduğu için tekrar SetPipelineState çağırmamıza gerek yok, GPU aynı kuralla devam eder
	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + 1 * 256);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView_Cube_2);
	m_commandList->IASetIndexBuffer(&m_indexBufferView_Cube2);
	m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);


	// BULUNAN KESİŞİM HACMİNE KOYU KIRMIZI KUTU ÇİZ (KATI / SOLID)
	if (g_bHasIntersection)
	{
		m_constantBufferData.mWorld = XMMatrixTranspose(g_IntersectionSolidMatrix);
		m_constantBufferData.mMeshColor = XMFLOAT4(0.4f, 0.0f, 0.0f, 1.0f);

		// Kırmızı kutunun içinin dolu olması için tekrar Katı (Phong) render moduna geçiyoruz
		m_commandList->SetPipelineState(m_pipelineState_Phong.Get());

		memcpy(m_pCbvDataBegin + 3 * 256, &m_constantBufferData, sizeof(m_constantBufferData));

		m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + 3 * 256);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView_Cube_1);
		m_commandList->IASetIndexBuffer(&m_indexBufferView_Cube_1);
		m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	}

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(m_commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	ThrowIfFailed(m_swapChain->Present(1, 0));
	WaitForPreviousFrame();
}

void WaitForPreviousFrame()
{
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;
	if (m_fence->GetCompletedValue() < fence) {
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void OnDestroy()
{
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

	MSG msg = { 0 };
	while (WM_QUIT != msg.message) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			OnUpdate();
			OnRender();
		}
	}
	OnDestroy();
	return (int)msg.wParam;
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"DX12 Box to Box";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex)) return E_FAIL;

	m_hinst = hInstance;
	RECT rc = { 0, 0, 1600, 900 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	m_hwnd = CreateWindow(L"DX12 Box to Box", L"DX12 Box to Box Intersection Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
	if (!m_hwnd) return E_FAIL;
	ShowWindow(m_hwnd, nCmdShow);
	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message) {
	case WM_PAINT: hdc = BeginPaint(hWnd, &ps); EndPaint(hWnd, &ps); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN: OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); return 0;
	case WM_LBUTTONUP: case WM_MBUTTONUP: case WM_RBUTTONUP: OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); return 0;
	case WM_MOUSEMOVE: OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); return 0;
	case WM_KEYDOWN:
		switch (wParam) {
		case 'P': pauseLight = !pauseLight; return 0;
		case 'R': renderWireFrame = !renderWireFrame; return 0;
		}
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

_Use_decl_annotations_
void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) break;
	}
	*ppAdapter = adapter.Detach();
}

void ThrowIfFailed(HRESULT hr) { if (FAILED(hr)) throw std::exception(); }
void OnMouseDown(WPARAM btnState, int x, int y) { mLastMousePos.x = x; mLastMousePos.y = y; SetCapture(m_hwnd); }
void OnMouseUp(WPARAM btnState, int x, int y) { ReleaseCapture(); }

void OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f * static_cast<float>(mLastMousePos.x - x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(mLastMousePos.y - y));
		mTheta += dx;
		mPhi += dy;
		mPhi = Clamp(mPhi, 0.1f, Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);
		mRadius += dx - dy;
		mRadius = Clamp(mRadius, 3.0f, 1000.0f);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float Clamp(float x, float low, float high) { return x < low ? low : (x > high ? high : x); }

Vertex* Obj_Loader(char* filename, int* verticesCount, DWORD(&Indices)[36])
{
	ifstream fin; char input;
	int vertexCount = 0, textureCount = 0, normalCount = 0, faceCount = 0;
	fin.open(filename);
	fin.get(input);
	while (!fin.eof()) {
		if (input == 'v') {
			fin.get(input);
			if (input == ' ') vertexCount++;
			if (input == 't') textureCount++;
			if (input == 'n') normalCount++;
		}
		if (input == 'f') { fin.get(input); if (input == ' ') faceCount++; }
		while (input != '\n') fin.get(input);
		fin.get(input);
	}
	fin.close();

	VertexType* vertices = new VertexType[vertexCount];
	VertexType* texcoords = new VertexType[textureCount];
	VertexType* normals = new VertexType[normalCount];
	FaceType* faces = new FaceType[faceCount];

	int vertexIndex = 0, texcoordIndex = 0, normalIndex = 0, faceIndex = 0;
	char input2;
	fin.open(filename);
	fin.get(input);
	while (!fin.eof()) {
		if (input == 'v') {
			fin.get(input);
			if (input == ' ') { fin >> vertices[vertexIndex].x >> vertices[vertexIndex].y >> vertices[vertexIndex].z; vertices[vertexIndex].z *= -1.0f; vertexIndex++; }
			if (input == 't') { fin >> texcoords[texcoordIndex].x >> texcoords[texcoordIndex].y; texcoords[texcoordIndex].y = 1.0f - texcoords[texcoordIndex].y; texcoordIndex++; }
			if (input == 'n') { fin >> normals[normalIndex].x >> normals[normalIndex].y >> normals[normalIndex].z; normals[normalIndex].z *= -1.0f; normalIndex++; }
		}
		if (input == 'f') {
			fin.get(input);
			if (input == ' ') {
				fin >> faces[faceIndex].vIndex3 >> input2 >> faces[faceIndex].tIndex3 >> input2 >> faces[faceIndex].nIndex3
					>> faces[faceIndex].vIndex2 >> input2 >> faces[faceIndex].tIndex2 >> input2 >> faces[faceIndex].nIndex2
					>> faces[faceIndex].vIndex1 >> input2 >> faces[faceIndex].tIndex1 >> input2 >> faces[faceIndex].nIndex1;
				faceIndex++;
			}
		}
		while (input != '\n') fin.get(input);
		fin.get(input);
	}
	fin.close();

	*verticesCount = vertexIndex;
	Vertex* verticesModel = new Vertex[vertexIndex];
	for (int i = 0; i < vertexIndex; i++) {
		verticesModel[i].position.x = vertices[i].x; verticesModel[i].position.y = vertices[i].y; verticesModel[i].position.z = vertices[i].z;
		verticesModel[i].texture.x = texcoords[i].x; verticesModel[i].texture.y = texcoords[i].y;
		verticesModel[i].normal.x = normals[i].x; verticesModel[i].normal.y = normals[i].y; verticesModel[i].normal.z = normals[i].z;
	}

	int k = 0;
	for (int i = 0; i < faceIndex; i++) {
		Indices[k + 0] = faces[i].vIndex1 - 1;
		Indices[k + 1] = faces[i].vIndex2 - 1;
		Indices[k + 2] = faces[i].vIndex3 - 1;
		k += 3;
	}

	for (int i = 0; i < 8; i++) verticesModel[i].normal = XMFLOAT3(0, 1, 0);

	delete[] vertices; delete[] texcoords; delete[] normals; delete[] faces;
	return verticesModel;
}

XMFLOAT3 Cross(XMFLOAT3 V0, XMFLOAT3 V1) { return XMFLOAT3(V0.y * V1.z - V0.z * V1.y, V0.z * V1.x - V0.x * V1.z, V0.x * V1.y - V0.y * V1.x); }
float Dot(XMFLOAT3 V0, XMFLOAT3 V1) { return V0.x * V1.x + V0.y * V1.y + V0.z * V1.z; }
float Distance(XMFLOAT3 V0, XMFLOAT3 V1) { return sqrt((V0.x - V1.x) * (V0.x - V1.x) + (V0.y - V1.y) * (V0.y - V1.y) + (V0.z - V1.z) * (V0.z - V1.z)); }
XMFLOAT3 fXMFLOAT3(float f, XMFLOAT3 V) { return XMFLOAT3(f * V.x, f * V.y, f * V.z); }
XMFLOAT3 Subtract(XMFLOAT3 V0, XMFLOAT3 V1) { return XMFLOAT3(V1.x - V0.x, V1.y - V0.y, V1.z - V0.z); }
XMFLOAT3 Sum(XMFLOAT3 V0, XMFLOAT3 V1) { return XMFLOAT3(V1.x + V0.x, V1.y + V0.y, V1.z + V0.z); }
XMFLOAT3 Div(float f, XMFLOAT3 V1) { return XMFLOAT3(V1.x / f, V1.y / f, V1.z / f); }
XMFLOAT3 Mul(float f, XMFLOAT3 V1) { return XMFLOAT3(f * V1.x, f * V1.y, f * V1.z); }
XMFLOAT3 Normalize(XMFLOAT3 V0) { float L = sqrt(V0.x * V0.x + V0.y * V0.y + V0.z * V0.z); return XMFLOAT3(V0.x / L, V0.y / L, V0.z / L); }
float Length(XMFLOAT3 V0) { return sqrt(V0.x * V0.x + V0.y * V0.y + V0.z * V0.z); }