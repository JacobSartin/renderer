# Renderer - SDL3 + WebGPU + Dear ImGui

A clean, modular renderer architecture with event handling separated from rendering.

## Architecture

- **EventHandler**: Processes SDL events using callbacks, keeping input logic decoupled from rendering
- **Renderer**: Manages WebGPU initialization, surface configuration, and rendering
- **Application**: Coordinates the event loop and rendering cycle

## Features

- SDL3 for windowing and input
- WebGPU with Dawn or WGPU-Native backends
- Dear ImGui for immediate mode GUI
- Callback-based event handling
- Separate event and render cycles (rendering won't block events)
- Resizable window with dynamic surface reconfiguration

## Building

### Prerequisites

1. CMake >= 3.22
2. C++17 compatible compiler
3. Git

### Setup

#### Option 1: Using Dawn (Google's WebGPU implementation)

```bash
# Clone dependencies
git clone https://github.com/google/dawn dawn
git clone https://github.com/ocornut/imgui imgui

# Configure
cmake --preset {OS}-dawn

# Build
cmake --build --preset {OS}-dawn-debug

# Run
./build/Debug/renderer.exe  # Windows
./build/renderer             # Linux/macOS
```

#### Option 2: Using WGPU-Native (Rust-based WebGPU)

```bash
# Download WGPU-Native from: https://github.com/gfx-rs/wgpu-native/releases
# Extract to a folder (e.g., wgpu-native)

# Clone ImGui
git clone https://github.com/ocornut/imgui imgui

# Configure
cmake -B build -DIMGUI_WGPU_DIR=wgpu-native

# Build
cmake --build build

# Run
./build/Debug/renderer.exe  # Windows
./build/renderer             # Linux/macOS
```

### SDL3 Installation

Should be done by vcpkg automatically as part of cmake configuration

```bash
vcpkg install sdl3
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
