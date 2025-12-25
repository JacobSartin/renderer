#include "EventHandler.h"
#include "imgui_impl_sdl3.h"

EventHandler::EventHandler() {}

EventHandler::~EventHandler() {}

bool EventHandler::ProcessEvents() {
  bool shouldQuit = false;
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    // Pass event to ImGui first
    ImGui_ImplSDL3_ProcessEvent(&event);

    HandleEvent(event);

    if (event.type == SDL_EVENT_QUIT) {
      shouldQuit = true;
    }
  }

  return !shouldQuit;
}

void EventHandler::HandleEvent(const SDL_Event &event) {
  switch (event.type) {
  case SDL_EVENT_QUIT:
    for (auto &callback : m_QuitCallbacks) {
      callback();
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    m_KeyStates[event.key.key] = true;
    for (auto &callback : m_KeyCallbacks) {
      callback(event.key.key, true);
    }
    break;

  case SDL_EVENT_KEY_UP:
    m_KeyStates[event.key.key] = false;
    for (auto &callback : m_KeyCallbacks) {
      callback(event.key.key, false);
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    for (auto &callback : m_MouseButtonCallbacks) {
      callback(event.button.button, true, static_cast<int>(event.button.x),
               static_cast<int>(event.button.y));
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    for (auto &callback : m_MouseButtonCallbacks) {
      callback(event.button.button, false, static_cast<int>(event.button.x),
               static_cast<int>(event.button.y));
    }
    break;

  case SDL_EVENT_MOUSE_MOTION:
    m_MouseX = static_cast<int>(event.motion.x);
    m_MouseY = static_cast<int>(event.motion.y);
    for (auto &callback : m_MouseMotionCallbacks) {
      callback(m_MouseX, m_MouseY);
    }
    break;

  case SDL_EVENT_WINDOW_RESIZED:
    for (auto &callback : m_WindowResizeCallbacks) {
      callback(event.window.data1, event.window.data2);
    }
    break;
  }
}

void EventHandler::RegisterQuitCallback(QuitCallback callback) {
  m_QuitCallbacks.push_back(callback);
}

void EventHandler::RegisterKeyCallback(KeyCallback callback) {
  m_KeyCallbacks.push_back(callback);
}

void EventHandler::RegisterMouseButtonCallback(MouseButtonCallback callback) {
  m_MouseButtonCallbacks.push_back(callback);
}

void EventHandler::RegisterMouseMotionCallback(MouseMotionCallback callback) {
  m_MouseMotionCallbacks.push_back(callback);
}

void EventHandler::RegisterWindowResizeCallback(WindowResizeCallback callback) {
  m_WindowResizeCallbacks.push_back(callback);
}

bool EventHandler::IsKeyPressed(SDL_Keycode key) const {
  auto it = m_KeyStates.find(key);
  return it != m_KeyStates.end() && it->second;
}

void EventHandler::GetMousePosition(int &x, int &y) const {
  x = m_MouseX;
  y = m_MouseY;
}
