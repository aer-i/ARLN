#pragma once
#include "ArlnUtility.hpp"
#include "ArlnImage.hpp"

namespace arln {

    class Swapchain
    {
    public:
        Swapchain() = default;
        Swapchain(Swapchain const&) = delete;
        Swapchain(Swapchain&&) = delete;
        Swapchain& operator=(Swapchain const&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;
        ~Swapchain() = default;

        void acquireNextImage(VkSemaphore t_semaphore) noexcept;
        void create() noexcept;
        void teardown() noexcept;
        void recreate() noexcept;

        inline auto  getHandle()     const { return m_handle;                }
        inline auto  getImageIndex() const { return m_imageIndex;            }
        inline auto& getFormat()     const { return m_format.format;         }
        inline auto& getExtent()     const { return m_extent;                }
        inline auto& getImage()      const { return m_images[m_imageIndex];  }

    private:
        VkSwapchainKHR           m_handle{ };
        VkExtent2D               m_extent;
        VkSurfaceFormatKHR       m_format;
        std::vector<Image>       m_images;
        u32                      m_imageIndex;
    };
}