cmake_minimum_required(VERSION 3.6)

project(Arln)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ARLN_BUILD_EXAMPLES "build examples" OFF)

set(SOURCES
"ARLN/ArlnWindow.cpp"
"ARLN/ArlnContext.cpp"
"ARLN/ArlnCommandBuffer.cpp"
"ARLN/ArlnSwapchain.cpp"
"ARLN/ArlnFrame.cpp"
"ARLN/ArlnImage.cpp"
"ARLN/ArlnPipeline.cpp"
"ARLN/ArlnDescriptor.cpp"
"ARLN/ArlnBuffer.cpp"
"ARLN/ArlnImGui.cpp"
"vendor/imgui/imgui.cpp"
"vendor/imgui/imgui_draw.cpp"
"vendor/imgui/imgui_demo.cpp"
"vendor/imgui/imgui_widgets.cpp"
"vendor/imgui/imgui_tables.cpp"
"vendor/imgui/backends/imgui_impl_sdl3.cpp"
)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/volk)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/Vulkan-Headers)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanMemoryAllocator)

add_library(ARLN STATIC ${SOURCES})

target_include_directories(ARLN PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ARLN/ ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/)
target_include_directories(ARLN PRIVATE ${Vulkan_INCLUDE_DIR})
target_link_libraries(ARLN PUBLIC volk Vulkan::Headers SDL3::SDL3-static GPUOpen::VulkanMemoryAllocator glm::glm)

if (ARLN_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif ()