#include "ArlnDescriptor.hpp"
#include "ArlnContext.hpp"

namespace arln {

    void DescriptorPool::create() noexcept
    {
        u32 descriptorCount = std::max(m_maxBindingCount * 2, 64u);

        std::array const descriptorPoolSizes = {
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER,                descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, descriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptorCount },
        };

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        descriptorPoolCreateInfo.maxSets = 64;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
        descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

        VkDescriptorPool descriptorPool;
        if (vkCreateDescriptorPool(CurrentContext()->getDevice(), &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create descriptor pool");
        }

        m_pools.emplace_back(descriptorPool);
    }

    DescriptorPool::DescriptorPool(std::nullptr_t) noexcept
        : m_maxBindingCount{ 0 }
    {
        create();
    }

    auto DescriptorPool::addBinding(u32 t_binding, DescriptorType t_type, ShaderStage t_stage, u32 t_count) noexcept -> DescriptorPool&
    {
        m_maxBindingCount = std::max(m_maxBindingCount, t_count);
        m_bindings.emplace_back(t_binding, static_cast<VkDescriptorType>(t_type), t_count, static_cast<VkShaderStageFlags>(t_stage), nullptr);

        return *this;
    }

    auto DescriptorPool::createDescriptor(u32 t_setLayout) noexcept -> Descriptor
    {
        if (t_setLayout + 1 > m_setLayouts.size())
        {
            std::vector<VkDescriptorBindingFlags> descriptorBindingFlags(
                m_bindings.size(),
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            );

            VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags;
            setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setLayoutBindingFlags.pNext = nullptr;
            setLayoutBindingFlags.bindingCount = static_cast<uint32_t>(descriptorBindingFlags.size());
            setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;
            descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
            descriptorSetLayoutCreateInfo.pBindings = m_bindings.data();

            VkDescriptorSetLayout layout;
            if (vkCreateDescriptorSetLayout(CurrentContext()->getDevice(), &descriptorSetLayoutCreateInfo, nullptr, &layout) != VK_SUCCESS)
            {
                CurrentContext()->getErrorCallback()("Failed to create descriptor set layout");
            }

            m_setLayouts.emplace_back(layout);
        }

        createDescriptorSet:

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = nullptr;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.descriptorPool = m_pools.back();
        descriptorSetAllocateInfo.pSetLayouts = &m_setLayouts[t_setLayout];

        VkDescriptorSet descriptorSet;
        switch (vkAllocateDescriptorSets(CurrentContext()->getDevice(), &descriptorSetAllocateInfo, &descriptorSet))
        {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_FRAGMENTED_POOL:
            create();
            goto createDescriptorSet;
        default:
            CurrentContext()->getErrorCallback()("Failed to allocate descriptor set");
            break;
        }

        m_bindings.clear();

        return { descriptorSet, m_setLayouts[t_setLayout] };
    }

    void DescriptorPool::destroy() noexcept
    {
        for (auto layout : m_setLayouts)
        {
            if (layout) vkDestroyDescriptorSetLayout(CurrentContext()->getDevice(), layout, nullptr);
        }

        for (auto pool : m_pools)
        {
            CurrentContext()->getFrame().addDescriptorPoolToDestroy(pool);
        }
    }

    void DescriptorPool::reset() noexcept
    {
        for (auto pool : m_pools)
        {
            vkResetDescriptorPool(CurrentContext()->getDevice(), pool, 0);
        }
    }

    auto DescriptorWriter::addBuffer(Descriptor& t_descriptor, Buffer& t_buffer, u32 t_binding, DescriptorType t_type, u32 t_element) noexcept -> DescriptorWriter&
    {
        m_bufferInfos.push_back(VkDescriptorBufferInfo{
            .buffer = t_buffer.getHandle(),
            .offset = 0,
            .range = t_buffer.getAllocationInfo().size
        });

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.pNext = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;
        descriptorWrite.dstArrayElement = t_element;
        descriptorWrite.dstBinding = t_binding;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = static_cast<VkDescriptorType>(t_type);
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pBufferInfo = &m_bufferInfos.back();
        descriptorWrite.dstSet = t_descriptor.getSet();

        m_writes.emplace_back(descriptorWrite);

        return *this;
    }

    auto DescriptorWriter::addImage(Descriptor& t_descriptor, Image* t_image, Sampler* t_sampler, u32 t_binding, DescriptorType t_type, u32 t_element) noexcept -> DescriptorWriter&
    {
        m_imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = (t_sampler) ? t_sampler->getHandle() : nullptr,
            .imageView = (t_image) ? t_image->getView() : nullptr,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        });

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.pNext = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;
        descriptorWrite.dstArrayElement = t_element;
        descriptorWrite.dstBinding = t_binding;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = static_cast<VkDescriptorType>(t_type);
        descriptorWrite.pImageInfo = &m_imageInfos.back();
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.dstSet = t_descriptor.getSet();

        m_writes.emplace_back(descriptorWrite);

        return *this;
    }

    void DescriptorWriter::write() noexcept
    {
        vkUpdateDescriptorSets(CurrentContext()->getDevice(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
    }

    void DescriptorWriter::clear() noexcept
    {
        m_writes.clear();
        m_imageInfos.clear();
        m_bufferInfos.clear();
    }
}