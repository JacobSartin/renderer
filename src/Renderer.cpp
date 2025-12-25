#include "Renderer.h"
#include "imgui.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>

#if defined(SDL_PLATFORM_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

Renderer::Renderer() {}

Renderer::~Renderer() { Shutdown(); }

bool Renderer::Initialize(SDL_Window *window, int width, int height) {
  m_Width = width;
  m_Height = height;

  if (!InitializeWebGPU(window)) {
    fprintf(stderr, "Failed to initialize WebGPU\n");
    return false;
  }

  if (!InitializeImGuiBackend()) {
    fprintf(stderr, "Failed to initialize ImGui backend\n");
    return false;
  }

  return true;
}

void Renderer::Shutdown() {
  if (m_Device) {
    ImGui_ImplWGPU_Shutdown();
  }

  if (m_Surface) {
    wgpuSurfaceUnconfigure(m_Surface);
    wgpuSurfaceRelease(m_Surface);
    m_Surface = nullptr;
  }

  if (m_Queue) {
    wgpuQueueRelease(m_Queue);
    m_Queue = nullptr;
  }

  if (m_Device) {
    wgpuDeviceRelease(m_Device);
    m_Device = nullptr;
  }

  if (m_Adapter) {
    wgpuAdapterRelease(m_Adapter);
    m_Adapter = nullptr;
  }

  if (m_Instance) {
    wgpuInstanceRelease(m_Instance);
    m_Instance = nullptr;
  }
}

void Renderer::Resize(int width, int height) {
  if (width <= 0 || height <= 0)
    return;

  m_Width = width;
  m_Height = height;
  ConfigureSurface();
}

void Renderer::BeginFrame() {
  if (m_IsFrameStarted) {
    fprintf(stderr, "Frame already started!\n");
    return;
  }

  // Get current surface texture
  wgpuSurfaceGetCurrentTexture(m_Surface, &m_CurrentSurfaceTexture);

  if (ImGui_ImplWGPU_IsSurfaceStatusError(m_CurrentSurfaceTexture.status)) {
    fprintf(stderr, "Unrecoverable Surface Texture status=%#.8x\n",
            m_CurrentSurfaceTexture.status);
    return;
  }

  if (ImGui_ImplWGPU_IsSurfaceStatusSubOptimal(
          m_CurrentSurfaceTexture.status)) {
    if (m_CurrentSurfaceTexture.texture) {
      wgpuTextureRelease(m_CurrentSurfaceTexture.texture);
    }
    if (m_Width > 0 && m_Height > 0) {
      Resize(m_Width, m_Height);
    }
    return;
  }

  // Create texture view
  WGPUTextureViewDescriptor viewDesc = {};
  viewDesc.format = m_SurfaceConfig.format;
  viewDesc.dimension = WGPUTextureViewDimension_2D;
  viewDesc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
  viewDesc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
  viewDesc.aspect = WGPUTextureAspect_All;
  m_CurrentTextureView =
      wgpuTextureCreateView(m_CurrentSurfaceTexture.texture, &viewDesc);

  // Create command encoder
  WGPUCommandEncoderDescriptor encDesc = {};
  m_CurrentEncoder = wgpuDeviceCreateCommandEncoder(m_Device, &encDesc);

  // Begin render pass
  WGPURenderPassColorAttachment colorAttachment = {};
  colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
  colorAttachment.loadOp = WGPULoadOp_Clear;
  colorAttachment.storeOp = WGPUStoreOp_Store;
  colorAttachment.clearValue = {
      m_ClearColor[0] * m_ClearColor[3], m_ClearColor[1] * m_ClearColor[3],
      m_ClearColor[2] * m_ClearColor[3], m_ClearColor[3]};
  colorAttachment.view = m_CurrentTextureView;

  WGPURenderPassDescriptor renderPassDesc = {};
  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &colorAttachment;
  renderPassDesc.depthStencilAttachment = nullptr;

  m_CurrentRenderPass =
      wgpuCommandEncoderBeginRenderPass(m_CurrentEncoder, &renderPassDesc);

  m_IsFrameStarted = true;
}

