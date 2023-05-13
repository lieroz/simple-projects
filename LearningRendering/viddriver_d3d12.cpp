#include "viddriver.h"

#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <comdef.h>
#include <array>
#include <format>
#include <system_error>
#include <vector>

#include "display.h"

#if defined(_DEBUG)
#define THROW_IF_FAILED(expr)                                   \
  if (HRESULT hr = expr; FAILED(hr)) {                          \
    _com_error err(hr);                                         \
    std::string errorStr =                                      \
        std::format(#expr " failed: {}\n", err.ErrorMessage()); \
    OutputDebugString(errorStr.c_str());                        \
    throw std::runtime_error(errorStr);                         \
  }

#else
#define THROW_IF_FAILED(expr) expr

#endif

static const char* kRootSignature =
    "#define RS \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "
    "DescriptorTable(CBV(b0, "
    "numDescriptors = 14), SRV(t0, numDescriptors = 128), UAV(u0, "
    "numDescriptors = 8)), DescriptorTable(Sampler(s0, space = 0, "
    "numDescriptors = 16))\"";

extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// Static descriptor heaps, without supporting chunks of ID3D12DescriptorHeap
// TODO: support heap growing
template <D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Size>
struct DescriptorAllocator {
  void Init(Microsoft::WRL::ComPtr<ID3D12Device4> device) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = Type,
        .NumDescriptors = Size,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    THROW_IF_FAILED(
        device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap.GetAddressOf())));

    offsets.reserve(Size);
    incrementSize = device->GetDescriptorHandleIncrementSize(Type);
  }

  std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, uint32_t> Allocate() {
    if (maxOffset == Size && offsets.empty()) {
      throw std::runtime_error(
          std::format("ID3D12DescriptorHeap limit reached: {}",
                      static_cast<uint32_t>(Type)));
    }

    uint32_t offset = 0;
    if (offsets.empty()) {
      offset = maxOffset;
      ++maxOffset;
    } else {
      offset = offsets.back();
      offsets.pop_back();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
        heap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += static_cast<uint64_t>(incrementSize) * offset;

    return {cpuHandle, offset};
  }

  void Release(uint32_t offset) { offsets.push_back(offset); }

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
  std::vector<uint32_t> offsets;
  uint32_t maxOffset = 0;
  uint32_t incrementSize = 0;
};

struct VidDriver::Impl {
  std::weak_ptr<Display> display;

  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  Microsoft::WRL::ComPtr<ID3D12Device4> device;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> presentQueue;
  std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>,
             kMaxGpuFramesInFlight>
      commandAllocators;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> graphicsCommandList;

  Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
  std::array<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>,
                       std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, uint32_t>>,
             kMaxGpuFramesInFlight>
      backBuffers;

  Microsoft::WRL::ComPtr<ID3D12Fence> frameFence;
  Microsoft::WRL::ComPtr<ID3D12Fence> flushFence;

  DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 65536>
      descriptorAllocator_CBV_SRV_UAV;
  DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024>
      descriptorAllocator_SAMPLER;
  DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024>
      descriptorAllocator_RTV;
  DescriptorAllocator<D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1024>
      descriptorAllocator_DSV;

  Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

  // TODO: make reusable
  std::array<std::vector<std::unique_ptr<Buffer>>, kMaxGpuFramesInFlight>
      uploadBuffers;

  std::array<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>,
             kMaxGpuFramesInFlight>
      descriptorHeaps;
};

VidDriver::VidDriver() {}
VidDriver::~VidDriver() {}

struct Shader::Impl {};

Shader::Shader() {
  impl = std::make_unique<Impl>();
}
Shader::~Shader() {}

