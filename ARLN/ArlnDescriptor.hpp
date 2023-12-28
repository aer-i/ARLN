#pragma once
#include "ArlnUtility.hpp"

namespace arln {

    class Descriptor
    {
    private:
        friend class DescriptorPool;
        Descriptor(VkDescriptorSet t_set, VkDescriptorSetLayout t_layout) noexcept
            : m_set{ t_set }, m_layout{ t_layout } {}

    public:
        Descriptor() = default;
        Descriptor(Descriptor const&) = default;
        Descriptor(Descriptor&&) = default;
        Descriptor& operator=(Descriptor const&) = default;
        Descriptor& operator=(Descriptor&&) = default;
        ~Descriptor() = default;

        inline auto& getSet()    const noexcept { return m_set; }
        inline auto& getLayout() const noexcept { return m_layout; }

    private:
        VkDescriptorSet       m_set;
        VkDescriptorSetLayout m_layout;
    };

    class DescriptorPool
    {
    private:
        friend class Context;
        DescriptorPool(std::nullptr_t) noexcept;

        void create() noexcept;

    public:
        DescriptorPool() = default;
        DescriptorPool(DescriptorPool const&) = default;
        DescriptorPool(DescriptorPool&&) = default;
        DescriptorPool& operator=(DescriptorPool const&) = default;
        DescriptorPool& operator=(DescriptorPool&&) = default;
        ~DescriptorPool() = default;

        auto addBinding(u32 t_binding, DescriptorType t_type, ShaderStage t_stage, u32 t_count = 1) noexcept -> DescriptorPool&;
        auto createDescriptor(u32 t_setLayout = 0) noexcept -> Descriptor;
        void destroy() noexcept;
        void reset() noexcept;

    private:
        std::vector<VkDescriptorPool>             m_pools;
        std::vector<VkDescriptorSetLayout>        m_setLayouts;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
        u32                                       m_maxBindingCount;
    };

    class DescriptorWriter
    {
    public:
        DescriptorWriter() = default;
        DescriptorWriter(DescriptorWriter const&) = default;
        DescriptorWriter(DescriptorWriter&&) = default;
        DescriptorWriter& operator=(DescriptorWriter const&) = default;
        DescriptorWriter& operator=(DescriptorWriter&&) = default;
        ~DescriptorWriter() = default;

        auto addBuffer(Descriptor& t_descriptor, Buffer& t_buffer, u32 t_binding, DescriptorType t_type, u32 t_element = 0) noexcept -> DescriptorWriter&;
        auto addImage(Descriptor& t_descriptor, Image* t_image, Sampler* t_sampler, u32 t_binding, DescriptorType t_type, u32 t_element = 0) noexcept -> DescriptorWriter&;
        void write() noexcept;
        void clear() noexcept;

    private:
        std::vector<VkDescriptorBufferInfo> m_bufferInfos;
        std::vector<VkDescriptorImageInfo> m_imageInfos;
        std::vector<VkWriteDescriptorSet> m_writes;
    };
}