#include "display.h"
#include "viddriver.h"

#include <filesystem>
#include <fstream>
#include <vector>

std::string readShader(std::filesystem::path shaderPath) {
  std::ifstream ifs(shaderPath, std::ios::in | std::ios::binary);
  const uint32_t size = std::filesystem::file_size(shaderPath);
  std::string result(size, '\0');
  ifs.read(result.data(), size);
  return result;
}

int main() {
  std::shared_ptr<Display> display = std::make_shared<Display>(
      "RenderingASceneWithDeferredLighting", 800, 600);
  std::shared_ptr<VidDriver> vidDriver = std::make_shared<VidDriver>();

  display->Init(vidDriver);
  vidDriver->InitAPI(display);

  std::vector<float> vertices = {1.f,  -1.f, 0.f, 1.f, 0.f, 0.f,
                                 -1.f, -1.f, 0.f, 0.f, 1.f, 0.f,
                                 0.f,  1.f,  0.f, 0.f, 0.f, 1.f};
  std::unique_ptr<VertexBuffer> vertexBuffer =
      vidDriver->CreateVertexBuffer(vertices, sizeof(float) * 6);

  std::unique_ptr<ShaderResourceViewBuffer> srv1 =
      vidDriver->CreateShaderResourceViewBuffer(vertexBuffer->buffer, false,
                                                vertices.size(), sizeof(float));

  vidDriver->FlushAndWait();

  std::vector<uint32_t> indices = {0, 1, 2};
  std::unique_ptr<IndexBuffer> indexBuffer =
      vidDriver->CreateIndexBuffer(indices);

  std::unique_ptr<ShaderResourceViewBuffer> srv2 =
      vidDriver->CreateShaderResourceViewBuffer(indexBuffer->buffer, true,
                                                indices.size());

  std::string vsShaderSrc = readShader("../../../triangle.vs.hlsl");
  std::vector<const wchar_t*> vsArgs = {L"-E main_VS", L"-T vs_6_0"};
  std::unique_ptr<Shader> vertexShader =
      vidDriver->CompileShaderFromSource(vsShaderSrc, vsArgs);

  std::string psShaderSrc = readShader("../../../triangle.ps.hlsl");
  std::vector<const wchar_t*> psArgs = {L"-E main_PS", L"-T ps_6_0"};
  std::unique_ptr<Shader> pixelShader =
      vidDriver->CompileShaderFromSource(psShaderSrc, psArgs);

  std::unique_ptr<PipelineState> pipelineState =
      vidDriver->CreatePipelineState(vertexShader->binary, pixelShader->binary);

  vidDriver->FlushAndWait();

  display
      ->SetDrawCallback([&]() {
        vidDriver->BeginFrame();

        vidDriver->SetPipelineState(*pipelineState);
        vidDriver->BindShaderResourceViewBuffers({*srv1, *srv2});

        std::vector<Viewport> viewports = {
            Viewport(0.f, 0.f, static_cast<float>(display->Width()),
                     static_cast<float>(display->Height()), 1.f, 1000.f)};
        vidDriver->SetViewports(viewports);

        std::vector<SurfaceSize> surfaceSizes = {
            SurfaceSize(0, 0, display->Width(), display->Height())};
        vidDriver->SetScissorRects(surfaceSizes);

        vidDriver->SetRenderTargets();
        const float clearColor[4] = {.2f, .2f, .2f, 1.f};
        vidDriver->ClearRenderTarget(clearColor);

        vidDriver->SetPrimitiveTopology(PrimitiveTopology::Trianglelist);
        std::vector<std::reference_wrapper<VertexBuffer>> vertexBuffers = {
            *vertexBuffer};
        vidDriver->SetVertexBuffers(vertexBuffers);
        vidDriver->SetIndexBuffer(*indexBuffer);

        vidDriver->DrawIndexedInstanced(3, 1);

        vidDriver->Present();

        vidDriver->EndFrame();
      })
      .Show()
      .Loop();

  return EXIT_SUCCESS;
}