void VidDriver::InitAPI(std::weak_ptr<Display> display) {
  impl = std::make_unique<Impl>(display);

  uint32_t dxgiFactoryFlags = 0;
#if defined(_DEBUG)
  dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

  {
    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
    THROW_IF_FAILED(
        D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
    debugController->EnableDebugLayer();
    debugController->SetEnableGPUBasedValidation(true);
    debugController->SetEnableSynchronizedCommandQueueValidation(true);
  }
#endif

  THROW_IF_FAILED(CreateDXGIFactory2(
      dxgiFactoryFlags, IID_PPV_ARGS(impl->factory.GetAddressOf())));

  {
    HRESULT hr = S_OK;
    for (uint32_t i = 0;
         (hr = impl->factory->EnumAdapterByGpuPreference(
              i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
              IID_PPV_ARGS(impl->adapter.GetAddressOf()))) == S_OK;
         ++i) {
      DXGI_ADAPTER_DESC1 adapterDesc = {};
      THROW_IF_FAILED(impl->adapter->GetDesc1(&adapterDesc));
      if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue;
      }

      OutputDebugString("Adapter description: ");
      OutputDebugStringW(adapterDesc.Description);

      std::string adapterParamsStr = std::format(
          "\nAdapter memory: sys {}, vid {}, sh {}\n",
          adapterDesc.DedicatedSystemMemory, adapterDesc.DedicatedVideoMemory,
          adapterDesc.SharedSystemMemory);
      OutputDebugString(adapterParamsStr.c_str());

      break;
    }

    if (FAILED(hr) && hr != DXGI_ERROR_NOT_FOUND) {
      throw std::runtime_error(std::format(
          "IDXGIFactory6::EnumAdapterByGpuPreference failed: %d", hr));
    }
  }

  THROW_IF_FAILED(D3D12CreateDevice(impl->adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                                    IID_PPV_ARGS(impl->device.GetAddressOf())));

  D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
  };

  THROW_IF_FAILED(impl->device->CreateCommandQueue(
      &commandQueueDesc, IID_PPV_ARGS(impl->presentQueue.GetAddressOf())));

  for (uint32_t i = 0; i < kMaxGpuFramesInFlight; ++i) {
    THROW_IF_FAILED(impl->device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(impl->commandAllocators[i].GetAddressOf())));
  }

  THROW_IF_FAILED(impl->device->CreateCommandList1(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
      IID_PPV_ARGS(impl->graphicsCommandList.GetAddressOf())));

  THROW_IF_FAILED(impl->graphicsCommandList->Reset(
      impl->commandAllocators[frameNumber].Get(), nullptr))

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
      .Width = display.lock()->Width(),
      .Height = display.lock()->Height(),
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = kMaxGpuFramesInFlight,
      .Scaling = DXGI_SCALING_NONE,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
      .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
      .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
  };

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFSDesc = {
      .Windowed = true,

  };

  THROW_IF_FAILED(impl->factory->CreateSwapChainForHwnd(
      impl->presentQueue.Get(),
      static_cast<HWND>(impl->display.lock()->GetContext()), &swapChainDesc,
      &swapChainFSDesc, nullptr, impl->swapChain.GetAddressOf()));

  THROW_IF_FAILED(impl->device->CreateFence(
      0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(impl->frameFence.GetAddressOf())));

  THROW_IF_FAILED(impl->device->CreateFence(
      0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(impl->flushFence.GetAddressOf())));

  impl->descriptorAllocator_CBV_SRV_UAV.Init(impl->device);
  impl->descriptorAllocator_SAMPLER.Init(impl->device);
  impl->descriptorAllocator_RTV.Init(impl->device);
  impl->descriptorAllocator_DSV.Init(impl->device);

  THROW_IF_FAILED(DxcCreateInstance(
      CLSID_DxcCompiler, IID_PPV_ARGS(impl->compiler.GetAddressOf())));

  std::vector<const wchar_t*> args = {L"-E RS", L"-T rootsig_1_1"};
  std::unique_ptr<Shader> rootSignatureShader =
      CompileShaderFromSource(kRootSignature, args);

  THROW_IF_FAILED(impl->device->CreateRootSignature(
      0, rootSignatureShader->binary.data(), rootSignatureShader->binary.size(),
      IID_PPV_ARGS(impl->rootSignature.GetAddressOf())));

  D3D12_DESCRIPTOR_HEAP_DESC desc = {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 166,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
  };
  for (uint32_t i = 0; i < kMaxGpuFramesInFlight; ++i) {
    THROW_IF_FAILED(impl->device->CreateDescriptorHeap(
        &desc, IID_PPV_ARGS(impl->descriptorHeaps[i].GetAddressOf())));
  }

  CreateBackBuffers();
}

