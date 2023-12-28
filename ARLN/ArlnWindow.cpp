#include "ArlnWindow.hpp"
#include "ArlnContext.hpp"
#include "ArlnImGui.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_sdl3.h>

namespace arln {

    Window::Window(arln::WindowCreateInfo const& t_createInfo)
        : m_size{ t_createInfo.size }, m_title{ t_createInfo.title }
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
        {
            t_createInfo.errorCallback("SDL initialization failed");
        }

        m_handle = SDL_CreateWindow(
            m_title.c_str(),
            static_cast<i32>(m_size.x),
            static_cast<i32>(m_size.y),
            SDL_WINDOW_VULKAN |
            (t_createInfo.resizable ? SDL_WINDOW_RESIZABLE : 0)
        );

        if (!m_handle)
        {
            t_createInfo.errorCallback("SDL Window creation failed");
        }

        if (t_createInfo.customPosition)
        {
            SDL_SetWindowPosition(m_handle, t_createInfo.position.x, t_createInfo.position.y);
        }
    }

    Window::~Window()
    {
        SDL_DestroyWindow(m_handle);
        SDL_Quit();
    }

    void Window::pollEvents() noexcept
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (ImguiContext::IsCreated())
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
            }

            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                m_shouldClose = true;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                m_size.x = 0;
                m_size.y = 0;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                SDL_GetWindowSize(m_handle, (i32*)&m_size.x, (i32*)&m_size.y);
                break;
            default:
                break;
            }
        }

        static f64 previousTime = 0.0;
        static u32 frames = 0;
        auto currentTime = getTime();
        frames++;

        if (currentTime - previousTime >= 1.0f)
        {
            m_fps = u32(1.f / (currentTime - previousTime) * frames);
            previousTime = currentTime;
            frames = 0;
        }

        m_keyboardState = (u8*)SDL_GetKeyboardState(nullptr);
    }

    auto Window::createSurface(VkInstance t_instance) const noexcept -> VkSurfaceKHR
    {
        VkSurfaceKHR surface;
        SDL_Vulkan_CreateSurface(m_handle, t_instance, nullptr, &surface);
        return surface;
    }

    auto Window::getInstanceExtensions() const noexcept -> std::vector<const char*>
    {
        u32 extensionCount;
        auto extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

        std::vector<const char*> extensions(extensionCount);

        for (u32 i = extensionCount; i--; )
        {
            extensions[i] = extensionNames[i];
        }

        return extensions;
    }

    auto Window::getTime() const noexcept -> f64
    {
        return static_cast<f64>(SDL_GetTicks()) / 1000.0;
    }

    void Window::setPosition(ivec2 const& t_position) noexcept
    {
        SDL_SetWindowPosition(m_handle, t_position.x, t_position.y);
    }

    auto Window::getPosition() const noexcept -> ivec2
    {
        ivec2 pos;
        SDL_GetWindowPosition(m_handle, &pos.x, &pos.y);
        return pos;
    }

    void Window::setSize(ivec2 const& t_size) noexcept
    {
        SDL_SetWindowSize(m_handle, t_size.x, t_size.y);
    }

    void Window::setTitle(std::string_view t_tile) noexcept
    {
        m_title = t_tile;
        SDL_SetWindowTitle(m_handle, m_title.c_str());
    }

    bool Window::isKeyPressed(Key t_key) noexcept
    {
        return m_keyboardState[static_cast<i32>(t_key)];
    }

    bool Window::isKeyDown(Key t_key) noexcept
    {
        static bool prevKeyboardState[SDL_NUM_SCANCODES] = { false };

        if (m_keyboardState[static_cast<i32>(t_key)] && !prevKeyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = true;
            return true;
        }
        else if (!m_keyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = false;
        }

        return false;
    }

    bool Window::isKeyUp(Key t_key) noexcept
    {
        static bool prevKeyboardState[SDL_NUM_SCANCODES] = { false };

        if (!m_keyboardState[static_cast<i32>(t_key)] && prevKeyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = false;
            return true;
        }
        else if (m_keyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = true;
        }

        return false;
    }
}