void Renderer::EndFrame() {
  if (!m_IsFrameStarted) {
    return;
  }

  // End render pass
  wgpuRenderPassEncoderEnd(m_CurrentRenderPass);

  // Submit command buffer
  WGPUCommandBufferDescriptor cmdBufferDesc = {};
  WGPUCommandBuffer cmdBuffer =
      wgpuCommandEncoderFinish(m_CurrentEncoder, &cmdBufferDesc);
  wgpuQueueSubmit(m_Queue, 1, &cmdBuffer);

  // Present
  wgpuSurfacePresent(m_Surface);

  // Tick device (Dawn specific)
  wgpuDeviceTick(m_Device);

  // Cleanup
  wgpuCommandBufferRelease(cmdBuffer);
  wgpuCommandEncoderRelease(m_CurrentEncoder);
  wgpuRenderPassEncoderRelease(m_CurrentRenderPass);
  wgpuTextureViewRelease(m_CurrentTextureView);

  m_CurrentEncoder = nullptr;
  m_CurrentRenderPass = nullptr;
  m_CurrentTextureView = nullptr;
  m_IsFrameStarted = false;
}

void Renderer::RenderImGui(ImDrawData *drawData) {
  if (!m_IsFrameStarted || !m_CurrentRenderPass) {
    fprintf(stderr, "Cannot render ImGui: frame not started\n");
    return;
  }

  ImGui_ImplWGPU_RenderDrawData(drawData, m_CurrentRenderPass);
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
  m_ClearColor[0] = r;
  m_ClearColor[1] = g;
  m_ClearColor[2] = b;
  m_ClearColor[3] = a;
}

bool Renderer::InitializeWebGPU(SDL_Window *window) {
  // Create instance
  wgpu::InstanceDescriptor instanceDesc = {};
  static constexpr wgpu::InstanceFeatureName timedWaitAny =
      wgpu::InstanceFeatureName::TimedWaitAny;
  instanceDesc.requiredFeatureCount = 1;
  instanceDesc.requiredFeatures = &timedWaitAny;
  wgpu::Instance instance = wgpu::CreateInstance(&instanceDesc);

  if (!instance) {
    fprintf(stderr, "Failed to create WebGPU instance\n");
    return false;
  }

  // Request adapter
  wgpu::Adapter adapter = RequestAdapter(instance);
  if (!adapter) {
    fprintf(stderr, "Failed to get WebGPU adapter\n");
    return false;
  }

  ImGui_ImplWGPU_DebugPrintAdapterInfo(adapter.Get());

  // Request device
  WGPUDevice device = RequestDevice(instance, adapter);
  if (!device) {
    fprintf(stderr, "Failed to get WebGPU device\n");
    return false;
  }

  // Create surface
  wgpu::Surface surface = CreateSurface(instance.Get(), window);
  if (!surface) {
    fprintf(stderr, "Failed to create surface\n");
    return false;
  }

  // Move to C handles
  m_Instance = instance.MoveToCHandle();
  m_Adapter = adapter.MoveToCHandle();
  m_Device = device;
  m_Surface = surface.MoveToCHandle();

  // Get surface capabilities
  WGPUSurfaceCapabilities surfaceCaps = {};
  wgpuSurfaceGetCapabilities(m_Surface, m_Adapter, &surfaceCaps);

  // Configure surface
  m_SurfaceConfig.presentMode = WGPUPresentMode_Fifo;
  m_SurfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
  m_SurfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
  m_SurfaceConfig.width = m_Width;
  m_SurfaceConfig.height = m_Height;
  m_SurfaceConfig.device = m_Device;
  m_SurfaceConfig.format = surfaceCaps.formats[0];

  ConfigureSurface();

  // Get queue
  m_Queue = wgpuDeviceGetQueue(m_Device);

  return true;
}

bool Renderer::InitializeImGuiBackend() {
  ImGui_ImplWGPU_InitInfo initInfo;
  initInfo.Device = m_Device;
  initInfo.NumFramesInFlight = 3;
  initInfo.RenderTargetFormat = m_SurfaceConfig.format;
  initInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;

  return ImGui_ImplWGPU_Init(&initInfo);
}

