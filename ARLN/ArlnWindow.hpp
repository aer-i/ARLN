#pragma once
#include "ArlnUtility.hpp"
#include <SDL3/SDL.h>

namespace arln {

    enum class Key : i32;
    enum class Button : i32;

    struct WindowCreateInfo
    {
        ivec2 size = { 1280, 720 };
        ivec2 position = { 0.f , 0.f };
        std::string_view title = "ARLN Application";
        std::function<void(std::string_view)> errorCallback = [](std::string_view){};
        bool resizable = true;
        bool customPosition = false;
    };

    class Window
    {
    public:
        explicit Window(WindowCreateInfo const& t_createInfo);
        Window(Window const&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&&) = delete;
        ~Window();

        void pollEvents() noexcept;
        auto createSurface(VkInstance t_instance) const noexcept -> VkSurfaceKHR;
        auto getInstanceExtensions() const noexcept -> std::vector<const char*>;

        auto getTime() const noexcept -> f64;
        void setPosition(ivec2 const& t_position) noexcept;
        auto getPosition() const noexcept -> ivec2;
        void setSize(ivec2 const& t_size) noexcept;
        void setTitle(std::string_view t_tile) noexcept;
        bool getKey(Key t_key) noexcept;
        bool getKeyDown(Key t_key) noexcept;
        bool getKeyUp(Key t_key) noexcept;

        inline void setShouldClose(bool t_shouldClose = true) { m_shouldClose = t_shouldClose; }

        [[nodiscard]] inline auto getHandle()        const noexcept { return m_handle;        }
        [[nodiscard]] inline auto shouldClose()      const noexcept { return m_shouldClose;   }
        [[nodiscard]] inline auto getWidth()         const noexcept { return m_size.x;        }
        [[nodiscard]] inline auto getHeight()        const noexcept { return m_size.y;        }
        [[nodiscard]] inline auto getFps()           const noexcept { return m_fps;           }
        [[nodiscard]] inline auto getSize()          const noexcept { return m_size;          }
        [[nodiscard]] inline auto getTitle()         const noexcept { return m_title.c_str(); }
        [[nodiscard]] inline auto getCursorOffsetX() const noexcept { return m_cursorOffsetX; }
        [[nodiscard]] inline auto getCursorOffsetY() const noexcept { return m_cursorOffsetY; }

    private:
        SDL_Window* m_handle{ nullptr };
        SDL_Event m_event{ };
        uvec2 m_size;
        std::string m_title;
        u8* m_keyboardState{ nullptr };
        u32 m_fps{ 0 };
        f32 m_cursorOffsetX{ };
        f32 m_cursorOffsetY{ };
        bool m_shouldClose{ false };
    };