void VidDriver::ResizeSwapChain(uint32_t width, uint32_t height) {
  FlushAndWait();

  for (uint32_t i = 0; i < kMaxGpuFramesInFlight; ++i) {
    auto& [backBuffer, pair] = impl->backBuffers[i];
    backBuffer.Reset();
    impl->descriptorAllocator_RTV.Release(pair.second);
  }

  THROW_IF_FAILED(impl->swapChain->ResizeBuffers(
      kMaxGpuFramesInFlight, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

  CreateBackBuffers();
}

void VidDriver::CreateBackBuffers() {
  for (uint32_t i = 0; i < kMaxGpuFramesInFlight; ++i) {
    auto [cpuHandle, offset] = impl->descriptorAllocator_RTV.Allocate();
    auto& [backBuffer, pair] = impl->backBuffers[i];
    pair = {cpuHandle, offset};

    THROW_IF_FAILED(
        impl->swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffer.GetAddressOf())));
    impl->device->CreateRenderTargetView(backBuffer.Get(), nullptr, cpuHandle);
  }
}

void VidDriver::BeginFrame() {
  impl->graphicsCommandList->SetGraphicsRootSignature(
      impl->rootSignature.Get());
}

void VidDriver::Present() {
  const uint32_t bufferIndex = frameNumber % kMaxGpuFramesInFlight;
  D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition = {
          .pResource = impl->backBuffers[bufferIndex].first.Get(),
          .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
          .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
          .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
      }};

  impl->graphicsCommandList->ResourceBarrier(1, &barrier);

  ID3D12CommandList* commandLists[1] = {impl->graphicsCommandList.Get()};
  THROW_IF_FAILED(impl->graphicsCommandList->Close());
  impl->presentQueue->ExecuteCommandLists(1, commandLists);
  THROW_IF_FAILED(impl->swapChain->Present(1, 0));
}

void VidDriver::EndFrame() {
  const uint64_t currentFrameNumber = frameNumber;
  const uint64_t nextFrameNumber = ++frameNumber;

  THROW_IF_FAILED(
      impl->presentQueue->Signal(impl->frameFence.Get(), nextFrameNumber));
  if (impl->frameFence->GetCompletedValue() < currentFrameNumber) {
    THROW_IF_FAILED(
        impl->frameFence->SetEventOnCompletion(currentFrameNumber, nullptr));
  }

  const uint32_t bufferIndex = nextFrameNumber % kMaxGpuFramesInFlight;
  THROW_IF_FAILED(impl->commandAllocators[bufferIndex]->Reset());
  THROW_IF_FAILED(impl->graphicsCommandList->Reset(
      impl->commandAllocators[bufferIndex].Get(), nullptr));

  impl->uploadBuffers[bufferIndex].clear();
}

Buffer::Buffer() {
  impl = std::make_unique<Impl>();
}
Buffer::~Buffer() {}

struct Buffer::Impl {
  void CreateBuffer(Microsoft::WRL::ComPtr<ID3D12Device4> device,
                    uint64_t sizeInBytes,
                    D3D12_HEAP_TYPE heapType) {
    D3D12_RESOURCE_DESC resourceDesc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = sizeInBytes,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc =
            {
                .Count = 1,
                .Quality = 0,
            },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    D3D12_HEAP_PROPERTIES heapProps = {
        .Type = heapType,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 1,
        .VisibleNodeMask = 1,
    };

    THROW_IF_FAILED(device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(resource.GetAddressOf())));
  }

  Microsoft::WRL::ComPtr<ID3D12Resource2> resource;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
};

VertexBuffer::VertexBuffer() {
  impl = std::make_unique<Impl>();
}
VertexBuffer::~VertexBuffer() {}

struct VertexBuffer::Impl {
  D3D12_VERTEX_BUFFER_VIEW bufferView;
};

IndexBuffer::IndexBuffer() {
  impl = std::make_unique<Impl>();
}
IndexBuffer::~IndexBuffer() {}

struct IndexBuffer::Impl {
  D3D12_INDEX_BUFFER_VIEW bufferView;
};

ConstantBuffer::ConstantBuffer() {
  impl = std::make_unique<Impl>();
}
ConstantBuffer::~ConstantBuffer() {}

struct ConstantBuffer::Impl {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
  uint32_t descriptorOffset = 0;
};

