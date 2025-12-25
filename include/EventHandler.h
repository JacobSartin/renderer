#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <unordered_map>
#include <vector>

// Event callback types
using QuitCallback = std::function<void()>;
using KeyCallback = std::function<void(SDL_Keycode, bool)>; // key, pressed
using MouseButtonCallback =
    std::function<void(int, bool, int, int)>; // button, pressed, x, y
using MouseMotionCallback = std::function<void(int, int)>;  // x, y
using WindowResizeCallback = std::function<void(int, int)>; // width, height

class EventHandler {
public:
  EventHandler();
  ~EventHandler();

  // Process all pending events (non-blocking)
  // Returns false if quit was requested
  bool ProcessEvents();

  // Register callbacks for various event types
  void RegisterQuitCallback(QuitCallback callback);
  void RegisterKeyCallback(KeyCallback callback);
  void RegisterMouseButtonCallback(MouseButtonCallback callback);
  void RegisterMouseMotionCallback(MouseMotionCallback callback);
  void RegisterWindowResizeCallback(WindowResizeCallback callback);

  // Check current input state (query-based, not event-based)
  bool IsKeyPressed(SDL_Keycode key) const;
  void GetMousePosition(int &x, int &y) const;

private:
  void HandleEvent(const SDL_Event &event);

  // Callbacks
  std::vector<QuitCallback> m_QuitCallbacks;
  std::vector<KeyCallback> m_KeyCallbacks;
  std::vector<MouseButtonCallback> m_MouseButtonCallbacks;
  std::vector<MouseMotionCallback> m_MouseMotionCallbacks;
  std::vector<WindowResizeCallback> m_WindowResizeCallbacks;

  // Current input state
  std::unordered_map<SDL_Keycode, bool> m_KeyStates;
  int m_MouseX = 0;
  int m_MouseY = 0;
};
