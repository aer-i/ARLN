#include "ArlnCommandBuffer.hpp"
#include "ArlnContext.hpp"
#include "ArlnPipeline.hpp"
#include "ArlnDescriptor.hpp"

namespace arln {

    CommandBuffer::CommandBuffer(CommandPoolArray const& t_commandPools) noexcept
    {
        VkCommandBufferAllocateInfo allocateInfo;
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        for (size_t i = t_commandPools.size(); i--; )
        {
            allocateInfo.commandPool = t_commandPools[i];
            vkAllocateCommandBuffers(CurrentContext()->getDevice(), &allocateInfo, &m_commandBuffers[i]);
        }
    }

    void CommandBuffer::begin() noexcept
    {
        m_currentHandle = &m_commandBuffers[CurrentContext()->getFrame().getIndex()];

        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(*m_currentHandle, &beginInfo);
    }

    void CommandBuffer::end() noexcept
    {
        vkEndCommandBuffer(*m_currentHandle);
    }

    void CommandBuffer::beginRendering(RenderingInfo const& t_renderingInfo) noexcept
    {
        VkRenderingInfo renderingInfo{};
        VkRenderingAttachmentInfo colorAttachment;
        VkRenderingAttachmentInfo depthAttachment;

        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.pNext = nullptr;
        colorAttachment.imageView = t_renderingInfo.pColorAttachment->image->getView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = t_renderingInfo.pColorAttachment->late ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color.float32[0] = t_renderingInfo.pColorAttachment->clearColor[0];
        colorAttachment.clearValue.color.float32[1] = t_renderingInfo.pColorAttachment->clearColor[1];
        colorAttachment.clearValue.color.float32[2] = t_renderingInfo.pColorAttachment->clearColor[2];
        colorAttachment.clearValue.color.float32[3] = t_renderingInfo.pColorAttachment->clearColor[3];
        colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.resolveImageView = nullptr;
        colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

        if (t_renderingInfo.pDepthAttachment)
        {
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.pNext = nullptr;
            depthAttachment.imageView = t_renderingInfo.pDepthAttachment->image->getView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachment.loadOp = t_renderingInfo.pDepthAttachment->late ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                                            : VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.clearValue.depthStencil.depth = t_renderingInfo.pDepthAttachment->depth;
            depthAttachment.clearValue.depthStencil.stencil = t_renderingInfo.pDepthAttachment->stencil;
            depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.resolveImageView = nullptr;
            depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

            renderingInfo.pDepthAttachment = &depthAttachment;
        }

        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.pNext = nullptr;
        renderingInfo.flags = 0;
        renderingInfo.renderArea.extent.width = t_renderingInfo.size.x == 0 ? CurrentContext()->getSwapchain().getExtent().width : t_renderingInfo.size.x;
        renderingInfo.renderArea.extent.height = t_renderingInfo.size.y == 0 ? CurrentContext()->getSwapchain().getExtent().height : t_renderingInfo.size.y;
        renderingInfo.renderArea.offset.x = t_renderingInfo.offset.x;
        renderingInfo.renderArea.offset.y = t_renderingInfo.offset.y;
        renderingInfo.layerCount = 1;
        renderingInfo.viewMask = 0;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pStencilAttachment = nullptr;

        vkCmdBeginRendering(*m_currentHandle, &renderingInfo);
    }

    void CommandBuffer::endRendering() noexcept
    {
        vkCmdEndRendering(*m_currentHandle);
    }

    void CommandBuffer::bindGraphicsPipeline(Pipeline& t_pipeline) noexcept
    {
        vkCmdBindPipeline(*m_currentHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, t_pipeline.getHandle());
    }

    void CommandBuffer::bindComputePipeline(Pipeline& t_pipeline) noexcept
    {
        vkCmdBindPipeline(*m_currentHandle, VK_PIPELINE_BIND_POINT_COMPUTE, t_pipeline.getHandle());
    }