ShaderResourceViewBuffer::ShaderResourceViewBuffer() {
  impl = std::make_unique<Impl>();
}
ShaderResourceViewBuffer::~ShaderResourceViewBuffer() {}

struct ShaderResourceViewBuffer::Impl {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
  uint32_t descriptorOffset = 0;
};

void VidDriver::UploadBuffer(void* data,
                             uint64_t sizeInBytes,
                             std::reference_wrapper<Buffer> dstBuffer) {
  const uint32_t bufferIndex = frameNumber % kMaxGpuFramesInFlight;
  Buffer& uploadBuffer = *impl->uploadBuffers[bufferIndex]
                              .emplace_back(std::make_unique<Buffer>())
                              .get();
  uploadBuffer.impl->CreateBuffer(impl->device, sizeInBytes,
                                  D3D12_HEAP_TYPE_UPLOAD);

  D3D12_RANGE range = {
      .Begin = 0,
      .End = 0,
  };
  uint8_t* dataBegin;
  THROW_IF_FAILED(uploadBuffer.impl->resource->Map(
      0, &range, reinterpret_cast<void**>(&dataBegin)));
  memcpy(dataBegin, data, sizeInBytes);
  uploadBuffer.impl->resource->Unmap(0, nullptr);

  std::array<D3D12_RESOURCE_BARRIER, 2> barriers = {
      D3D12_RESOURCE_BARRIER{
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition =
              {
                  .pResource = uploadBuffer.impl->resource.Get(),
                  .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                  .StateBefore = uploadBuffer.impl->state,
                  .StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
              },
      },
      D3D12_RESOURCE_BARRIER{
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition =
              {
                  .pResource = dstBuffer.get().impl->resource.Get(),
                  .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                  .StateBefore = dstBuffer.get().impl->state,
                  .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
              },
      },
  };

  impl->graphicsCommandList->ResourceBarrier(2, barriers.data());
  impl->graphicsCommandList->CopyResource(dstBuffer.get().impl->resource.Get(),
                                          uploadBuffer.impl->resource.Get());

  barriers = {
      D3D12_RESOURCE_BARRIER{
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition =
              {
                  .pResource = uploadBuffer.impl->resource.Get(),
                  .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                  .StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
                  .StateAfter = uploadBuffer.impl->state,
              },
      },
      D3D12_RESOURCE_BARRIER{
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition =
              {
                  .pResource = dstBuffer.get().impl->resource.Get(),
                  .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                  .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
                  .StateAfter = dstBuffer.get().impl->state,
              },
      },
  };

  impl->graphicsCommandList->ResourceBarrier(2, barriers.data());
}

std::unique_ptr<VertexBuffer> VidDriver::CreateVertexBuffer(
    std::span<float> vertices,
    uint32_t strideInBytes) {
  std::unique_ptr<VertexBuffer> vertexBuffer = std::make_unique<VertexBuffer>();

  vertexBuffer->buffer.impl->CreateBuffer(impl->device, vertices.size_bytes(),
                                          D3D12_HEAP_TYPE_DEFAULT);
  vertexBuffer->buffer.sizeInBytes = vertices.size_bytes();

  UploadBuffer(vertices.data(), vertices.size_bytes(), vertexBuffer->buffer);

  vertexBuffer->impl->bufferView = {
      .BufferLocation =
          vertexBuffer->buffer.impl->resource->GetGPUVirtualAddress(),
      .SizeInBytes = static_cast<uint32_t>(vertices.size_bytes()),
      .StrideInBytes = strideInBytes,
  };

  return vertexBuffer;
}

std::unique_ptr<IndexBuffer> VidDriver::CreateIndexBuffer(
    std::span<uint32_t> indices) {
  std::unique_ptr<IndexBuffer> indexBuffer = std::make_unique<IndexBuffer>();
  indexBuffer->buffer.sizeInBytes = indices.size_bytes();

  indexBuffer->buffer.impl->CreateBuffer(impl->device, indices.size_bytes(),
                                         D3D12_HEAP_TYPE_DEFAULT);

  UploadBuffer(indices.data(), indices.size_bytes(), indexBuffer->buffer);

  indexBuffer->impl->bufferView = {
      .BufferLocation =
          indexBuffer->buffer.impl->resource->GetGPUVirtualAddress(),
      .SizeInBytes = static_cast<uint32_t>(indices.size_bytes()),
      .Format = DXGI_FORMAT_R32_UINT,
  };

  return indexBuffer;
}

