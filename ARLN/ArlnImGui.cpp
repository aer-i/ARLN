#include "ArlnImGui.hpp"
#include "ArlnContext.hpp"
#include "ArlnCommandBuffer.hpp"
#include "ArlnWindow.hpp"
#include <backends/imgui_impl_sdl3.h>

namespace arln {

    static struct ImGuiVulkanContext
    {
        DescriptorPool descriptorPool;
        Descriptor descriptor;
        Pipeline pipeline;
        Buffer vertexBuffer;
        Buffer indexBuffer;
        Sampler sampler;
        Image texture;
        bool isCreated{};
    } g_imguiVulkanContext;

    static void prepareResources() noexcept
    {
        auto& io = ImGui::GetIO();

        u8* fontData;
        i32 texWidth, texHeight;

        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        size_t uploadSize = texHeight * texWidth * 4 * sizeof(u8);

        g_imguiVulkanContext.texture = CurrentContext()->allocateImage(texWidth, texHeight, Format::eR8G8B8A8Unorm, ImageUsageBits::eSampled, arln::MemoryType::eGpuOnly);
        g_imguiVulkanContext.texture.transition(ImageLayout::eUndefined, ImageLayout::eTransferDst, PipelineStageBits::eTopOfPipe, PipelineStageBits::eTransfer, 0, AccessBits::eTransferWrite);
        g_imguiVulkanContext.texture.writeToImage(fontData, uploadSize, {texWidth, texHeight});
        g_imguiVulkanContext.texture.transition(ImageLayout::eTransferDst, ImageLayout::eShaderReadOnly, PipelineStageBits::eTransfer, PipelineStageBits::eFragmentShader, AccessBits::eTransferWrite, AccessBits::eShaderRead);

        g_imguiVulkanContext.sampler = CurrentContext()->createSampler(SamplerOptions{
            .magFilter = Filter::eNearest,
            .minFilter = Filter::eNearest,
            .mipmapMode = Filter::eNearest,
            .addressModeU = SamplerAddressMode::eClampToEdge,
            .addressModeV = SamplerAddressMode::eClampToEdge,
            .addressModeW = SamplerAddressMode::eClampToEdge
        });

        g_imguiVulkanContext.descriptorPool = CurrentContext()->createDescriptorPool();
        g_imguiVulkanContext.descriptor = ImguiContext::CreateImguiImage(g_imguiVulkanContext.texture);

        io.Fonts->SetTexID(&g_imguiVulkanContext.descriptor);

        GraphicsPipelineInfo pipelineInfo;
        pipelineInfo.vertShaderPath = "shaders/gui.vert.spv";
        pipelineInfo.fragShaderPath = "shaders/gui.frag.spv";
        pipelineInfo.colorBlending = true;
        pipelineInfo.cullMode = CullMode::eNone;
        pipelineInfo.lineWidth = 1.f;
        pipelineInfo.topology = Topology::eTriangleList;
        pipelineInfo.polygonMode = PolygonMode::eFill;
        pipelineInfo.descriptors << g_imguiVulkanContext.descriptor;
        pipelineInfo.pushConstants << PushConstantRange{ShaderStageBits::eVertex, sizeof(vec2), 0};
        pipelineInfo.bindings << BindingDescription{0, sizeof(ImDrawVert), VertexInputRate::eVertex};
        pipelineInfo.attributes << AttributeDescription{0, 0, Format::eR32G32Sfloat, static_cast<u32>(offsetof(ImDrawVert, pos))}
                                << AttributeDescription{1, 0, Format::eR32G32Sfloat, static_cast<u32>(offsetof(ImDrawVert, uv)) }
                                << AttributeDescription{2, 0, Format::eR8G8B8A8Unorm, static_cast<u32>(offsetof(ImDrawVert, col))};

        g_imguiVulkanContext.pipeline = CurrentContext()->createGraphicsPipeline(pipelineInfo);

        // TODO: Add support for multi viewport
        //ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
        //platformIo.
    }

    static void NewFrame() noexcept
    {
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    }

    void ImguiContext::Init(Window& t_window) noexcept
    {
        ImGui::CreateContext();

        auto& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiBackendFlags_PlatformHasViewports;

        ImGui_ImplSDL3_InitForVulkan(t_window.getHandle());
        prepareResources();

        NewFrame();
        g_imguiVulkanContext.isCreated = true;
    }

