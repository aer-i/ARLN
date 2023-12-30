#include "ArlnWindow.hpp"
#include "ArlnContext.hpp"
#include "ArlnImGui.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_sdl3.h>

namespace arln {

    Window::Window(arln::WindowCreateInfo const& t_createInfo)
        : m_title{ t_createInfo.title }
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
        {
            t_createInfo.errorCallback("SDL initialization failed");
        }

        m_handle = SDL_CreateWindow(
            m_title.c_str(),
            static_cast<i32>(t_createInfo.size.x),
            static_cast<i32>(t_createInfo.size.y),
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

        SDL_GetWindowSize(m_handle, (i32*)&m_size.x, (i32*)&m_size.y);
        m_keyboardState = (u8*)SDL_GetKeyboardState(nullptr);
    }

    Window::~Window()
    {
        SDL_DestroyWindow(m_handle);
        SDL_Quit();
    }

    void Window::pollEvents() noexcept
    {
        while (SDL_PollEvent(&m_event))
        {
            if (ImguiContext::IsCreated())
            {
                ImGui_ImplSDL3_ProcessEvent(&m_event);
            }

            switch (m_event.type)
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
            case SDL_EVENT_MOUSE_MOTION:
                SDL_GetMouseState(&m_cursorPos.x, &m_cursorPos.y);
                SDL_GetGlobalMouseState(&m_globalCursorPos.x, &m_globalCursorPos.y);
                break;
            default:
                break;
            }
        }

        SDL_GetRelativeMouseState(&m_cursorOffset.x, &m_cursorOffset.y);

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

    auto Window::getKey(Key t_key) noexcept -> bool
    {
        return m_keyboardState[static_cast<i32>(t_key)];
    }

    auto Window::getKeyDown(arln::Key t_key) noexcept -> bool
    {
        static bool prevKeyboardState[SDL_NUM_SCANCODES] = { false };

        if (m_keyboardState[static_cast<i32>(t_key)] && !prevKeyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = true;
            return true;
        }
        if (!m_keyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = false;
        }

        return false;
    }

    auto Window::getKeyUp(Key t_key) noexcept -> bool
    {
        static bool prevKeyboardState[SDL_NUM_SCANCODES] = { false };

        if (!m_keyboardState[static_cast<i32>(t_key)] && prevKeyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = false;
            return true;
        }
        if (m_keyboardState[static_cast<i32>(t_key)])
        {
            prevKeyboardState[static_cast<i32>(t_key)] = true;
        }

        return false;
    }
}