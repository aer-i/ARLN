#include "ArlnFrame.hpp"
#include "ArlnContext.hpp"
#include "ArlnPipeline.hpp"
#include "ArlnCommandBuffer.hpp"

namespace arln {

    void Frame::beginFrame() noexcept
    {
        m_currentFrame = m_frameContexts[m_frameIndex];

        vkWaitForFences(CurrentContext()->getDevice(), 1, &m_currentFrame.get().renderFence, false, UINT64_MAX);
        vkResetFences(CurrentContext()->getDevice(), 1, &m_currentFrame.get().renderFence);

        for (auto& image : m_currentFrame.get().imagesToFree)
        {
            if (image.getView()) vkDestroyImageView(CurrentContext()->getDevice(), image.getView(), nullptr);
            if (image.getAllocation()) vmaDestroyImage(CurrentContext()->getAllocator(), image.getHandle(), image.getAllocation());
        }

        for (auto& buffer : m_currentFrame.get().buffersToFree)
        {
            vmaDestroyBuffer(CurrentContext()->getAllocator(), buffer.getHandle(), buffer.getAllocation());
        }

        for (auto& pipeline : m_currentFrame.get().pipelinesToFree)
        {
            if (pipeline.getHandle()) vkDestroyPipeline(CurrentContext()->getDevice(), pipeline.getHandle(), nullptr);
            if (pipeline.getLayout()) vkDestroyPipelineLayout(CurrentContext()->getDevice(), pipeline.getLayout(), nullptr);
        }

        for (auto pool : m_currentFrame.get().descriptorPoolsToFree)
        {
            if (pool) vkDestroyDescriptorPool(CurrentContext()->getDevice(), pool, nullptr);
        }

        m_currentFrame.get().imagesToFree.clear();
        m_currentFrame.get().buffersToFree.clear();
        m_currentFrame.get().pipelinesToFree.clear();
        m_currentFrame.get().descriptorPoolsToFree.clear();

        static auto previousWidth = CurrentContext()->getWindowWidth();
        static auto previousHeight = CurrentContext()->getWindowHeight();

        if (previousHeight != CurrentContext()->getWindowHeight() ||
            previousWidth != CurrentContext()->getWindowWidth())
        {
            CurrentContext()->getSwapchain().recreate();
            previousWidth = CurrentContext()->getWindowWidth();
            previousHeight = CurrentContext()->getWindowHeight();
        }

        CurrentContext()->getSwapchain().acquireNextImage(
            m_currentFrame.get().imageAvailableSemaphore
        );

        for (auto commandPool : m_currentFrame.get().commandPools)
        {
            vkResetCommandPool(CurrentContext()->getDevice(), commandPool, 0);
        }
    }

    void Frame::endFrame(std::span<const CommandBufferHandle> t_commandBuffers) noexcept
    {
        {
            VkPipelineStageFlags constexpr pipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            VkSubmitInfo submitInfo;
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.commandBufferCount = static_cast<u32>(t_commandBuffers.size());
            submitInfo.pCommandBuffers = t_commandBuffers.data();
            submitInfo.pWaitDstStageMask = &pipelineStage;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &m_currentFrame.get().imageAvailableSemaphore;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &m_currentFrame.get().renderFinishedSemaphore;

            vkQueueSubmit(CurrentContext()->getGraphicsQueue(), 1, &submitInfo, m_currentFrame.get().renderFence);
        }
        {
            VkSwapchainKHR swapchain = CurrentContext()->getSwapchain().getHandle();
            u32 imageIndex = CurrentContext()->getSwapchain().getImageIndex();

            VkPresentInfoKHR presentInfo;
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &m_currentFrame.get().renderFinishedSemaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr;

            switch (vkQueuePresentKHR(CurrentContext()->getPresentQueue(), &presentInfo))
            {
            case VK_SUCCESS:
                break;
            case VK_SUBOPTIMAL_KHR:
            case VK_ERROR_OUT_OF_DATE_KHR:
                CurrentContext()->getSwapchain().recreate();
                break;
            default:
                CurrentContext()->getErrorCallback()("Failed to present image");
                break;
            }
        }

        m_frameIndex = (m_frameIndex + 1) % s_frameCount<u32>;
    }

