#include "ArlnBuffer.hpp"
#include "ArlnContext.hpp"
#include <cstring>

namespace arln {

    Buffer::Buffer(BufferUsage t_usage, MemoryType t_memoryType, size_t t_size) noexcept
    {
        recreate(t_usage, t_memoryType, t_size);
    }

    void Buffer::recreate(BufferUsage t_usage, MemoryType t_memoryType, size_t t_size) noexcept
    {
        free();

        VkBufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.flags = 0;
        bufferCreateInfo.usage = t_usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        bufferCreateInfo.size = t_size;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices = nullptr;

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        switch (t_memoryType)
        {
            case MemoryType::eGpu:
                bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;
            case MemoryType::eGpuOnly:
                bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case MemoryType::eDedicated:
                bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
                break;
            case MemoryType::eCpu:
                bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                             VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;
            default:
                break;
        }

        if (vmaCreateBuffer(
            CurrentContext()->getAllocator(),
            &bufferCreateInfo,
            &allocationCreateInfo,
            &m_handle,
            &m_allocation,
            &m_allocationInfo)
            != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to allocate buffer");
        }

        VkBufferDeviceAddressInfo bufferDeviceAddressInfo;
        bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAddressInfo.buffer = m_handle;
        bufferDeviceAddressInfo.pNext = nullptr;

        m_deviceAddress = vkGetBufferDeviceAddress(CurrentContext()->getDevice(), &bufferDeviceAddressInfo);
    }

    void Buffer::free() noexcept
    {
        if (m_handle)
        {
            CurrentContext()->getFrame().addBufferToFree(*this);
        }

        m_handle         = nullptr;
        m_allocation     = nullptr;
        m_allocationInfo = { };
    }

    void Buffer::writeData(void const* t_data, size_t t_size, size_t t_offset) noexcept
    {
        if (m_allocationInfo.memoryType & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            u8* memoryOffset = (u8*)m_allocationInfo.pMappedData;
            memoryOffset += t_offset;
            memcpy(memoryOffset, t_data, t_size);

            vmaFlushAllocation(CurrentContext()->getAllocator(), m_allocation, 0, ~0ULL);
        }
        else
        {
            if (t_size > CurrentContext()->getFrame().getStagingBuffer().getSize())
            {
                Buffer staging;
                staging.recreate(0, MemoryType::eCpu, t_size);

                staging.writeData(t_data, t_size);

                CurrentContext()->immediateSubmit([&](VkCommandBuffer t_cmd)
                {
                    VkBufferCopy bufferCopy = {
                        .srcOffset = 0,
                        .dstOffset = t_offset,
                        .size = t_size
                    };

                    vkCmdCopyBuffer(
                        t_cmd,
                        staging.getHandle(),
                        m_handle,
                        1,
                        &bufferCopy
                    );
                });

                staging.free();
            }
            else
            {
                CurrentContext()->getFrame().getStagingBuffer().writeData(t_data, t_size, 0);

                CurrentContext()->immediateSubmit([&](VkCommandBuffer t_cmd)
                {
                    VkBufferCopy bufferCopy = {
                        .srcOffset = 0,
                        .dstOffset = t_offset,
                        .size = t_size
                    };

                    vkCmdCopyBuffer(
                        t_cmd,
                        CurrentContext()->getFrame().getStagingBuffer().getHandle(),
                        m_handle,
                        1,
                        &bufferCopy
                    );
                });
            }

        }
    }
}