WGPUAdapter Renderer::RequestAdapter(wgpu::Instance &instance) {
  wgpu::Adapter acquiredAdapter;
  wgpu::RequestAdapterOptions adapterOptions;
  adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;

  auto onRequestAdapter = [&](wgpu::RequestAdapterStatus status,
                              wgpu::Adapter adapter, wgpu::StringView message) {
    if (status != wgpu::RequestAdapterStatus::Success) {
      fprintf(stderr, "Failed to get adapter: %s\n", message.data);
      return;
    }
    acquiredAdapter = std::move(adapter);
  };

  wgpu::Future waitAdapterFunc = instance.RequestAdapter(
      &adapterOptions, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter);

  wgpu::WaitStatus waitStatus = instance.WaitAny(waitAdapterFunc, UINT64_MAX);

  if (waitStatus != wgpu::WaitStatus::Success || !acquiredAdapter) {
    fprintf(stderr, "Error on adapter request\n");
    return nullptr;
  }

  return acquiredAdapter.MoveToCHandle();
}

WGPUDevice Renderer::RequestDevice(wgpu::Instance &instance,
                                   wgpu::Adapter &adapter) {
  wgpu::DeviceDescriptor deviceDesc;

  deviceDesc.SetDeviceLostCallback(
      wgpu::CallbackMode::AllowSpontaneous,
      [](const wgpu::Device &, wgpu::DeviceLostReason type,
         wgpu::StringView msg) {
        fprintf(stderr, "Device lost (%d): %s\n", (int)type, msg.data);
      });

  deviceDesc.SetUncapturedErrorCallback(
      [](const wgpu::Device &, wgpu::ErrorType type, wgpu::StringView msg) {
        fprintf(stderr, "WebGPU error (%d): %s\n", (int)type, msg.data);
      });

  wgpu::Device acquiredDevice;
  auto onRequestDevice = [&](wgpu::RequestDeviceStatus status,
                             wgpu::Device device, wgpu::StringView message) {
    if (status != wgpu::RequestDeviceStatus::Success) {
      fprintf(stderr, "Failed to get device: %s\n", message.data);
      return;
    }
    acquiredDevice = std::move(device);
  };

  wgpu::Future waitDeviceFunc = adapter.RequestDevice(
      &deviceDesc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice);

  wgpu::WaitStatus waitStatus = instance.WaitAny(waitDeviceFunc, UINT64_MAX);

  if (waitStatus != wgpu::WaitStatus::Success || !acquiredDevice) {
    fprintf(stderr, "Error on device request\n");
    return nullptr;
  }

  return acquiredDevice.MoveToCHandle();
}

WGPUSurface Renderer::CreateSurface(const WGPUInstance &instance,
                                    SDL_Window *window) {
  SDL_PropertiesID propertiesID = SDL_GetWindowProperties(window);

  ImGui_ImplWGPU_CreateSurfaceInfo createInfo = {};
  createInfo.Instance = instance;

#if defined(SDL_PLATFORM_MACOS)
  createInfo.System = "cocoa";
  createInfo.RawWindow = (void *)SDL_GetPointerProperty(
      propertiesID, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
  return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&createInfo);
#elif defined(SDL_PLATFORM_LINUX)
  if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
    createInfo.System = "wayland";
    createInfo.RawDisplay = (void *)SDL_GetPointerProperty(
        propertiesID, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
    createInfo.RawSurface = (void *)SDL_GetPointerProperty(
        propertiesID, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&createInfo);
  } else if (!SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11")) {
    createInfo.System = "x11";
    createInfo.RawWindow = (void *)SDL_GetNumberProperty(
        propertiesID, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    createInfo.RawDisplay = (void *)SDL_GetPointerProperty(
        propertiesID, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&createInfo);
  }
#elif defined(SDL_PLATFORM_WIN32)
  createInfo.System = "win32";
  createInfo.RawWindow = (void *)SDL_GetPointerProperty(
      propertiesID, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
  createInfo.RawInstance = (void *)::GetModuleHandle(NULL);
  return ImGui_ImplWGPU_CreateWGPUSurfaceHelper(&createInfo);
#else
#error "Unsupported WebGPU native platform!"
#endif

  return nullptr;
}

void Renderer::ConfigureSurface() {
  m_SurfaceConfig.width = m_Width;
  m_SurfaceConfig.height = m_Height;
  wgpuSurfaceConfigure(m_Surface, &m_SurfaceConfig);
}
