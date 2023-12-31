#include "ArlnSwapchain.hpp"
#include "ArlnContext.hpp"
#include <algorithm>

namespace arln {

    void Swapchain::acquireNextImage(VkSemaphore t_semaphore) noexcept
    {
        switch (vkAcquireNextImageKHR(
            CurrentContext()->getDevice(),
            m_handle,
            UINT64_MAX,
            t_semaphore,
            nullptr,
            &m_imageIndex
        ))
        {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            recreate();
            break;
        default:
            CurrentContext()->getErrorCallback()("Failed to acquire swapchain image");
            break;
        }
    }

    void Swapchain::create() noexcept
    {


        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(CurrentContext()->getPhysicalDevice(), CurrentContext()->getSurface(), &capabilities);

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        if (CurrentContext()->isPresentModeSupported(CurrentContext()->getSurfacePresentMode()))
        {
            presentMode = static_cast<VkPresentModeKHR>(CurrentContext()->getSurfacePresentMode());
        }

        m_extent.width = std::clamp(CurrentContext()->getWindowWidth(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_extent.height = std::clamp(CurrentContext()->getWindowHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.minImageCount    = std::max(3u, CurrentContext()->getSurfaceCapabilities().minImageCount);
        swapchainCreateInfo.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.surface          = CurrentContext()->getSurface();
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.imageFormat      = static_cast<VkFormat>(CurrentContext()->getDefaultColorFormat());
        swapchainCreateInfo.presentMode      = presentMode;
        swapchainCreateInfo.imageExtent      = m_extent;
        swapchainCreateInfo.oldSwapchain     = nullptr;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.clipped          = 1;

        if (vkCreateSwapchainKHR(CurrentContext()->getDevice(), &swapchainCreateInfo, nullptr, &m_handle) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create vulkan swapchain");
        }

        u32 imageCount;
        vkGetSwapchainImagesKHR(CurrentContext()->getDevice(), m_handle, &imageCount, nullptr);
        std::vector<VkImage> images(imageCount);
        vkGetSwapchainImagesKHR(CurrentContext()->getDevice(), m_handle, &imageCount, images.data());

        bool isImageVectorEmpty = m_images.empty();

        for (size_t i = 0; i < images.size(); ++i)
        {
            VkImageViewCreateInfo imageViewCreateInfo;
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.pNext = nullptr;
            imageViewCreateInfo.flags = 0;
            imageViewCreateInfo.image = images[i];
            imageViewCreateInfo.format = static_cast<VkFormat>(CurrentContext()->getDefaultColorFormat());
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.subresourceRange = VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };

            VkImageView imageView;
            vkCreateImageView(CurrentContext()->getDevice(), &imageViewCreateInfo, nullptr, &imageView);

            if (isImageVectorEmpty)
            {
                m_images.push_back({images[i], imageView});
            }
            else
            {
                m_images[i].recreate(images[i], imageView);
            }
        }

        CurrentContext()->getInfoCallback()(
            "Created vulkan swapchain [ width{" + std::to_string(m_extent.width)
            + "}, height{" + std::to_string(m_extent.height) + "} ]"
        );
    }

    void Swapchain::teardown() noexcept
    {
        for (auto& image : m_images)
            image.free();

        if (m_handle) vkDestroySwapchainKHR(CurrentContext()->getDevice(), m_handle, nullptr);
        m_handle = nullptr;
    }

    void Swapchain::recreate() noexcept
    {
        vkDeviceWaitIdle(CurrentContext()->getDevice());

        teardown();
        create();

        CurrentContext()->getResizeCallback()(m_extent.width, m_extent.height);
    }
}