    void Frame::create() noexcept
    {
        for (auto& frame : m_frameContexts)
        {
            VkFenceCreateInfo fenceCreateInfo;
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.pNext = nullptr;
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateFence(CurrentContext()->getDevice(), &fenceCreateInfo, nullptr, &frame.renderFence) != VK_SUCCESS)
            {
                CurrentContext()->getErrorCallback()("Failed to create vulkan fence");
            }

            VkSemaphoreCreateInfo semaphoreCreateInfo;
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreCreateInfo.pNext = nullptr;
            semaphoreCreateInfo.flags = 0;

            if (vkCreateSemaphore(CurrentContext()->getDevice(), &semaphoreCreateInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS ||
                vkCreateSemaphore(CurrentContext()->getDevice(), &semaphoreCreateInfo, nullptr, &frame.renderFinishedSemaphore) != VK_SUCCESS)
            {
                CurrentContext()->getErrorCallback()("Failed to create vulkan semaphore");
            }

            frame.stagingBuffer.recreate(0, MemoryType::eCpu, 64 * 1024 * 1024);
        }
    }

    void Frame::teardown() noexcept
    {
        for (auto& fc : m_frameContexts)
        {
            fc.stagingBuffer.free();
        }

        for (auto& fc : m_frameContexts)
        {
            if (fc.renderFence) vkDestroyFence(CurrentContext()->getDevice(), fc.renderFence, nullptr);
            if (fc.renderFinishedSemaphore) vkDestroySemaphore(CurrentContext()->getDevice(), fc.renderFinishedSemaphore, nullptr);
            if (fc.imageAvailableSemaphore) vkDestroySemaphore(CurrentContext()->getDevice(), fc.imageAvailableSemaphore, nullptr);

            for (auto& image : fc.imagesToFree)
            {
                if (image.getView()) vkDestroyImageView(CurrentContext()->getDevice(), image.getView(), nullptr);
                if (image.getAllocation()) vmaDestroyImage(CurrentContext()->getAllocator(), image.getHandle(), image.getAllocation());
            }

            for (auto& buffer : fc.buffersToFree)
            {
                vmaDestroyBuffer(CurrentContext()->getAllocator(), buffer.getHandle(), buffer.getAllocation());
            }

            for (auto& pipeline : fc.pipelinesToFree)
            {
                if (pipeline.getHandle()) vkDestroyPipeline(CurrentContext()->getDevice(), pipeline.getHandle(), nullptr);
                if (pipeline.getLayout()) vkDestroyPipelineLayout(CurrentContext()->getDevice(), pipeline.getLayout(), nullptr);
            }

            for (auto pool : fc.descriptorPoolsToFree)
            {
                if (pool) vkDestroyDescriptorPool(CurrentContext()->getDevice(), pool, nullptr);
            }

            for (auto pool : fc.commandPools)
            {
                if (pool) vkDestroyCommandPool(CurrentContext()->getDevice(), pool, nullptr);;
            }
        }
    }

    auto Frame::allocateCommandBuffers() noexcept -> CommandBuffer
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = nullptr;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        commandPoolCreateInfo.queueFamilyIndex = CurrentContext()->getQueueIndex();

        std::array<VkCommandPool, s_frameCount<u32>> commandPools{};

        for (u32 i = s_frameCount<u32>; i--; )
        {
            VkCommandPool commandPool;
            if (vkCreateCommandPool(CurrentContext()->getDevice(), &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
            {
                CurrentContext()->getErrorCallback()("Failed to create command pool");
            }
            m_frameContexts[i].commandPools.emplace_back(commandPool);

            commandPools[i] = m_frameContexts[i].commandPools.back();
        }

        return CommandBuffer(commandPools);
    }

    void Frame::addImageToFree(Image t_imageToFree) noexcept
    {
        m_currentFrame.get().imagesToFree.push_back(t_imageToFree);
    }

    void Frame::addBufferToFree(Buffer t_buffer) noexcept
    {
        m_currentFrame.get().buffersToFree.push_back(t_buffer);
    }

    void Frame::addPipelineToDestroy(Pipeline t_pipeline) noexcept
    {
        m_currentFrame.get().pipelinesToFree.push_back(t_pipeline);
    }

    void Frame::addDescriptorPoolToDestroy(VkDescriptorPool t_pool) noexcept
    {
        m_currentFrame.get().descriptorPoolsToFree.push_back(t_pool);
    }
}