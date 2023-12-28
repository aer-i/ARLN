#include "ArlnImage.hpp"
#include "ArlnContext.hpp"

namespace arln {

    Sampler::Sampler(SamplerOptions const& t_options) noexcept
    {
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.flags = 0;
        samplerCreateInfo.pNext = nullptr;
        samplerCreateInfo.magFilter = (VkFilter)t_options.magFilter;
        samplerCreateInfo.minFilter = (VkFilter)t_options.minFilter;
        samplerCreateInfo.mipmapMode = (VkSamplerMipmapMode)t_options.mipmapMode;
        samplerCreateInfo.addressModeU = (VkSamplerAddressMode)t_options.addressModeU;
        samplerCreateInfo.addressModeV = (VkSamplerAddressMode)t_options.addressModeV;
        samplerCreateInfo.addressModeW = (VkSamplerAddressMode)t_options.addressModeW;
        samplerCreateInfo.minLod = t_options.minLod;
        samplerCreateInfo.maxLod = t_options.maxLod;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.unnormalizedCoordinates = t_options.unnormalizedCoordinates;

        if (vkCreateSampler(CurrentContext()->getDevice(), &samplerCreateInfo, nullptr, &m_handle) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create sampler");
        }
    }

    void Sampler::destroy() noexcept
    {
        if (m_handle) vkDestroySampler(CurrentContext()->getDevice(), m_handle, nullptr);
    }

    Image::Image(u32 t_width, u32 t_height, Format t_format, ImageUsage t_usage, MemoryType t_memoryType) noexcept
    {
        this->recreate(t_width, t_height, t_format, t_usage, t_memoryType);
    }

    Image::Image(VkImage t_image, VkImageView t_imageView) noexcept
    {
        this->recreate(t_image, t_imageView);
    }

    void Image::recreate(VkImage t_image, VkImageView t_imageView) noexcept
    {
        this->free();

        m_handle = t_image;
        m_view = t_imageView;
    }

    void Image::recreate(u32 t_width, u32 t_height, Format t_format, ImageUsage t_usage, MemoryType t_memoryType) noexcept
    {
        this->free();

        VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageCreateInfo.format = static_cast<VkFormat>(t_format);
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = t_width;
        imageCreateInfo.extent.height = t_height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | t_usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        switch (t_memoryType)
        {
        case MemoryType::eGpu:
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                         VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryType::eGpuOnly:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;
        case MemoryType::eDedicated:
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            break;
        case MemoryType::eCpu:
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                         VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        default:
            break;
        }

        VmaAllocationInfo allocationInfo;
        if (vmaCreateImage(CurrentContext()->getAllocator(), &imageCreateInfo, &allocationCreateInfo, &m_handle, &m_allocation, &allocationInfo) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to allocate image");
        }

        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = m_handle;
        imageViewCreateInfo.format = static_cast<VkFormat>(t_format);
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange = VkImageSubresourceRange{
            .aspectMask = t_usage & ImageUsageBits::eDepthStencilAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : (u32)VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        if (vkCreateImageView(CurrentContext()->getDevice(), &imageViewCreateInfo, nullptr, &m_view) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create image view");
        }
    }

    void Image::free() noexcept
    {
        if (m_handle)
        {
            CurrentContext()->getFrame().addImageToFree(*this);

            m_handle     = nullptr;
            m_view       = nullptr;
            m_allocation = nullptr;
        }
    }

    void Image::writeToImage(void const* t_data, size_t t_dataSize, uvec2 t_size) noexcept
    {
        if (t_dataSize > CurrentContext()->getFrame().getStagingBuffer().getSize())
        {
            Buffer staging;
            staging.recreate(0, MemoryType::eCpu, t_dataSize);

            staging.writeData(t_data, t_dataSize, 0);

            VkBufferImageCopy copy{};
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent = {
                .width = t_size.x,
                .height = t_size.y,
                .depth = 1
            };

            CurrentContext()->immediateSubmit([&](VkCommandBuffer t_cmd)
            {
                vkCmdCopyBufferToImage(t_cmd, staging.getHandle(), m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            });

            staging.free();
        }
        else
        {
            auto& staging = CurrentContext()->getFrame().getStagingBuffer();
            staging.writeData(t_data, t_dataSize, 0);

            VkBufferImageCopy copy{};
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent = {
                .width = t_size.x,
                .height = t_size.y,
                .depth = 1
            };

            CurrentContext()->immediateSubmit([&](VkCommandBuffer t_cmd)
            {
                vkCmdCopyBufferToImage(t_cmd, staging.getHandle(), m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            });
        }
    }

    void Image::transition(ImageLayout t_old, ImageLayout t_new, PipelineStage t_srcStage, PipelineStage t_dstStage, Access t_srcAccess, Access t_dstAccess) noexcept
    {
        VkImageMemoryBarrier2 barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.srcStageMask = t_srcStage;
        barrier.dstStageMask = t_dstStage;
        barrier.srcAccessMask = t_srcAccess;
        barrier.dstAccessMask = t_dstAccess;
        barrier.oldLayout = static_cast<VkImageLayout>(t_old);
        barrier.newLayout = static_cast<VkImageLayout>(t_new);
        barrier.image = m_handle;
        barrier.subresourceRange.aspectMask = t_new == ImageLayout::eDepthStencilAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.srcQueueFamilyIndex = 0;
        barrier.dstQueueFamilyIndex = 0;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        CurrentContext()->immediateSubmit([&](VkCommandBuffer t_cmd)
        {
            vkCmdPipelineBarrier2(t_cmd, &dependencyInfo);
        });
    }
}