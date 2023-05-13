#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <vector>

class Display;

struct Buffer {
  Buffer();
  ~Buffer();

  struct Impl;
  std::unique_ptr<Impl> impl;

  uint32_t sizeInBytes;
};

struct VertexBuffer {
  VertexBuffer();
  ~VertexBuffer();

  struct Impl;
  std::unique_ptr<Impl> impl;

  Buffer buffer;
};

struct IndexBuffer {
  IndexBuffer();
  ~IndexBuffer();

  struct Impl;
  std::unique_ptr<Impl> impl;

  Buffer buffer;
};

struct ConstantBuffer {
  ConstantBuffer();
  ~ConstantBuffer();

  struct Impl;
  std::unique_ptr<Impl> impl;
};

struct ShaderResourceViewBuffer {
  ShaderResourceViewBuffer();
  ~ShaderResourceViewBuffer();

  struct Impl;
  std::unique_ptr<Impl> impl;
};

struct Shader {
  Shader();
  ~Shader();

  struct Impl;
  std::unique_ptr<Impl> impl;

  std::vector<std::byte> binary;
};

struct PipelineState {
  PipelineState();
  ~PipelineState();

  struct Impl;
  std::unique_ptr<Impl> impl;
};

struct Viewport {
  float topLeftX = 0.f;
  float topLeftY = 0.f;
  float width = 0.f;
  float height = 0.f;
  float minDepth = 0.f;
  float maxDepth = 0.f;
};

struct SurfaceSize {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
};

enum class PrimitiveTopology : uint8_t {
  Undefined = 0,
  Pointlist = 1,
  Linelist = 2,
  Linestrip = 3,
  Trianglelist = 4,
  Trianglestrip = 5,

  Count,
};

class VidDriver {
 public:
  VidDriver();
  ~VidDriver();

  void InitAPI(std::weak_ptr<Display> display);
  void ResizeSwapChain(uint32_t width, uint32_t height);

  void BeginFrame();
  void Present();
  void EndFrame();

  std::unique_ptr<VertexBuffer> CreateVertexBuffer(std::span<float> vertices,
                                                   uint32_t strideInBytes);
  std::unique_ptr<IndexBuffer> CreateIndexBuffer(std::span<uint32_t> indices);

  std::unique_ptr<ConstantBuffer> CreateConstantBuffer(
      std::reference_wrapper<Buffer> buffer);

  // TODO: make normal bind parameters
  std::unique_ptr<ShaderResourceViewBuffer> CreateShaderResourceViewBuffer(
      std::reference_wrapper<Buffer> buffer,
      bool isIndexBuffer,
      uint32_t numElements,
      uint32_t structureByteStride = 0);

  // TODO: support more options
  std::unique_ptr<PipelineState> CreatePipelineState(
      std::span<std::byte> vsShaderByteCode,
      std::span<std::byte> psShaderByteCode);

  std::unique_ptr<Shader> CompileShaderFromSource(
      std::string_view shaderSource,
      std::span<const wchar_t*> args);

  void FlushAndWait();

  void SetPipelineState(std::reference_wrapper<PipelineState> pipelineState);
  // TODO: implement Resource/Descriptor structure
  void BindShaderResourceViewBuffers(
      std::vector<std::reference_wrapper<ShaderResourceViewBuffer>> buffers);

  void SetViewports(std::span<Viewport> viewports);
  void SetScissorRects(std::span<SurfaceSize> surfaceSizes);

  // TODO: implement texture
  void SetRenderTargets();
  // TODO: implement texture
  void ClearRenderTarget(const float clearColor[4]);

  void SetPrimitiveTopology(PrimitiveTopology primitiveTopology);
  void SetVertexBuffers(
      std::span<std::reference_wrapper<VertexBuffer>> vertexBuffers);
  void SetIndexBuffer(std::reference_wrapper<IndexBuffer> indexBuffer);

  void DrawIndexedInstanced(uint32_t indexCountPerInstance,
                            uint32_t instanceCount,
                            uint32_t startIndexLocation = 0,
                            int32_t baseVertexLocation = 0,
                            uint32_t startInstanceLocation = 0);

 private:
  void CreateBackBuffers();

  void UploadBuffer(void* data,
                    uint64_t sizeInBytes,
                    std::reference_wrapper<Buffer> dstBuffer);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl = nullptr;

  uint64_t frameNumber = 0;
};

inline constexpr uint8_t kMaxGpuFramesInFlight = 3;
