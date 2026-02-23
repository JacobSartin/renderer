cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(renderer LANGUAGES CXX)

include(GNUInstallDirs)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

add_executable(renderer ./src/main.cpp)

set_property(TARGET renderer PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET renderer PROPERTY CXX_STANDARD 23)
set_property(TARGET renderer PROPERTY CXX_STANDARD_REQUIRED ON)

set(MODULES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/modules")
# Glob all modules then pass them to target_sources so CMake can scan and compile them correctly
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS
    "${MODULES_PATH}/*.cppm"
)

target_sources(renderer
    PRIVATE
        src/Application.cpp
        src/EventHandler.cpp
        src/Renderer.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS "${MODULES_PATH}"
        FILES
            ${MODULE_FILES}
)

# TODO Remove when vcpkg works for imgui webgpu
target_include_directories(renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3)
find_package(Dawn CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

target_compile_definitions(renderer PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_DAWN)

target_link_libraries(renderer
    PRIVATE
        SDL3::SDL3
        dawn::webgpu_dawn
        assimp::assimp
)

# copy dx dlls for dawn, bad solution but I don't know how to automate
if(WIN32)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
    endif()

    add_custom_command(TARGET renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin/dxcompiler.dll,${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll>"
            "$<TARGET_FILE_DIR:renderer>/dxcompiler.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin/dxil.dll,${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/dxil.dll>"
            "$<TARGET_FILE_DIR:renderer>/dxil.dll"
        VERBATIM
    )
endif()

if(MSVC)
    target_compile_options(renderer
        PRIVATE
            /W4
            /wd5045
            /wd4464
            $<$<CONFIG:Debug>:/Zi>
            $<$<CONFIG:RelWithDebInfo>:/Zi>
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:RelWithDebInfo>:/O2>
    )
    target_compile_definitions(renderer PRIVATE _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
else()
    target_compile_options(renderer
        PRIVATE
            -Wall
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:RelWithDebInfo>:-g>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:RelWithDebInfo>:-O2>
    )
endif()

install(
    TARGETS renderer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
