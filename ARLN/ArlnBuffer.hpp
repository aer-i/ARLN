#pragma once
#include "ArlnUtility.hpp"

namespace arln {

    class Buffer
    {
    private:
        friend class Context;
        Buffer(BufferUsage t_usage, MemoryType t_memoryType, size_t t_size) noexcept;

    public:
        Buffer() = default;
        ~Buffer() = default;
        Buffer(Buffer const&) = default;
        Buffer(Buffer&&) = default;
        Buffer& operator=(Buffer const&) = default;
        Buffer& operator=(Buffer&&) = default;

        void recreate(BufferUsage t_usage, MemoryType t_memoryType, size_t t_size) noexcept;
        void free() noexcept;
        void writeData(void const* t_data, size_t t_size, size_t t_offset = 0) noexcept;

        inline auto& getHandle()         const noexcept { return m_handle;              }
        inline auto& getAllocation()     const noexcept { return m_allocation;          }
        inline auto& getAllocationInfo() const noexcept { return m_allocationInfo;      }
        inline auto& getSize()           const noexcept { return m_allocationInfo.size; }
        inline auto* getDeviceAddress()  const noexcept { return &m_deviceAddress;      }

    private:
        VkBuffer          m_handle        { };
        VmaAllocation     m_allocation    { };
        VmaAllocationInfo m_allocationInfo{ };
        u64               m_deviceAddress { };
    };
}