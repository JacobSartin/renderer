# Renderer - SDL3 + WebGPU + Dear ImGui

A clean, modular renderer architecture with event handling separated from rendering.

## Architecture

- **EventHandler**: Processes SDL events using callbacks, keeping input logic decoupled from rendering
- **Renderer**: Manages WebGPU initialization, surface configuration, and rendering
- **Application**: Coordinates the event loop and rendering cycle

## Features

- SDL3 for windowing and input
- WebGPU with Dawn backend (via vcpkg)
- Dear ImGui for immediate mode GUI
- Callback-based event handling
- Separate event and render cycles (rendering won't block events)
- Resizable window with dynamic surface reconfiguration

## Building

### Prerequisites

1. CMake >= 3.28
2. C++17 compatible compiler
3. Git
4. vcpkg (with `VCPKG_ROOT` set)

### Setup

#### vcpkg-managed setup (recommended)

```bash
# Clone dependencies
git clone https://github.com/ocornut/imgui imgui

# Configure
cmake --preset {OS}

# Build
cmake --build --preset {OS}-debug

# Run
./build/Debug/renderer.exe  # Windows
./build/renderer             # Linux/macOS
```

### Dependencies

Dependencies (including SDL3 and Dawn) are installed automatically by vcpkg during CMake configure.

```bash
vcpkg install
```

## Usage

The application demonstrates:

- Event callbacks for keyboard, mouse, and window events
- ImGui demo window showing various UI widgets
- Custom "Hello, World!" window with interactive controls
- Press `ESC` to quit
- Press `F11` to toggle fullscreen

## Project Structure

```text
renderer/
├── include/
│   ├── Application.h      # Main application coordinator
│   ├── EventHandler.h     # Event processing with callbacks
│   └── Renderer.h         # WebGPU rendering
├── src/
│   ├── main.cpp          # Entry point
│   ├── Application.cpp
│   ├── EventHandler.cpp
│   └── Renderer.cpp
├── CMakeLists.txt
└── imgui/               # Dear ImGui library
```

## Event Handling

The event handler uses callbacks to keep logic separate:

```cpp
// Register callbacks
m_EventHandler->RegisterKeyCallback([](SDL_Keycode key, bool pressed) {
    if (pressed && key == SDLK_SPACE) {
        printf("Space pressed!\n");
    }
});

m_EventHandler->RegisterMouseButtonCallback([](int button, bool pressed, int x, int y) {
    printf("Mouse button %d at (%d, %d)\n", button, x, y);
});
```

Events are processed independently from rendering, so slow rendering won't block input responsiveness.

## License

MIT License - see LICENSE file for details.