    enum class Key : i32
    {
        eUnknown         = SDL_SCANCODE_UNKNOWN,
        eN0			     = SDL_SCANCODE_0,
        eN1			     = SDL_SCANCODE_1,
        eN2			     = SDL_SCANCODE_2,
        eN3			     = SDL_SCANCODE_3,
        eN4			     = SDL_SCANCODE_4,
        eN5			     = SDL_SCANCODE_5,
        eN6			     = SDL_SCANCODE_6,
        eN7			     = SDL_SCANCODE_7,
        eN8			     = SDL_SCANCODE_8,
        eN9			     = SDL_SCANCODE_9,
        eQ				 = SDL_SCANCODE_Q,
        eW				 = SDL_SCANCODE_W,
        eE				 = SDL_SCANCODE_E,
        eR				 = SDL_SCANCODE_R,
        eT				 = SDL_SCANCODE_T,
        eY				 = SDL_SCANCODE_Y,
        eU				 = SDL_SCANCODE_U,
        eI				 = SDL_SCANCODE_I,
        eO				 = SDL_SCANCODE_O,
        eP				 = SDL_SCANCODE_P,
        eA				 = SDL_SCANCODE_A,
        eS				 = SDL_SCANCODE_S,
        eD				 = SDL_SCANCODE_D,
        eF				 = SDL_SCANCODE_F,
        eG				 = SDL_SCANCODE_G,
        eH				 = SDL_SCANCODE_H,
        eJ				 = SDL_SCANCODE_J,
        eK				 = SDL_SCANCODE_K,
        eL				 = SDL_SCANCODE_L,
        eZ				 = SDL_SCANCODE_Z,
        eX				 = SDL_SCANCODE_X,
        eC				 = SDL_SCANCODE_C,
        eV				 = SDL_SCANCODE_V,
        eB				 = SDL_SCANCODE_B,
        eN				 = SDL_SCANCODE_N,
        eM				 = SDL_SCANCODE_M,
        eTab			 = SDL_SCANCODE_TAB,
        eLeftArrow		 = SDL_SCANCODE_LEFT,
        eRightArrow	     = SDL_SCANCODE_RIGHT,
        eUpArrow		 = SDL_SCANCODE_UP,
        eDownArrow		 = SDL_SCANCODE_DOWN,
        ePageUp		     = SDL_SCANCODE_PAGEUP,
        ePageDown		 = SDL_SCANCODE_PAGEDOWN,
        eHome			 = SDL_SCANCODE_HOME,
        eEnd			 = SDL_SCANCODE_END,
        eInsert		     = SDL_SCANCODE_INSERT,
        eDelete		     = SDL_SCANCODE_DELETE,
        eBackspace		 = SDL_SCANCODE_BACKSPACE,
        eSpace			 = SDL_SCANCODE_SPACE,
        eEnter			 = SDL_SCANCODE_RETURN,
        eEscape		     = SDL_SCANCODE_ESCAPE,
        eApostrophe	     = SDL_SCANCODE_APOSTROPHE,
        eComma			 = SDL_SCANCODE_COMMA,
        eMinus			 = SDL_SCANCODE_MINUS,
        ePeriod		     = SDL_SCANCODE_PERIOD,
        eSlash			 = SDL_SCANCODE_SLASH,
        eSemicolon		 = SDL_SCANCODE_SEMICOLON,
        eEqual			 = SDL_SCANCODE_EQUALS,
        eLeftBracket	 = SDL_SCANCODE_LEFTBRACKET,
        eBackslash		 = SDL_SCANCODE_BACKSLASH,
        eRightBracket	 = SDL_SCANCODE_RIGHTBRACKET,
        eGraveAccent	 = SDL_SCANCODE_GRAVE,
        eCapsLock		 = SDL_SCANCODE_CAPSLOCK,
        eScrollLock	     = SDL_SCANCODE_SCROLLLOCK,
        eNumLock		 = SDL_SCANCODE_NUMLOCKCLEAR,
        ePrintScreen	 = SDL_SCANCODE_PRINTSCREEN,
        ePause			 = SDL_SCANCODE_PAUSE,
        eKeypad0		 = SDL_SCANCODE_KP_0,
        eKeypad1		 = SDL_SCANCODE_KP_1,
        eKeypad2		 = SDL_SCANCODE_KP_2,
        eKeypad3		 = SDL_SCANCODE_KP_3,
        eKeypad4		 = SDL_SCANCODE_KP_4,
        eKeypad5		 = SDL_SCANCODE_KP_5,
        eKeypad6		 = SDL_SCANCODE_KP_6,
        eKeypad7		 = SDL_SCANCODE_KP_7,
        eKeypad8		 = SDL_SCANCODE_KP_8,
        eKeypad9		 = SDL_SCANCODE_KP_9,
        eKeypadDecimal   = SDL_SCANCODE_DECIMALSEPARATOR,
        eKeypadDivide	 = SDL_SCANCODE_KP_DIVIDE,
        eKeypadMultiply  = SDL_SCANCODE_KP_MULTIPLY,
        eKeypadSubtract  = SDL_SCANCODE_KP_MINUS,
        eKeypadAdd		 = SDL_SCANCODE_KP_PLUS,
        eLeftShift		 = SDL_SCANCODE_LSHIFT,
        eLeftCtrl		 = SDL_SCANCODE_LCTRL,
        eLeftAlt		 = SDL_SCANCODE_LALT,
        eLeftSuper		 = SDL_SCANCODE_LGUI,
        eRightSuper	     = SDL_SCANCODE_RGUI,
        eRightShift	     = SDL_SCANCODE_RSHIFT,
        eRightCtrl		 = SDL_SCANCODE_RCTRL,
        eRightAlt		 = SDL_SCANCODE_RALT,
        eMenu			 = SDL_SCANCODE_MENU,
        eF1			     = SDL_SCANCODE_F1,
        eF2			     = SDL_SCANCODE_F2,
        eF3			     = SDL_SCANCODE_F3,
        eF4			     = SDL_SCANCODE_F4,
        eF5			     = SDL_SCANCODE_F5,
        eF6			     = SDL_SCANCODE_F6,
        eF7			     = SDL_SCANCODE_F7,
        eF8			     = SDL_SCANCODE_F8,
        eF9			     = SDL_SCANCODE_F9,
        eF10			 = SDL_SCANCODE_F10,
        eF11			 = SDL_SCANCODE_F11,
        eF12			 = SDL_SCANCODE_F12
    };
}