    void ImguiContext::Terminate() noexcept
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        g_imguiVulkanContext.descriptorPool.destroy();
        g_imguiVulkanContext.pipeline.destroy();
        g_imguiVulkanContext.sampler.destroy();
        g_imguiVulkanContext.texture.free();
        g_imguiVulkanContext.indexBuffer.free();
        g_imguiVulkanContext.vertexBuffer.free();
    }

    void ImguiContext::Render(arln::CommandBuffer& t_commandBuffer) noexcept
    {
        auto update = []
        {
            ImDrawData* imDrawData = ImGui::GetDrawData();

            size_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
            size_t indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

            if ((vertexBufferSize == 0) || (indexBufferSize == 0))
                return;

            if (vertexBufferSize > g_imguiVulkanContext.vertexBuffer.getSize())
            {
                g_imguiVulkanContext.vertexBuffer.recreate(
                    BufferUsageBits::eVertexBuffer,
                    MemoryType::eGpu,
                    vertexBufferSize
                );
            }

            if (indexBufferSize > g_imguiVulkanContext.indexBuffer.getSize())
            {
                g_imguiVulkanContext.indexBuffer.recreate(
                    BufferUsageBits::eIndexBuffer,
                    MemoryType::eGpu,
                    indexBufferSize
                );
            }

            u32 vtxOffset = 0;
            u32 idxOffset = 0;

            for (auto cmdList : imDrawData->CmdLists)
            {
                g_imguiVulkanContext.vertexBuffer.writeData(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), vtxOffset);
                g_imguiVulkanContext.indexBuffer.writeData(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), idxOffset);
                vtxOffset += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
                idxOffset += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
            }
        };

        auto render = [&t_commandBuffer]
        {
            ImDrawData* imDrawData = ImGui::GetDrawData();

            if ((imDrawData) && (imDrawData->CmdListsCount > 0))
            {
                ImGuiIO& io = ImGui::GetIO();
                vec2 scale = vec2{ 2.f / io.DisplaySize.x, 2.f / io.DisplaySize.y };

                t_commandBuffer.setViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                t_commandBuffer.bindGraphicsPipeline(g_imguiVulkanContext.pipeline);
                t_commandBuffer.bindDescriptorGraphics(g_imguiVulkanContext.pipeline, g_imguiVulkanContext.descriptor);
                t_commandBuffer.pushConstant(g_imguiVulkanContext.pipeline, ShaderStageBits::eVertex, sizeof(vec2), &scale);
                t_commandBuffer.bindVertexBuffer(g_imguiVulkanContext.vertexBuffer);
                t_commandBuffer.bindIndexBuffer16(g_imguiVulkanContext.indexBuffer);

                i32 vertexOffset = 0;
                u32 indexOffset = 0;
                Descriptor* previousDescriptor = nullptr;

                for (auto cmdList : imDrawData->CmdLists)
                {
                    for (auto& dCmd : cmdList->CmdBuffer)
                    {
                        auto currentDescriptor = (Descriptor*)dCmd.TextureId;

                        if (previousDescriptor != currentDescriptor)
                        {
                            previousDescriptor = currentDescriptor;
                            t_commandBuffer.bindDescriptorGraphics(g_imguiVulkanContext.pipeline, *currentDescriptor);
                        }

                        i32 offsetX = std::max((i32)(dCmd.ClipRect.x), 0);
                        i32 offsetY = std::max((i32)(dCmd.ClipRect.y), 0);
                        u32 width = static_cast<u32>((dCmd.ClipRect.z - dCmd.ClipRect.x));
                        u32 height = static_cast<u32>((dCmd.ClipRect.w - dCmd.ClipRect.y));
                        t_commandBuffer.setScissor(offsetX, offsetY, width, height);
                        t_commandBuffer.drawIndexed(dCmd.ElemCount, 1, indexOffset, vertexOffset);
                        indexOffset += dCmd.ElemCount;
                    }
                    vertexOffset += cmdList->VtxBuffer.Size;
                }
            }
        };

        ImGui::Render();

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        update();
        render();
        NewFrame();
    }

    auto ImguiContext::CreateImguiImage(Image& t_image) noexcept -> Descriptor
    {
        g_imguiVulkanContext.descriptorPool
            .addBinding(0, DescriptorType::eCombinedImageSampler, ShaderStageBits::eFragment);

        auto descriptor = g_imguiVulkanContext.descriptorPool.createDescriptor();

        DescriptorWriter()
            .addImage(descriptor, t_image, g_imguiVulkanContext.sampler, 0, DescriptorType::eCombinedImageSampler)
            .write();

        return descriptor;
    }

    void ImguiContext::RecreateImguiImage(Image& t_image, Descriptor t_descriptor) noexcept
    {
        DescriptorWriter()
            .addImage(t_descriptor, t_image, g_imguiVulkanContext.sampler, 0, DescriptorType::eCombinedImageSampler)
            .write();
    }

    auto ImguiContext::IsCreated() noexcept -> bool
    {
        return g_imguiVulkanContext.isCreated;
    }
}