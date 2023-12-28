#pragma once
#include "ArlnUtility.hpp"
#include "ArlnPipeline.hpp"
#include "ArlnBuffer.hpp"
#include "ArlnImage.hpp"

namespace arln {

    class Frame
    {
    public:
        Frame() = default;
        Frame(Frame const&) = delete;
        Frame(Frame&&) = delete;
        Frame& operator=(Frame const&) = delete;
        Frame& operator=(Frame&&) = delete;
        ~Frame() = default;

        void beginFrame() noexcept;
        void endFrame(std::span<const CommandBufferHandle> t_commandBuffers) noexcept;
        void create() noexcept;
        void teardown() noexcept;
        auto allocateCommandBuffers() noexcept -> CommandBuffer;
        void addImageToFree(Image t_imageToFree) noexcept;
        void addBufferToFree(Buffer t_buffer) noexcept;
        void addPipelineToDestroy(Pipeline t_pipeline) noexcept;
        void addDescriptorPoolToDestroy(VkDescriptorPool t_pool) noexcept;

        template<typename T>
        static constexpr T s_frameCount = static_cast<T>(2);
        inline auto  getIndex() const   noexcept { return m_frameIndex; }
        inline auto& getStagingBuffer() noexcept { return m_currentFrame.get().stagingBuffer; }

    private:
        struct FrameContext
        {
            std::vector<Image>            imagesToFree;
            std::vector<Buffer>           buffersToFree;
            std::vector<Pipeline>         pipelinesToFree;
            std::vector<VkDescriptorPool> descriptorPoolsToFree;
            std::vector<VkCommandPool>    commandPools;
            VkSemaphore                   imageAvailableSemaphore;
            VkSemaphore                   renderFinishedSemaphore;
            VkFence                       renderFence;
            Buffer                        stagingBuffer;
        };

        using FrameContextArray = std::array<FrameContext, s_frameCount<u32>>;
        using FrameContextRef   = std::reference_wrapper<FrameContext>;

        FrameContextArray m_frameContexts{                        };
        FrameContextRef   m_currentFrame { m_frameContexts.back() };
        u32               m_frameIndex   {                        };
    };
}