    void CommandBuffer::dispatch(u32 t_x, u32 t_y, u32 t_z) noexcept
    {
        vkCmdDispatch(*m_currentHandle, t_x, t_y, t_z);
    }

    void CommandBuffer::draw(u32 t_vertexCount, u32 t_instanceCount, i32 t_firstVertex, u32 t_firstInstance) noexcept
    {
        vkCmdDraw(*m_currentHandle, t_vertexCount, t_instanceCount, t_firstVertex, t_firstInstance);
    }

    void CommandBuffer::drawIndexed(u32 t_indexCount, u32 t_instanceCount, u32 t_firstIndex, i32 t_vertexOffset, u32 t_firstInstance) noexcept
    {
        vkCmdDrawIndexed(*m_currentHandle, t_indexCount, t_instanceCount, t_firstIndex, t_vertexOffset, t_firstInstance);
    }

    void CommandBuffer::drawIndirect(Buffer& t_buffer, size_t t_offset, u32 t_drawCount, u32 t_stride) noexcept
    {
        vkCmdDrawIndirect(*m_currentHandle, t_buffer.getHandle(), t_offset, t_drawCount, t_stride);
    }

    void CommandBuffer::drawIndirectCount(Buffer& t_buffer, size_t t_offset, Buffer& t_countBuffer, size_t t_countBufferOffset, u32 t_maxDrawCount, u32 t_stride) noexcept
    {
        vkCmdDrawIndirectCount(*m_currentHandle, t_buffer.getHandle(), t_offset, t_countBuffer.getHandle(), t_countBufferOffset, t_maxDrawCount, t_stride);
    }

    void CommandBuffer::drawIndexedIndirect(Buffer& t_buffer, size_t t_offset, u32 t_drawCount, u32 t_stride) noexcept
    {
        vkCmdDrawIndexedIndirect(*m_currentHandle, t_buffer.getHandle(), t_offset, t_drawCount, t_stride);
    }

    void CommandBuffer::drawIndexedIndirectCount(Buffer& t_buffer, size_t t_offset, Buffer& t_countBuffer, size_t t_countBufferOffset, u32 t_maxDrawCount, u32 t_stride) noexcept
    {
        vkCmdDrawIndexedIndirectCount(*m_currentHandle, t_buffer.getHandle(), t_offset, t_countBuffer.getHandle(), t_countBufferOffset, t_maxDrawCount, t_stride);
    }

    void CommandBuffer::drawMeshTask(u32 t_x, u32 t_y, u32 t_z) noexcept
    {
        vkCmdDrawMeshTasksEXT(*m_currentHandle, t_x, t_y, t_z);
    }

    void CommandBuffer::drawMeshTaskIndirect(Buffer& t_buffer, size_t t_offset, u32 t_drawCount, u32 t_stride) noexcept
    {
        vkCmdDrawMeshTasksIndirectEXT(*m_currentHandle, t_buffer.getHandle(), t_offset, t_drawCount, t_stride);
    }

    void CommandBuffer::drawMeshTaskIndirectCount(Buffer& t_buffer, size_t t_offset, Buffer& t_countBuffer, size_t t_countBufferOffset, u32 t_maxDrawCount, u32 t_stride) noexcept
    {
        vkCmdDrawMeshTasksIndirectCountEXT(*m_currentHandle, t_buffer.getHandle(), t_offset, t_countBuffer.getHandle(), t_countBufferOffset, t_maxDrawCount, t_stride);
    }

    void CommandBuffer::setViewport(f32 t_x, f32 t_y, f32 t_width, f32 t_height) noexcept
    {
        VkViewport viewport;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        viewport.x = t_x;
        viewport.y = t_y;
        viewport.width = t_width;
        viewport.height = t_height;

        vkCmdSetViewport(*m_currentHandle, 0, 1, &viewport);
    }

