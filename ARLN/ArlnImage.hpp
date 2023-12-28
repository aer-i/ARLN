#pragma once
#include "ArlnUtility.hpp"

namespace arln {

    class Sampler
    {
    private:
        friend class Context;
        explicit Sampler(SamplerOptions const& t_options) noexcept;

    public:
        Sampler() = default;
        ~Sampler() = default;
        Sampler(Sampler const&) = default;
        Sampler(Sampler&&) = default;
        Sampler& operator=(Sampler const&) = default;
        Sampler& operator=(Sampler&&) = default;

        void destroy() noexcept;

        inline auto getHandle() const noexcept { return m_handle; }
        inline operator Sampler*() const noexcept { return (Sampler*)this; }

    private:
        VkSampler m_handle{ };
    };

    class Image
    {
    private:
        friend class Context;
        friend class Swapchain;
        Image(u32 t_width, u32 t_height, Format t_format, ImageUsage t_usage, MemoryType t_memoryType) noexcept;
        Image(VkImage t_image, VkImageView t_imageView) noexcept;

        void recreate(VkImage t_image, VkImageView t_imageView) noexcept;

    public:
        Image() = default;
        ~Image() = default;
        Image(Image const&) = default;
        Image(Image&&) = default;
        Image& operator=(Image const&) = default;
        Image& operator=(Image&&) = default;

        void recreate(u32 t_width, u32 t_height, Format t_format, ImageUsage t_usage, MemoryType t_memoryType) noexcept;
        void free() noexcept;
        void transition(ImageLayout t_old, ImageLayout t_new, PipelineStage t_srcStage, PipelineStage t_dstStage, Access t_srcAccess, Access t_dstAccess) noexcept;
        void writeToImage(void const* t_data, size_t t_dataSize, uvec2 t_size) noexcept;

        inline auto& getHandle()     const noexcept { return m_handle;     }
        inline auto& getView()       const noexcept { return m_view;       }
        inline auto& getAllocation() const noexcept { return m_allocation; }
        inline operator Image*()     const noexcept { return (Image*)this; }

    private:
        VkImage       m_handle    { };
        VkImageView   m_view      { };
        VmaAllocation m_allocation{ };
    };
}