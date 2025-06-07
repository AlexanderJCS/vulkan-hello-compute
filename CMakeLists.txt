cmake_minimum_required(VERSION 3.29)
project(raymarcher)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

include(FetchContent)

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-error=unknown-pragmas)
endif()

add_executable(raymarcher src/main.cpp
        src/tools/consts.h
        src/tools/vktools.cpp
        src/tools/vktools.h
        src/window/Window.cpp
        src/window/Window.h
        src/graphics/Shader.cpp
        src/graphics/Shader.h
        src/core/DescriptorSet.cpp
        src/core/DescriptorSet.h
        src/core/PushConstants.h
        src/core/Buffer.cpp
        src/core/Buffer.h
        src/graphics/Camera.cpp
        src/graphics/Camera.h
        src/core/CmdBuffer.cpp
        src/core/CmdBuffer.h
        src/graphics/Image.cpp
        src/graphics/Image.h
        src/Raymarcher.cpp
        src/Raymarcher.h
        src/tools/Clock.cpp
        src/tools/Clock.h
        polyglot/common.h)

target_link_libraries(raymarcher
        PRIVATE
        Vulkan::Vulkan
        glfw
)

target_include_directories(raymarcher PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${stb_SOURCE_DIR}
)