std::unique_ptr<ConstantBuffer> VidDriver::CreateConstantBuffer(
    std::reference_wrapper<Buffer> buffer) {
  std::unique_ptr<ConstantBuffer> constantBuffer =
      std::make_unique<ConstantBuffer>();

  D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
      .BufferLocation = buffer.get().impl->resource->GetGPUVirtualAddress(),
      .SizeInBytes = buffer.get().sizeInBytes,
  };
  auto [cpuHandle, offset] = impl->descriptorAllocator_CBV_SRV_UAV.Allocate();
  impl->device->CreateConstantBufferView(&desc, cpuHandle);

  constantBuffer->impl->cpuHandle = cpuHandle;
  constantBuffer->impl->descriptorOffset = offset;

  return constantBuffer;
}

// TODO: make normal bind parameters
std::unique_ptr<ShaderResourceViewBuffer>
VidDriver::CreateShaderResourceViewBuffer(std::reference_wrapper<Buffer> buffer,
                                          bool isIndexBuffer,
                                          uint32_t numElements,
                                          uint32_t structureByteStride) {
  std::unique_ptr<ShaderResourceViewBuffer> srvBuffer =
      std::make_unique<ShaderResourceViewBuffer>();

  D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
      .Format = isIndexBuffer ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Buffer =
          {
              .FirstElement = 0,
              .NumElements = numElements,
              .StructureByteStride = isIndexBuffer ? 0 : structureByteStride,
              .Flags = D3D12_BUFFER_SRV_FLAG_NONE,
          },
  };
  auto [cpuHandle, offset] = impl->descriptorAllocator_CBV_SRV_UAV.Allocate();
  impl->device->CreateShaderResourceView(buffer.get().impl->resource.Get(),
                                         &desc, cpuHandle);

  srvBuffer->impl->cpuHandle = cpuHandle;
  srvBuffer->impl->descriptorOffset = offset;

  return srvBuffer;
}

std::unique_ptr<Shader> VidDriver::CompileShaderFromSource(
    std::string_view shaderSource,
    std::span<const wchar_t*> args) {
  std::unique_ptr<Shader> shader = std::make_unique<Shader>();

  DxcBuffer sourceBuffer = {
      .Ptr = shaderSource.data(),
      .Size = shaderSource.size(),
      .Encoding = 0,
  };
  Microsoft::WRL::ComPtr<IDxcResult> result;
  THROW_IF_FAILED(impl->compiler->Compile(&sourceBuffer, args.data(),
                                          args.size(), nullptr,
                                          IID_PPV_ARGS(result.GetAddressOf())));

  Microsoft::WRL::ComPtr<IDxcBlobUtf8> error;
  THROW_IF_FAILED(result->GetOutput(
      DXC_OUT_ERRORS, IID_PPV_ARGS(error.GetAddressOf()), nullptr));
  if (error && error->GetStringLength() > 0) {
    OutputDebugString(std::format("IDxcCompiler::Compile error: {}\n",
                                  static_cast<char*>(error->GetBufferPointer()))
                          .c_str());
    throw std::runtime_error(static_cast<char*>(error->GetBufferPointer()));
  }

  Microsoft::WRL::ComPtr<IDxcBlob> blob;
  THROW_IF_FAILED(result->GetOutput(
      DXC_OUT_OBJECT, IID_PPV_ARGS(blob.GetAddressOf()), nullptr));

  shader->binary.resize(blob->GetBufferSize());
  memcpy(shader->binary.data(), blob->GetBufferPointer(),
         blob->GetBufferSize());

  return shader;
}

struct PipelineState::Impl {
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
};

PipelineState::PipelineState() {
  impl = std::make_unique<Impl>();
}
PipelineState::~PipelineState() {}

