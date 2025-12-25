# Building for desktop with SDL3, WebGPU, and Dear ImGui:
#
# Option 1: Using vcpkg (Recommended):
#  1. Set VCPKG_ROOT environment variable
#  2. git clone --branch docking https://github.com/ocornut/imgui imgui
#  3. cmake --preset windows-dawn (or linux-dawn, macos-dawn)
#  4. cmake --build build
#  Note: vcpkg.json will automatically install Dawn and SDL3
#
# Option 2: Using Dawn from source (git clone):
#  1. git clone https://github.com/google/dawn dawn
#  2. git clone --branch docking https://github.com/ocornut/imgui imgui
#  3. cmake -B build -DIMGUI_DAWN_DIR=dawn
#  4. cmake --build build
#
# Option 3: Using WGPU-Native (Rust-based WebGPU):
#  1. Download WGPU-Native from: https://github.com/gfx-rs/wgpu-native/releases
#  2. Extract to a folder (e.g., wgpu-native)
#  3. git clone --branch docking https://github.com/ocornut/imgui imgui
#  4. cmake -B build -DIMGUI_WGPU_DIR=wgpu-native
#  5. cmake --build build

cmake_minimum_required(VERSION 3.22) # Dawn requires CMake >= 3.22

set(CMAKE_CXX_STANDARD 20) # Dawn requires C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(renderer C CXX)

# Export compile commands for IDE integration (clangd, VSCode, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Hot Reload for MSVC compilers if supported
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(RENDERER_EXECUTABLE renderer)

