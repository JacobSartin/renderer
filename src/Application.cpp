#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_wgpu.h"
#include <stdio.h>

Application::Application() {}

Application::~Application() { Shutdown(); }

bool Application::Initialize(int width, int height, const char *title) {
  m_Width = width;
  m_Height = height;

  // Initialize SDL
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
    return false;
  }

  // Create window
  SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
  m_Window = SDL_CreateWindow(title, m_Width, m_Height, windowFlags);
  if (!m_Window) {
    fprintf(stderr, "Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return false;
  }

  // Create event handler
  m_EventHandler = std::make_unique<EventHandler>();

  // Setup Dear ImGui context (must be done before renderer initialization)
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup scaling
  float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(mainScale);
  style.FontScaleDpi = mainScale;

  // Setup Platform backend
  ImGui_ImplSDL3_InitForOther(m_Window);

  // Create and initialize renderer on main thread
  // (blocks here but ensures window shows with content, not black screen)
  m_Renderer = std::make_unique<Renderer>();
  if (!m_Renderer->Initialize(m_Window, m_Width, m_Height)) {
    fprintf(stderr, "Failed to initialize renderer\n");
    return false;
  }

  // Setup event callbacks
  SetupCallbacks();

  printf("Application initialized successfully\n");
  printf("Press ESC to quit\n");

  return true;
}

void Application::Run() {
  m_Running = true;

  // Start render thread
  m_RenderThread = std::thread(&Application::RenderThreadFunc, this);

  // Wait for render thread to be ready
  while (!m_RenderThreadReady && m_Running) {
    SDL_Delay(1);
  }

  // Main event loop (must stay on main thread for SDL)
  while (m_Running) {
    // Process events on main thread (SDL requirement)
    if (!m_EventHandler->ProcessEvents()) {
      m_Running = false;
      break;
    }

    // Check window size on main thread
    int currentWidth, currentHeight;
    SDL_GetWindowSize(m_Window, &currentWidth, &currentHeight);
    if (currentWidth != m_Width || currentHeight != m_Height) {
      std::lock_guard<std::mutex> lock(m_StateMutex);
      m_PendingWidth = currentWidth;
      m_PendingHeight = currentHeight;
      m_ResizePending = true;
    }
  }

  // Shutdown will handle thread cleanup
}

void Application::Shutdown() {
  // Stop the render thread first (if not already stopped)
  m_Running = false;
  if (m_RenderThread.joinable()) {
    m_RenderThread.join();
  }

  // Shutdown ImGui backends in correct order:
  // 1. Platform backend (SDL3)
  ImGui_ImplSDL3_Shutdown();

  // 2. Renderer backend (WGPU) - called by Renderer destructor
  m_Renderer.reset();

  // 3. Destroy ImGui context
  ImGui::DestroyContext();

  m_EventHandler.reset();

  if (m_Window) {
    SDL_DestroyWindow(m_Window);
    m_Window = nullptr;
  }

  SDL_Quit();
}

void Application::SetupCallbacks() {
  // Register quit callback
  m_EventHandler->RegisterQuitCallback([this]() { OnQuit(); });

  // Register key callback
  m_EventHandler->RegisterKeyCallback(
      [this](SDL_Keycode key, bool pressed) { OnKey(key, pressed); });

  // Register mouse button callback
  m_EventHandler->RegisterMouseButtonCallback(
      [this](int button, bool pressed, int x, int y) {
        OnMouseButton(button, pressed, x, y);
      });

  // Register mouse motion callback
  m_EventHandler->RegisterMouseMotionCallback(
      [this](int x, int y) { OnMouseMotion(x, y); });

  // Register window resize callback
  m_EventHandler->RegisterWindowResizeCallback(
      [this](int width, int height) { OnWindowResize(width, height); });
}

void Application::UpdateImGui() {
  // Start Dear ImGui frame
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // 1. Show the big demo window
  if (m_ShowDemoWindow) {
    ImGui::ShowDemoWindow(&m_ShowDemoWindow);
  }

  // 2. Show a simple custom window
  {
    static float f = 0.0f;

    ImGui::Begin("Hello, World!");
    ImGui::Text("This is some useful text.");
    ImGui::Checkbox("Demo Window", &m_ShowDemoWindow);
    ImGui::Checkbox("Another Window", &m_ShowAnotherWindow);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", m_ClearColor);

    if (ImGui::Button("Button")) {
      m_Counter++;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", m_Counter);

    ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / io.Framerate, io.Framerate);

    // Display mouse position from event handler
    int mouseX, mouseY;
    m_EventHandler->GetMousePosition(mouseX, mouseY);
    ImGui::Text("Mouse Position: (%d, %d)", mouseX, mouseY);

    ImGui::End();
  }

  // 3. Show another simple window
  if (m_ShowAnotherWindow) {
    ImGui::Begin("Another Window", &m_ShowAnotherWindow);
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me")) {
      m_ShowAnotherWindow = false;
    }
    ImGui::End();
  }

  ImGui::Render();
}

void Application::RenderFrame() {
  // Update clear color
  m_Renderer->SetClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2],
                            m_ClearColor[3]);

  // Begin rendering
  m_Renderer->BeginFrame();

  // Update ImGui
  UpdateImGui();

  // Render ImGui
  m_Renderer->RenderImGui(ImGui::GetDrawData());

  // End rendering and present
  m_Renderer->EndFrame();
}

// Callback implementations
void Application::OnQuit() {
  printf("Quit requested\n");
  m_Running = false;
}

void Application::OnKey(SDL_Keycode key, bool pressed) {
  if (pressed) {
    if (key == SDLK_ESCAPE) {
      printf("ESC pressed - quitting\n");
      m_Running = false;
    } else if (key == SDLK_F11) {
      // Toggle fullscreen
      bool isFullscreen = SDL_GetWindowFlags(m_Window) & SDL_WINDOW_FULLSCREEN;
      SDL_SetWindowFullscreen(m_Window, !isFullscreen);
      printf("Toggled fullscreen\n");
    } else if (key == SDLK_SPACE) {
      printf("Space key pressed\n");
    }
  }
}

void Application::OnMouseButton(int button, bool pressed, int x, int y) {
  if (pressed) {
    printf("Mouse button %d pressed at (%d, %d)\n", button, x, y);
  }
}

void Application::OnMouseMotion(int x, int y) {
  // Can log mouse motion if needed, but it's very frequent
  // printf("Mouse moved to (%d, %d)\n", x, y);
}

void Application::OnWindowResize(int width, int height) {
  printf("Window resized to %dx%d\n", width, height);
  std::lock_guard<std::mutex> lock(m_StateMutex);
  m_PendingWidth = width;
  m_PendingHeight = height;
  m_ResizePending = true;
}

void Application::RenderThreadFunc() {
  printf("Render thread started\n");

  // Render first frame immediately
  RenderFrame();
  m_RenderThreadReady = true;

  while (m_Running) {
    // Handle pending resize
    if (m_ResizePending) {
      std::lock_guard<std::mutex> lock(m_StateMutex);
      m_Width = m_PendingWidth;
      m_Height = m_PendingHeight;
      m_Renderer->Resize(m_Width, m_Height);
      m_ResizePending = false;
    }

    // Render frame at full speed
    RenderFrame();
  }

  printf("Render thread stopped\n");
}