    void CommandBuffer::setScissor(i32 t_x, i32 t_y, u32 t_width, u32 t_height) noexcept
    {
        VkRect2D scissor;
        scissor.offset = { t_x, t_y };
        scissor.extent = { t_width, t_height };

        vkCmdSetScissor(*m_currentHandle, 0, 1, &scissor);
    }

    void CommandBuffer::bindVertexBuffer(Buffer& t_buffer) noexcept
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(*m_currentHandle, 0, 1, &t_buffer.getHandle(), &offset);
    }

    void CommandBuffer::bindIndexBuffer16(Buffer& t_buffer) noexcept
    {
        vkCmdBindIndexBuffer(*m_currentHandle, t_buffer.getHandle(), 0, VK_INDEX_TYPE_UINT16);
    }

    void CommandBuffer::bindIndexBuffer32(Buffer& t_buffer) noexcept
    {
        vkCmdBindIndexBuffer(*m_currentHandle, t_buffer.getHandle() , 0, VK_INDEX_TYPE_UINT32);
    }

    void CommandBuffer::pushConstant(Pipeline& t_pipeline, ShaderStage t_stage, u32 t_size, void const* t_data) noexcept
    {
        vkCmdPushConstants(
            *m_currentHandle,
            t_pipeline.getLayout(),
            static_cast<VkShaderStageFlags>(t_stage),
            0,
            t_size,
            (void*)t_data
        );
    }

    void CommandBuffer::bindDescriptorGraphics(Pipeline& t_pipeline, Descriptor& t_descriptor, u32 t_firstSet) noexcept
    {
        vkCmdBindDescriptorSets(
            *m_currentHandle,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            t_pipeline.getLayout(),
            t_firstSet,
            1,
            &t_descriptor.getSet(),
            0,
            nullptr
        );
    }

    void CommandBuffer::bindDescriptorGraphics(Pipeline& t_pipeline, const std::vector<std::reference_wrapper<Descriptor>>& t_descriptors, u32 t_firstSet) noexcept
    {
        std::vector<VkDescriptorSet> sets(t_descriptors.size());
        for (size_t i = sets.size(); i--; )
        {
            sets[i] = t_descriptors[i].get().getSet();
        }

        vkCmdBindDescriptorSets(
            *m_currentHandle,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            t_pipeline.getLayout(),
            t_firstSet,
            static_cast<u32>(sets.size()),
            sets.data(),
            0,
            nullptr
        );
    }

    void CommandBuffer::bindDescriptorCompute(Pipeline& t_pipeline, Descriptor& t_descriptor, u32 t_firstSet) noexcept
    {
        vkCmdBindDescriptorSets(
            *m_currentHandle,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            t_pipeline.getLayout(),
            t_firstSet,
            1,
            &t_descriptor.getSet(),
            0,
            nullptr
        );
    }

    void CommandBuffer::bindDescriptorCompute(Pipeline& t_pipeline, const std::vector<std::reference_wrapper<Descriptor>>& t_descriptors, u32 t_firstSet) noexcept
    {
        std::vector<VkDescriptorSet> sets(t_descriptors.size());
        for (size_t i = sets.size(); i--; )
        {
            sets[i] = t_descriptors[i].get().getSet();
        }

        vkCmdBindDescriptorSets(
            *m_currentHandle,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            t_pipeline.getLayout(),
            t_firstSet,
            static_cast<u32>(sets.size()),
            sets.data(),
            0,
            nullptr
        );
    }

    void CommandBuffer::transitionImages(std::vector<ImageTransitionInfo> const& t_transitionInfos) noexcept
    {
        std::vector<VkImageMemoryBarrier2> barriers(t_transitionInfos.size());

        for (size_t i = barriers.size(); i--; )
        {
            barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barriers[i].pNext = nullptr;
            barriers[i].srcStageMask = t_transitionInfos[i].srcStageMask;
            barriers[i].dstStageMask = t_transitionInfos[i].dstStageMask;
            barriers[i].srcAccessMask = t_transitionInfos[i].srcAccessMask;
            barriers[i].dstAccessMask = t_transitionInfos[i].dstAccessMask;
            barriers[i].oldLayout = static_cast<VkImageLayout>(t_transitionInfos[i].oldLayout);
            barriers[i].newLayout = static_cast<VkImageLayout>(t_transitionInfos[i].newLayout);
            barriers[i].image = t_transitionInfos[i].image->getHandle();
            barriers[i].srcQueueFamilyIndex = 0;
            barriers[i].dstQueueFamilyIndex = 0;
            barriers[i].subresourceRange.baseMipLevel = 0;
            barriers[i].subresourceRange.levelCount = 1;
            barriers[i].subresourceRange.baseArrayLayer = 0;
            barriers[i].subresourceRange.layerCount = 1;

            switch (t_transitionInfos[i].newLayout)
            {
            case ImageLayout::eDepthStencilAttachment:
            case ImageLayout::eDepthAttachment:
            case ImageLayout::eDepthAttachmentStencilReadOnly:
            case ImageLayout::eDepthReadOnly:
            case ImageLayout::eDepthStencilReadOnly:
            case ImageLayout::eDepthReadOnlyStencilAttachment:
                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            case ImageLayout::eStencilAttachment:
            case ImageLayout::eStencilReadOnly:
                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
            default:
                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                break;
            }
        }

        VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dependencyInfo.imageMemoryBarrierCount = static_cast<u32>(barriers.size());
        dependencyInfo.pImageMemoryBarriers = barriers.data();

        vkCmdPipelineBarrier2(*m_currentHandle, &dependencyInfo);
    }

    void CommandBuffer::transitionImages(ImageTransitionInfo const& t_transitionInfo) noexcept
    {
        VkImageMemoryBarrier2 barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.srcStageMask = t_transitionInfo.srcStageMask;
        barrier.dstStageMask = t_transitionInfo.dstStageMask;
        barrier.srcAccessMask = t_transitionInfo.srcAccessMask;
        barrier.dstAccessMask = t_transitionInfo.dstAccessMask;
        barrier.oldLayout = static_cast<VkImageLayout>(t_transitionInfo.oldLayout);
        barrier.newLayout = static_cast<VkImageLayout>(t_transitionInfo.newLayout);
        barrier.image = t_transitionInfo.image->getHandle();
        barrier.srcQueueFamilyIndex = 0;
        barrier.dstQueueFamilyIndex = 0;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        switch (t_transitionInfo.newLayout)
        {
        case ImageLayout::eDepthStencilAttachment:
        case ImageLayout::eDepthAttachment:
        case ImageLayout::eDepthAttachmentStencilReadOnly:
        case ImageLayout::eDepthReadOnly:
        case ImageLayout::eDepthStencilReadOnly:
        case ImageLayout::eDepthReadOnlyStencilAttachment:
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case ImageLayout::eStencilAttachment:
        case ImageLayout::eStencilReadOnly:
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        default:
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        }

        VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(*m_currentHandle, &dependencyInfo);
    }

    void CommandBuffer::blitImage(Image& t_src, Image& t_dst, ImageBlit const& t_blit) noexcept
    {
        VkImageBlit blit;
        blit.srcOffsets[0] = { t_blit.srcOffset.x, t_blit.srcOffset.y, t_blit.srcOffset.z };
        blit.dstOffsets[0] = { t_blit.dstOffset.x, t_blit.dstOffset.y, t_blit.dstOffset.z };
        blit.srcOffsets[1] = { t_blit.srcSize.x, t_blit.srcSize.y, t_blit.srcSize.z };
        blit.dstOffsets[1] = { t_blit.dstSize.x, t_blit.dstSize.y, t_blit.dstSize.z };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = 0;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;

        vkCmdBlitImage(
            *m_currentHandle,
            t_src.getHandle(),
            static_cast<VkImageLayout>(t_blit.srcLayout),
            t_dst.getHandle(),
            static_cast<VkImageLayout>(t_blit.dstLayout),
            1,
            &blit,
            static_cast<VkFilter>(t_blit.filter)
        );
    }

    void CommandBuffer::copyImage(Image& t_src, Image& t_dst, ImageCopy const& t_copyInfo) noexcept
    {
        VkImageCopy copy;
        copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.srcSubresource.layerCount = 1;
        copy.srcSubresource.mipLevel = 0;
        copy.srcSubresource.baseArrayLayer = 0;
        copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.dstSubresource.layerCount = 1;
        copy.dstSubresource.mipLevel = 0;
        copy.dstSubresource.baseArrayLayer = 0;
        copy.dstOffset = { t_copyInfo.dstOffset.x, t_copyInfo.dstOffset.y, t_copyInfo.dstOffset.z };
        copy.srcOffset = { t_copyInfo.srcOffset.x, t_copyInfo.srcOffset.y, t_copyInfo.srcOffset.z };
        copy.extent    = { t_copyInfo.extent.x,    t_copyInfo.extent.y,    t_copyInfo.extent.z    };

        vkCmdCopyImage(
            *m_currentHandle,
            t_src.getHandle(),
            static_cast<VkImageLayout>(t_copyInfo.srcLayout),
            t_dst.getHandle(),
            static_cast<VkImageLayout>(t_copyInfo.dstLayout),
            1,
            &copy
        );
    }

    void CommandBuffer::copyBuffer(Buffer& t_src, Buffer& t_dst, size_t t_size, size_t t_dstOffset, size_t t_srcOffset) noexcept
    {
        VkBufferCopy copy;
        copy.size = t_size;
        copy.dstOffset = t_dstOffset;
        copy.srcOffset = t_srcOffset;

        vkCmdCopyBuffer(*m_currentHandle, t_src.getHandle(), t_dst.getHandle(), 1, &copy);
    }

    void CommandBuffer::copyBufferToImage(Buffer& t_src, Image& t_dst, BufferImageCopy const& t_copyInfo) noexcept
    {
        VkBufferImageCopy copy;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageSubresource.mipLevel = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.bufferOffset = t_copyInfo.bufferOffset;
        copy.bufferImageHeight = t_copyInfo.bufferImageHeight;
        copy.bufferRowLength = t_copyInfo.bufferRowLength;
        copy.imageOffset = { t_copyInfo.imageOffset.x, t_copyInfo.imageOffset.y, t_copyInfo.imageOffset.z };
        copy.imageExtent = { t_copyInfo.imageExtent.x, t_copyInfo.imageExtent.y, t_copyInfo.imageExtent.z };

        vkCmdCopyBufferToImage(*m_currentHandle, t_src.getHandle(), t_dst.getHandle(), static_cast<VkImageLayout>(t_copyInfo.imageLayout), 1, &copy);
    }

    void CommandBuffer::copyImageToBuffer(Image& t_src, Buffer& t_dst, BufferImageCopy const& t_copyInfo) noexcept
    {
        VkBufferImageCopy copy;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageSubresource.mipLevel = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.bufferOffset = t_copyInfo.bufferOffset;
        copy.bufferImageHeight = t_copyInfo.bufferImageHeight;
        copy.bufferRowLength = t_copyInfo.bufferRowLength;
        copy.imageOffset = { t_copyInfo.imageOffset.x, t_copyInfo.imageOffset.y, t_copyInfo.imageOffset.z };
        copy.imageExtent = { t_copyInfo.imageExtent.x, t_copyInfo.imageExtent.y, t_copyInfo.imageExtent.z };

        vkCmdCopyImageToBuffer(*m_currentHandle, t_src.getHandle(), static_cast<VkImageLayout>(t_copyInfo.imageLayout), t_dst.getHandle(), 1, &copy);
    }
}