# Set default build type to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Platform-specific optimizations
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-march=native HAS_MARCH)
check_cxx_compiler_flag(-mtune=native HAS_MTUNE)
if(HAS_MARCH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()
if(HAS_MTUNE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=native")
endif()

# Debug build flags
check_cxx_compiler_flag(-ggdb HAS_GGDB)
check_cxx_compiler_flag(-Z7 HAS_Z7)
if(HAS_GGDB)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
elseif(HAS_Z7)
    if(MSVC)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ZI")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Z7")
    endif()
endif()

# Release build flags
check_cxx_compiler_flag(-O3 HAS_O3)
check_cxx_compiler_flag(-Ob3 HAS_OB3)
if(HAS_OB3)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ob3")
elseif(HAS_O3)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

# MSVC-specific settings
if(MSVC)
    add_definitions(-D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
    add_definitions(/wd5045)  # Disable Spectre mitigation warnings
    add_definitions(/wd4464)  # Disable warning about relative include paths
endif()

# Dear ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

# Source files
set(SOURCE_FILES
    src/main.cpp
    src/Application.cpp
    src/EventHandler.cpp
    src/Renderer.cpp
    # ImGui backend files
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
    # Dear ImGui core files
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

# Check for conflicting backend specifications
if(IMGUI_DAWN_DIR AND IMGUI_WGPU_DIR)
    message(FATAL_ERROR "Please specify only one: IMGUI_DAWN_DIR or IMGUI_WGPU_DIR, not both")
endif()


# macOS specific settings
if(APPLE)
    set_source_files_properties(${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set(OS_LIBRARIES "-framework CoreFoundation -framework QuartzCore -framework Metal -framework MetalKit -framework Cocoa")
endif()

# Find SDL3
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

# Setup WebGPU backend (Dawn or WGPU-Native)
if(IMGUI_WGPU_DIR)
    # Setup WGPU-Native
    set(WGPU_NATIVE_LIB_DIR ${IMGUI_WGPU_DIR}/lib)
    find_library(WGPU_LIBRARY NAMES libwgpu_native.a wgpu_native.lib wgpu_native HINTS ${WGPU_NATIVE_LIB_DIR} REQUIRED)
    
    if(WIN32)
        set(OS_LIBRARIES d3dcompiler ws2_32 userenv bcrypt ntdll opengl32 Propsys RuntimeObject)
    elseif(UNIX AND NOT APPLE)
        set(OS_LIBRARIES "-lm -ldl")
    endif()
    
    set(LIBRARIES ${WGPU_LIBRARY} ${OS_LIBRARIES})
    message("Using WGPU-Native from ${IMGUI_WGPU_DIR}")
else()
    # Use Dawn (either from vcpkg or git clone)
    find_package(Threads) # required by Dawn
    
    if(IMGUI_DAWN_DIR)
        # User specified Dawn source directory - build from source
        message(STATUS "Building Dawn from source: ${IMGUI_DAWN_DIR}")
        list(APPEND CMAKE_PREFIX_PATH ${IMGUI_DAWN_DIR})
        
        option(DAWN_USE_GLFW OFF) # disable built-in GLFW in Dawn when using SDL3
        option(DAWN_FETCH_DEPENDENCIES "Use fetch_dawn_dependencies.py" ON)
        set(DAWN_BUILD_MONOLITHIC_LIBRARY "STATIC" CACHE STRING "Build monolithic library")

        # Dawn builds many things - disable what we don't need
        option(DAWN_BUILD_SAMPLES "Build Dawn samples" OFF)
        option(TINT_BUILD_CMD_TOOLS "Build Tint command line tools" OFF)
        option(TINT_BUILD_DOCS "Build documentation" OFF)
        option(TINT_BUILD_TESTS "Build tests" OFF)
        
        if(NOT APPLE)
            option(TINT_BUILD_MSL_WRITER "Build MSL writer" OFF)
        endif()
        
        if(WIN32)
            option(TINT_BUILD_SPV_READER "Build SPIR-V reader" OFF)
            option(TINT_BUILD_WGSL_READER "Build WGSL reader" ON)
            option(TINT_BUILD_GLSL_WRITER "Build GLSL writer" OFF)
            option(TINT_BUILD_GLSL_VALIDATOR "Build GLSL validator" OFF)
            option(TINT_BUILD_SPV_WRITER "Build SPIR-V writer" ON)
            option(TINT_BUILD_WGSL_WRITER "Build WGSL writer" ON)
        endif()

        add_subdirectory("${IMGUI_DAWN_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/dawn" EXCLUDE_FROM_ALL)
        set(LIBRARIES webgpu_dawn ${OS_LIBRARIES})
    else()
        # Try to find Dawn from vcpkg
        find_package(Dawn QUIET)
        
        if(Dawn_FOUND)
            message(STATUS "Using Dawn from vcpkg")
            set(LIBRARIES dawn::webgpu_dawn ${OS_LIBRARIES})
        else()
            message(FATAL_ERROR "Dawn not found. Either:\n"
                    "  1. Install via vcpkg (recommended): Add 'dawn' to vcpkg.json\n"
                    "  2. Build from source: cmake -B build -DIMGUI_DAWN_DIR=dawn")
        endif()
    endif()
endif()

# Create executable
add_executable(${RENDERER_EXECUTABLE} ${SOURCE_FILES})

target_include_directories(${RENDERER_EXECUTABLE} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Set backend-specific defines
if(IMGUI_WGPU_DIR)
    target_compile_definitions(${RENDERER_EXECUTABLE} PUBLIC "IMGUI_IMPL_WEBGPU_BACKEND_WGPU")
    target_include_directories(${RENDERER_EXECUTABLE} PUBLIC ${IMGUI_WGPU_DIR}/include)
else()
    # Using Dawn (from vcpkg or source)
    target_compile_definitions(${RENDERER_EXECUTABLE} PUBLIC "IMGUI_IMPL_WEBGPU_BACKEND_DAWN")
endif()

# Compiler warnings and definitions
if(MSVC)
    target_compile_options(${RENDERER_EXECUTABLE} PRIVATE /W4)
    # Use MultiThreaded runtime for static linking
    set_property(TARGET ${RENDERER_EXECUTABLE} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(${RENDERER_EXECUTABLE} PRIVATE -Wall)
endif()

target_link_libraries(${RENDERER_EXECUTABLE} PUBLIC ${LIBRARIES} SDL3::SDL3)

# Link additional Dawn C++ wrapper if built from source
if(IMGUI_DAWN_DIR)
    target_link_libraries(${RENDERER_EXECUTABLE} INTERFACE webgpu_cpp)
endif()

# Copy runtime DLLs to output directory (Windows only)
if(WIN32)
    # Copy vcpkg DLLs after build
    if(VCPKG_INSTALLED_DIR)
        add_custom_command(TARGET ${RENDERER_EXECUTABLE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:${RENDERER_EXECUTABLE}>"
            "$<TARGET_FILE_DIR:${RENDERER_EXECUTABLE}>"
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs from vcpkg"
        )
    endif()
    
    # Copy d3dcompiler_47.dll from System32 (required by Dawn on Windows)
    add_custom_command(TARGET ${RENDERER_EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{SystemRoot}/System32/d3dcompiler_47.dll"
        "$<TARGET_FILE_DIR:${RENDERER_EXECUTABLE}>"
        COMMENT "Copying d3dcompiler_47.dll from System32"
    )
endif()
