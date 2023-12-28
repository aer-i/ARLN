#pragma once
#include <imgui.h>

namespace arln {

    class Window;
    class Context;
    class CommandBuffer;
    class Image;
    class Descriptor;

    class ImguiContext
    {
    public:
        static void Init(Window& t_window) noexcept;
        static void Terminate() noexcept;
        static void Render(CommandBuffer& t_commandBuffer) noexcept;
        static auto CreateImguiImage(Image& t_image) noexcept -> Descriptor;
        static void RecreateImguiImage(Image& t_image, Descriptor t_descriptor) noexcept;
        static auto IsCreated() noexcept -> bool;
    };
}