// TODO: support more options
std::unique_ptr<PipelineState> VidDriver::CreatePipelineState(
    std::span<std::byte> vsShaderByteCode,
    std::span<std::byte> psShaderByteCode) {
  std::unique_ptr<PipelineState> pso = std::make_unique<PipelineState>();

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
      .pRootSignature = impl->rootSignature.Get(),
      .VS =
          {
              .pShaderBytecode = vsShaderByteCode.data(),
              .BytecodeLength = vsShaderByteCode.size_bytes(),
          },
      .PS =
          {
              .pShaderBytecode = psShaderByteCode.data(),
              .BytecodeLength = psShaderByteCode.size_bytes(),
          },
      .DS = {},
      .HS = {},
      .GS = {},
      .StreamOutput = {},
      .BlendState =
          {
              .AlphaToCoverageEnable = false,
              .IndependentBlendEnable = false,
              .RenderTarget =
                  {
                      {
                          .BlendEnable = false,
                          .LogicOpEnable = false,
                          .SrcBlend = D3D12_BLEND_ONE,
                          .DestBlend = D3D12_BLEND_ZERO,
                          .BlendOp = D3D12_BLEND_OP_ADD,
                          .SrcBlendAlpha = D3D12_BLEND_ONE,
                          .DestBlendAlpha = D3D12_BLEND_ZERO,
                          .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                          .LogicOp = D3D12_LOGIC_OP_NOOP,
                          .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                      },
                  },
          },
      .SampleMask = UINT_MAX,
      .RasterizerState =
          {
              .FillMode = D3D12_FILL_MODE_SOLID,
              .CullMode = D3D12_CULL_MODE_NONE,
              .FrontCounterClockwise = false,
              .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
              .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
              .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
              .DepthClipEnable = true,
              .MultisampleEnable = false,
              .AntialiasedLineEnable = false,
              .ForcedSampleCount = 0,
              .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
          },
      .DepthStencilState = {},
      .InputLayout =
          {
              .pInputElementDescs = inputElementDescs,
              .NumElements = 2,
          },
      .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats =
          {
              DXGI_FORMAT_R8G8B8A8_UNORM,
          },
      .DSVFormat = DXGI_FORMAT_UNKNOWN,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .NodeMask = 0,
      .CachedPSO = {},
      .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
  };

  THROW_IF_FAILED(impl->device->CreateGraphicsPipelineState(
      &desc, IID_PPV_ARGS(pso->impl->pipelineState.GetAddressOf())));
  return pso;
}

void VidDriver::FlushAndWait() {
  if (impl->frameFence->GetCompletedValue() < frameNumber) {
    THROW_IF_FAILED(
        impl->frameFence->SetEventOnCompletion(frameNumber, nullptr));
  }

  ID3D12CommandList* commandLists[1] = {impl->graphicsCommandList.Get()};
  THROW_IF_FAILED(impl->graphicsCommandList->Close());
  impl->presentQueue->ExecuteCommandLists(1, commandLists);

  const uint32_t bufferIndex = frameNumber % kMaxGpuFramesInFlight;
  // TODO: cache PSO and reset it here
  impl->graphicsCommandList->Reset(impl->commandAllocators[bufferIndex].Get(),
                                   nullptr);

  static uint64_t flushValue = 0;
  THROW_IF_FAILED(
      impl->presentQueue->Signal(impl->flushFence.Get(), ++flushValue));
  THROW_IF_FAILED(impl->flushFence->SetEventOnCompletion(flushValue, nullptr));
}

void VidDriver::SetPipelineState(
    std::reference_wrapper<PipelineState> pipelineState) {
  impl->graphicsCommandList->SetPipelineState(
      pipelineState.get().impl->pipelineState.Get());
}

