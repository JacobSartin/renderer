cmake_minimum_required(VERSION 3.28)

project(renderer LANGUAGES C CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

add_executable(renderer)

target_sources(renderer
    PRIVATE
        src/main.cpp
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
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules
)

file(GLOB MODULE_INTERFACE_UNITS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm
)

if(MODULE_INTERFACE_UNITS)
    target_sources(renderer
        PRIVATE
            FILE_SET cxx_modules TYPE CXX_MODULES FILES ${MODULE_INTERFACE_UNITS}
    )
endif()

# TODO Remove when vcpkg works for imgui webgpu
target_include_directories(renderer PUBLIC
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
