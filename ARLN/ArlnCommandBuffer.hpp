#pragma once
#include "ArlnUtility.hpp"
#include "ArlnFrame.hpp"

namespace arln {

    class CommandBuffer
    {
    private:
        using CommandPoolArray   = std::array<VkCommandPool,   Frame::s_frameCount<u32>>;
        using CommandBufferArray = std::array<VkCommandBuffer, Frame::s_frameCount<u32>>;
        using HandlePointer      = VkCommandBuffer*;
        friend class Context;
        friend class Frame;
        explicit CommandBuffer(CommandPoolArray const& t_commandPools) noexcept;

    public:
        CommandBuffer() = default;
        CommandBuffer(CommandBuffer const&) = default;
        CommandBuffer(CommandBuffer&&) = default;
        CommandBuffer& operator=(CommandBuffer const&) = default;
        CommandBuffer& operator=(CommandBuffer&&) = default;
        ~CommandBuffer() = default;

        inline operator auto() const noexcept { return *m_currentHandle; }

        void begin() noexcept;
        void end() noexcept;
        void beginRendering(RenderingInfo const& t_renderingInfo) noexcept;
        void endRendering() noexcept;
        void bindGraphicsPipeline(Pipeline& t_pipeline) noexcept;
        void bindComputePipeline(Pipeline& t_pipeline) noexcept;
        void dispatch(u32 t_x, u32 t_y, u32 t_z) noexcept;
        void draw(u32 t_vertexCount, u32 t_instanceCount = 1, i32 t_firstVertex = 0, u32 t_firstInstance = 0) noexcept;
        void drawIndexed(u32 t_indexCount, u32 t_instanceCount = 1, u32 t_firstIndex = 0, i32 t_vertexOffset = 0, u32 t_firstInstance = 0) noexcept;
        void drawIndirect(Buffer& t_indirectBuffer, size_t t_offset, u32 t_drawCount, u32 t_stride) noexcept;
        void drawIndirectCount(Buffer& t_buffer, size_t t_offset, Buffer& t_countBuffer, size_t t_countBufferOffset, u32 t_maxDrawCount, u32 t_stride) noexcept;
        void drawIndexedIndirect(Buffer& t_indirectBuffer, size_t t_offset, u32 t_drawCount, u32 t_stride) noexcept;
        void drawIndexedIndirectCount(Buffer& t_buffer, size_t t_offset, Buffer& t_countBuffer, size_t t_countBufferOffset, u32 t_maxDrawCount, u32 t_stride) noexcept;
        void drawMeshTask(u32 t_x, u32 t_y, u32 t_z) noexcept;
        void drawMeshTaskIndirect(Buffer& t_indirectBuffer, size_t t_offset, u32 t_drawCount, u32 t_stride) noexcept;
        void drawMeshTaskIndirectCount(Buffer& t_buffer, size_t t_offset, Buffer& t_countBuffer, size_t t_countBufferOffset, u32 t_maxDrawCount, u32 t_stride) noexcept;
        void setViewport(f32 t_x, f32 t_y, f32 t_width, f32 t_height) noexcept;
        void setScissor(i32 t_x, i32 t_y, u32 t_width, u32 t_height) noexcept;
        void bindVertexBuffer(Buffer& t_buffer) noexcept;
        void bindIndexBuffer16(Buffer& t_buffer) noexcept;
        void bindIndexBuffer32(Buffer& t_buffer) noexcept;
        void pushConstant(Pipeline& t_pipeline, ShaderStage t_stage, u32 t_size, void const* t_data) noexcept;
        void bindDescriptorGraphics(Pipeline& t_pipeline, Descriptor& t_descriptor) noexcept;
        void bindDescriptorCompute(Pipeline& t_pipeline, Descriptor& t_descriptor) noexcept;
        void transitionImages(std::vector<ImageTransitionInfo> const& t_transitionInfos) noexcept;
        void transitionImages(ImageTransitionInfo const& t_transitionInfo) noexcept;
        void blitImage(Image& t_src, Image& t_dst, ImageBlit const& t_blit) noexcept;
        void copyImage(Image& t_src, Image& t_dst, ImageCopy const& t_copyInfo) noexcept;
        void copyBuffer(Buffer& t_src, Buffer& t_dst, size_t t_size, size_t t_dstOffset = 0, size_t t_srcOffset = 0) noexcept;
        void copyBufferToImage(Buffer& t_src, Image& t_dst, BufferImageCopy const& t_copyInfo) noexcept;
        void copyImageToBuffer(Image& t_src, Buffer& t_dst, BufferImageCopy const& t_copyInfo) noexcept;

    private:
        CommandBufferArray m_commandBuffers;
        HandlePointer      m_currentHandle;
    };

}