void VidDriver::BindShaderResourceViewBuffers(
    std::vector<std::reference_wrapper<ShaderResourceViewBuffer>> buffers) {
  const uint32_t bufferIndex = frameNumber % kMaxGpuFramesInFlight;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap =
      impl->descriptorHeaps[bufferIndex];
  D3D12_CPU_DESCRIPTOR_HANDLE handle =
      heap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += (14 * impl->descriptorAllocator_CBV_SRV_UAV.incrementSize);

  for (std::reference_wrapper<ShaderResourceViewBuffer> buffer : buffers) {
    impl->device->CopyDescriptorsSimple(1, handle, buffer.get().impl->cpuHandle,
                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    handle.ptr += impl->descriptorAllocator_CBV_SRV_UAV.incrementSize;
  }

  ID3D12DescriptorHeap* heaps[1] = {heap.Get()};
  impl->graphicsCommandList->SetDescriptorHeaps(1, heaps);
}

void VidDriver::SetViewports(std::span<Viewport> viewports) {
  std::vector<D3D12_VIEWPORT> d3d12Viewports;
  d3d12Viewports.reserve(viewports.size());
  for (Viewport& viewport : viewports) {
    d3d12Viewports.emplace_back(viewport.topLeftX, viewport.topLeftY,
                                viewport.width, viewport.height,
                                viewport.minDepth, viewport.maxDepth);
  }
  impl->graphicsCommandList->RSSetViewports(d3d12Viewports.size(),
                                            d3d12Viewports.data());
}

void VidDriver::SetScissorRects(std::span<SurfaceSize> surfaceSizes) {
  std::vector<D3D12_RECT> d3d12SurfaceSizes;
  d3d12SurfaceSizes.reserve(surfaceSizes.size());
  for (SurfaceSize& surfaceSize : surfaceSizes) {
    d3d12SurfaceSizes.emplace_back(surfaceSize.left, surfaceSize.top,
                                   surfaceSize.right, surfaceSize.bottom);
  }
  impl->graphicsCommandList->RSSetScissorRects(d3d12SurfaceSizes.size(),
                                               d3d12SurfaceSizes.data());
}

// TODO: implement texture
void VidDriver::SetRenderTargets() {
  const uint32_t bufferIndex = frameNumber % kMaxGpuFramesInFlight;
  auto [backBuffer, pair] = impl->backBuffers[bufferIndex];
  auto [cpuHandle, _] = pair;

  D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
      .Transition = {
          .pResource = backBuffer.Get(),
          .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
          .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
          .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
      }};
  impl->graphicsCommandList->ResourceBarrier(1, &barrier);

  impl->graphicsCommandList->OMSetRenderTargets(1, &cpuHandle, false, nullptr);
}

// TODO: implement texture
void VidDriver::ClearRenderTarget(const float clearColor[4]) {
  const uint32_t bufferIndex = frameNumber % kMaxGpuFramesInFlight;
  auto [cpuHandle, _] = impl->backBuffers[bufferIndex].second;

  impl->graphicsCommandList->ClearRenderTargetView(cpuHandle, clearColor, 0,
                                                   nullptr);
}

void VidDriver::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
  static std::array<D3D12_PRIMITIVE_TOPOLOGY,
                    static_cast<uint8_t>(PrimitiveTopology::Count)>
      d3d12PrimitiveTopologies = {
          D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,
          D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
          D3D_PRIMITIVE_TOPOLOGY_LINELIST,
          D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
      };
  impl->graphicsCommandList->IASetPrimitiveTopology(
      d3d12PrimitiveTopologies[static_cast<uint8_t>(primitiveTopology)]);
}

void VidDriver::SetVertexBuffers(
    std::span<std::reference_wrapper<VertexBuffer>> vertexBuffers) {
  std::vector<D3D12_VERTEX_BUFFER_VIEW> d3d12VertexBuffers;
  d3d12VertexBuffers.reserve(vertexBuffers.size());
  for (std::reference_wrapper<VertexBuffer> vertexBuffer : vertexBuffers) {
    d3d12VertexBuffers.push_back(vertexBuffer.get().impl->bufferView);
  }
  impl->graphicsCommandList->IASetVertexBuffers(0, d3d12VertexBuffers.size(),
                                                d3d12VertexBuffers.data());
}

void VidDriver::SetIndexBuffer(
    std::reference_wrapper<IndexBuffer> indexBuffer) {
  impl->graphicsCommandList->IASetIndexBuffer(
      &indexBuffer.get().impl->bufferView);
}

void VidDriver::DrawIndexedInstanced(uint32_t indexCountPerInstance,
                                     uint32_t instanceCount,
                                     uint32_t startIndexLocation,
                                     int32_t baseVertexLocation,
                                     uint32_t startInstanceLocation) {
  impl->graphicsCommandList->DrawIndexedInstanced(
      indexCountPerInstance, instanceCount, startIndexLocation,
      baseVertexLocation, startInstanceLocation);
}
