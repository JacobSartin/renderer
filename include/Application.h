#pragma once

#include "EventHandler.h"
#include "Renderer.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>


class Application {
public:
  Application();
  ~Application();

  // Initialize the application
  bool Initialize(int width = 1280, int height = 800,
                  const char *title = "Renderer");

  // Run the main loop
  void Run();

  // Shutdown the application
  void Shutdown();

private:
  void SetupCallbacks();
  void UpdateImGui();
  void RenderFrame();

  // Callback handlers
  void OnQuit();
  void OnKey(SDL_Keycode key, bool pressed);
  void OnMouseButton(int button, bool pressed, int x, int y);
  void OnMouseMotion(int x, int y);
  void OnWindowResize(int width, int height);

  void RenderThreadFunc(); // Render thread entry point

  SDL_Window *m_Window = nullptr;
  std::unique_ptr<EventHandler> m_EventHandler;
  std::unique_ptr<Renderer> m_Renderer;

  // Threading
  std::thread m_RenderThread;
  std::atomic<bool> m_Running{false};
  std::atomic<bool> m_RenderThreadReady{false};
  std::mutex m_StateMutex;

  // Window dimensions (protected by mutex)
  int m_Width = 1280;
  int m_Height = 800;
  std::atomic<bool> m_ResizePending{false};
  int m_PendingWidth = 1280;
  int m_PendingHeight = 800;

  // Demo UI state (accessed from render thread)
  bool m_ShowDemoWindow = true;
  bool m_ShowAnotherWindow = false;
  float m_ClearColor[4] = {0.45f, 0.55f, 0.60f, 1.00f};
  int m_Counter = 0;
};
