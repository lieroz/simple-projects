#include <d3d11.h>
#include <d3dcompiler.h>

#include <array>
#include <format>
#include <system_error>

#include "viddriver.h"


struct Vertex {
  std::array<float, 3> position;
  std::array<float, 3> color;
};

std::array<Vertex, 3> vertexBufferData = {
    Vertex{.position = {0.f, .5f, 0.f}, .color = {1.f, 0.f, 0.f}},
    Vertex{.position = {.5f, -.5f, 0.f}, .color = {0.f, 1.f, 0.f}},
    Vertex{.position = {-.5f, -.5f, 0.f}, .color = {0.f, 0.f, 1.f}},
};
std::array<uint32_t, 3> indexBufferData = {0, 1, 2};

struct VidDriver::Impl {
  HWND hwnd = nullptr;

#ifdef _DEBUG
  ID3D11Debug* debugController = nullptr;
#endif

  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* deviceContext = nullptr;

  IDXGISwapChain* swapChain = nullptr;
  ID3D11Texture2D* backbuffer = nullptr;
  ID3D11RenderTargetView* renderTargetView = nullptr;

  ID3D11Buffer* vertexBuffer = nullptr;
  ID3D11Buffer* indexBuffer = nullptr;
  ID3D11InputLayout* inputLayout = nullptr;
  ID3D11VertexShader* vertexShader = nullptr;
  ID3D11PixelShader* pixelShader = nullptr;
};

VidDriver::~VidDriver() {
  if (impl->swapChain) {
    impl->swapChain->Release();
    impl->swapChain = nullptr;
  }

  // destroy framebuffer
  if (impl->backbuffer) {
    impl->backbuffer->Release();
    impl->backbuffer = nullptr;
  }

  if (impl->renderTargetView) {
    impl->renderTargetView->Release();
    impl->renderTargetView = nullptr;
  }

  // destroy resources
  if (impl->vertexBuffer) {
    impl->vertexBuffer->Release();
    impl->vertexBuffer = nullptr;
  }

  if (impl->indexBuffer) {
    impl->indexBuffer->Release();
    impl->indexBuffer = nullptr;
  }

  if (impl->inputLayout) {
    impl->inputLayout->Release();
    impl->inputLayout = nullptr;
  }

  if (impl->vertexShader) {
    impl->vertexShader->Release();
    impl->vertexShader = nullptr;
  }

  if (impl->pixelShader) {
    impl->pixelShader->Release();
    impl->pixelShader = nullptr;
  }

  // destroy d3d11 api
  if (impl->deviceContext != nullptr) {
    impl->deviceContext->ClearState();
    impl->deviceContext->Flush();
  }

  if (impl->device != nullptr) {
    impl->device->Release();
    impl->device = nullptr;
  }

  if (impl->deviceContext != nullptr) {
    impl->deviceContext->Release();
    impl->deviceContext = nullptr;
  }

#if defined(_DEBUG)
  if (impl->debugController != nullptr) {
    impl->debugController->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY |
                                                   D3D11_RLDO_DETAIL);
    impl->debugController->Release();
    impl->debugController = nullptr;
  }
#endif
}

void VidDriver::Init(void* windowContext) {
  impl = std::make_shared<Impl>(static_cast<HWND>(windowContext));

  // initialize d3d11 api
  DXGI_SWAP_CHAIN_DESC desc = {};
  desc.BufferDesc.RefreshRate.Numerator = 0;
  desc.BufferDesc.RefreshRate.Denominator = 1;
  desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = 2;
  desc.OutputWindow = impl->hwnd;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  desc.Windowed = true;

  D3D_FEATURE_LEVEL featureLevelInputs[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,
      D3D_FEATURE_LEVEL_9_1};
  D3D_FEATURE_LEVEL featureLevelOutputs = D3D_FEATURE_LEVEL_11_1;
  UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  if (HRESULT hr = D3D11CreateDeviceAndSwapChain(
          nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelInputs,
          static_cast<uint32_t>(std::size(featureLevelInputs)),
          D3D11_SDK_VERSION, &desc, &impl->swapChain, &impl->device,
          &featureLevelOutputs, &impl->deviceContext);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("D3D11CreateDeviceAndSwapChain failed: %d", hr));
  }

#ifdef _DEBUG
  if (HRESULT hr =
          impl->device->QueryInterface(IID_PPV_ARGS(&impl->debugController));
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::QueryInterface failed: %d", hr));
  }
