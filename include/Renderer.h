#pragma once

#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>

// Forward declarations for ImGui
struct ImDrawData;

class Renderer {
public:
  Renderer();
  ~Renderer();

  // Initialize WebGPU and ImGui rendering
  bool Initialize(SDL_Window *window, int width, int height);
  void Shutdown();

  // Resize the rendering surface
  void Resize(int width, int height);

  // Begin a new frame
  void BeginFrame();

  // End frame and present
  void EndFrame();

  // Render ImGui draw data
  void RenderImGui(ImDrawData *drawData);

  // Clear color
  void SetClearColor(float r, float g, float b, float a);

  // Get device info
  WGPUDevice GetDevice() const { return m_Device; }

private:
  bool InitializeWebGPU(SDL_Window *window);
  bool InitializeImGuiBackend();
  WGPUAdapter RequestAdapter(wgpu::Instance &instance);
  WGPUDevice RequestDevice(wgpu::Instance &instance, wgpu::Adapter &adapter);
  WGPUSurface CreateSurface(const WGPUInstance &instance, SDL_Window *window);
  void ConfigureSurface();

  // WebGPU handles
  WGPUInstance m_Instance = nullptr;
  WGPUAdapter m_Adapter = nullptr;
  WGPUDevice m_Device = nullptr;
  WGPUSurface m_Surface = nullptr;
  WGPUQueue m_Queue = nullptr;
  WGPUSurfaceConfiguration m_SurfaceConfig = {};

  // Rendering state
  int m_Width = 0;
  int m_Height = 0;
  float m_ClearColor[4] = {0.45f, 0.55f, 0.60f, 1.00f};

  // Current frame resources
  WGPUSurfaceTexture m_CurrentSurfaceTexture = {};
  WGPUTextureView m_CurrentTextureView = nullptr;
  WGPUCommandEncoder m_CurrentEncoder = nullptr;
  WGPURenderPassEncoder m_CurrentRenderPass = nullptr;

  bool m_IsFrameStarted = false;
};