#endif

  // initialize framebuffer
  if (HRESULT hr =
          impl->swapChain->GetBuffer(0, IID_PPV_ARGS(&impl->backbuffer));
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("IDXGISwapChain::GetBuffer failed: %d", hr));
  }

  if (HRESULT hr = impl->device->CreateRenderTargetView(
          impl->backbuffer, nullptr, &impl->renderTargetView);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::CreateRenderTargetView failed: %d", hr));
  }

  // initialize resources
  ID3DBlob* vertexShaderBlob = nullptr;
  ID3DBlob* pixelShaderBlob = nullptr;
  ID3DBlob* errorsBlob = nullptr;

  UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
  compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  if (HRESULT hr = D3DCompileFromFile(L"../../triangle.hlsl", nullptr, nullptr,
                                      "vs_main", "vs_5_0", compileFlags, 0,
                                      &vertexShaderBlob, &errorsBlob);
      FAILED(hr)) {
    if (vertexShaderBlob != nullptr) {
      vertexShaderBlob->Release();
    }
    if (errorsBlob != nullptr) {
      std::string message = std::format("D3DCompileFromFile failed: %d, %s", hr,
                                        errorsBlob->GetBufferPointer());
      errorsBlob->Release();
      throw std::runtime_error(std::move(message));
    }
  }

  if (HRESULT hr = D3DCompileFromFile(L"../../triangle.hlsl", nullptr, nullptr,
                                      "ps_main", "ps_5_0", compileFlags, 0,
                                      &pixelShaderBlob, &errorsBlob);
      FAILED(hr)) {
    if (pixelShaderBlob != nullptr) {
      pixelShaderBlob->Release();
    }
    if (errorsBlob != nullptr) {
      std::string message = std::format("D3DCompileFromFile failed: %d, %s", hr,
                                        errorsBlob->GetBufferPointer());
      errorsBlob->Release();
      throw std::runtime_error(std::move(message));
    }
  }

  if (HRESULT hr = impl->device->CreateVertexShader(
          vertexShaderBlob->GetBufferPointer(),
          vertexShaderBlob->GetBufferSize(), nullptr, &impl->vertexShader);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::CreateVertexShader failed: %d", hr));
  }

  if (HRESULT hr = impl->device->CreatePixelShader(
          pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(),
          nullptr, &impl->pixelShader);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::CreatePixelShader failed: %d", hr));
  }

  D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
      {.SemanticName = "POSITION",
       .SemanticIndex = 0,
       .Format = DXGI_FORMAT_R32G32B32_FLOAT,
       .InputSlot = 0,
       .AlignedByteOffset = 0,
       .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
       .InstanceDataStepRate = 0},
      {.SemanticName = "COLOR",
       .SemanticIndex = 0,
       .Format = DXGI_FORMAT_R32G32B32_FLOAT,
       .InputSlot = 0,
       .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
       .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
       .InstanceDataStepRate = 0}};

  if (HRESULT hr = impl->device->CreateInputLayout(
          polygonLayout, static_cast<uint32_t>(std::size(polygonLayout)),
          vertexShaderBlob->GetBufferPointer(),
          vertexShaderBlob->GetBufferSize(), &impl->inputLayout);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::CreateInputLayout failed: %d", hr));
  }

  vertexShaderBlob->Release();
  vertexShaderBlob = nullptr;

  pixelShaderBlob->Release();
  pixelShaderBlob = nullptr;

  D3D11_BUFFER_DESC vertexBufferDesc = {
      .ByteWidth = sizeof(vertexBufferData.front()) *
                   static_cast<uint32_t>(vertexBufferData.size()),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
      .StructureByteStride = 0,
  };
  D3D11_SUBRESOURCE_DATA vertexData = {
      .pSysMem = vertexBufferData.data(),
      .SysMemPitch = 0,
      .SysMemSlicePitch = 0,
  };

  if (HRESULT hr = impl->device->CreateBuffer(&vertexBufferDesc, &vertexData,
                                              &impl->vertexBuffer);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::CreateBuffer failed: %d", hr));
  }

  D3D11_BUFFER_DESC indexBufferDesc = {
      .ByteWidth = sizeof(indexBufferData.front()) *
                   static_cast<uint32_t>(indexBufferData.size()),
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_INDEX_BUFFER,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
      .StructureByteStride = 0,
  };
  D3D11_SUBRESOURCE_DATA indexData = {
      .pSysMem = indexBufferData.data(),
      .SysMemPitch = 0,
      .SysMemSlicePitch = 0,
  };

  if (HRESULT hr = impl->device->CreateBuffer(&indexBufferDesc, &indexData,
                                              &impl->indexBuffer);
      FAILED(hr)) {
    throw std::runtime_error(
        std::format("ID3D11Device::CreateBuffer failed: %d", hr));
  }
}

void VidDriver::Render() {
  float color[4] = {0.2f, 0.2f, 0.2f, 1.0f};
  impl->deviceContext->ClearRenderTargetView(impl->renderTargetView, color);

  RECT rect;
  GetClientRect(impl->hwnd, &rect);
  D3D11_VIEWPORT viewport = {
      .TopLeftX = 0.0f,
      .TopLeftY = 0.0f,
      .Width = static_cast<float>(rect.right - rect.left),
      .Height = static_cast<float>(rect.bottom - rect.top),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
  };
  impl->deviceContext->RSSetViewports(1, &viewport);
  impl->deviceContext->OMSetRenderTargets(1, &impl->renderTargetView, nullptr);

  impl->deviceContext->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  impl->deviceContext->IASetInputLayout(impl->inputLayout);

  impl->deviceContext->VSSetShader(impl->vertexShader, nullptr, 0);
  impl->deviceContext->PSSetShader(impl->pixelShader, nullptr, 0);

  uint32_t stride = sizeof(vertexBufferData.front());
  uint32_t offset = 0;
  impl->deviceContext->IASetVertexBuffers(0, 1, &impl->vertexBuffer, &stride,
                                          &offset);
  impl->deviceContext->IASetIndexBuffer(impl->indexBuffer, DXGI_FORMAT_R32_UINT,
                                        0);

  impl->deviceContext->DrawIndexed(3, 0, 0);
  impl->swapChain->Present(1, 0);
}