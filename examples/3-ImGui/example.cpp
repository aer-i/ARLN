#include <Arln.hpp>
#include <iostream>

auto main() -> int
{
    using namespace arln;

    Window window = Window({});

    auto errorCallback   = [ ](std::string_view t_error) { std::cerr << "[ERROR]\t" << t_error << std::endl; SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", t_error.data(), nullptr); std::exit(1); };
    auto infoCallback    = [ ](std::string_view t_info) { std::cout << "[INFO]\t" << t_info << std::endl; };
    auto surfaceCreation = [&](VkInstance t_instance) { return window.createSurface(t_instance); };

    Context context = Context({
        .errorCallback = errorCallback,
        .infoCallback = infoCallback,
        .getWindowWidthFunc = [&]{ return window.getWidth(); },
        .getWindowHeightFunc = [&]{ return window.getHeight(); },
        .surfaceCreation = surfaceCreation,
        .extensions = window.getInstanceExtensions(),
#ifndef NDEBUG
        .layers = { "VK_LAYER_KHRONOS_validation" }
#endif
    });
    ImguiContext::Init(window);
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().LogFilename = nullptr;

    auto pipeline = context.createGraphicsPipeline(GraphicsPipelineInfo{
        .vertShaderPath = "shaders/colorAttachment.vert.spv",
        .fragShaderPath = "shaders/colorAttachment.frag.spv"
    });

    auto commandBuffer = context.allocateCommandBuffer();
    auto imguiCommandBuffer = context.allocateCommandBuffer();
    auto renderAttachment = context.allocateImage(1, 1, Format::eR8G8B8A8Unorm, ImageUsageBits::eColorAttachment | ImageUsageBits::eSampled, MemoryType::eCpu);
    auto imguiDescriptor = ImguiContext::CreateImguiImage(renderAttachment);

    ImVec2 currentSize = { 1, 1 };
    std::array<f32, 3> clearColors{ 0.5f, 0.25f, 0.75f };

    while (!window.shouldClose())
    {
        window.pollEvents();

        static u32 fps = 0;
        u32 currentFps = window.getFps();
        if (fps != currentFps)
        {
            fps = currentFps;
            window.setTitle(std::to_string(fps));
        }

        if (context.canRender())
        {
            if (ImGui::Begin("Test 1"))
            {
                ImGui::TextColored({1.f, 1.f, 0.f, 1.f}, "Hello world!");
                ImGui::TextColored({1.f, 1.f, 0.f, 1.f}, "Hello world!");
                ImGui::TextColored({1.f, 1.f, 0.f, 1.f}, "Hello world!");
            }
            ImGui::End();

            if (ImGui::Begin("Test 2"))
            {
                ImGui::PushItemWidth(200);
                ImGui::ColorPicker3("Color picker", clearColors.data(), ImGuiColorEditFlags_PickerHueWheel);
            }
            ImGui::End();

            ImGui::ShowDemoWindow();

            ImGui::Begin("Viewport");
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();

            if ((viewportSize.x != currentSize.x || viewportSize.y != currentSize.y) && (viewportSize.x > 0 && viewportSize.y > 0))
            {
                currentSize = viewportSize;
                renderAttachment.recreate((u32)viewportSize.x, (u32)viewportSize.y, Format::eR8G8B8A8Unorm, ImageUsageBits::eColorAttachment | ImageUsageBits::eSampled, MemoryType::eGpu);
                ImguiContext::RecreateImguiImage(renderAttachment, imguiDescriptor);
            }

            ImGui::Image(&imguiDescriptor, viewportSize);
            ImGui::End();

            context.beginFrame();
            {
                {
                    commandBuffer.begin();
                    ColorAttachmentInfo colorAttachment;
                    colorAttachment.image = renderAttachment;
                    colorAttachment.clearColor = {clearColors[0], clearColors[1], clearColors[2], 1.f};

                    RenderingInfo renderingInfo;
                    renderingInfo.pColorAttachment = &colorAttachment;
                    renderingInfo.size = {currentSize.x, currentSize.y};

                    commandBuffer.transitionImages(ImageTransitionInfo{
                        renderAttachment,
                        ImageLayout::eUndefined,
                        ImageLayout::eColorAttachment,
                        PipelineStageBits::eFragmentShader,
                        PipelineStageBits::eColorAttachmentOutput,
                        AccessBits::eShaderRead,
                        AccessBits::eColorAttachmentWrite
                    });
                    commandBuffer.beginRendering(renderingInfo);

                    commandBuffer.setScissor(0, 0, static_cast<u32>(currentSize.x), static_cast<u32>(currentSize.y));
                    commandBuffer.setViewport(0, currentSize.y, currentSize.x, -currentSize.y);
                    commandBuffer.bindGraphicsPipeline(pipeline);
                    commandBuffer.draw(3);

                    commandBuffer.endRendering();
                    commandBuffer.transitionImages(ImageTransitionInfo{
                        renderAttachment,
                        ImageLayout::eColorAttachment,
                        ImageLayout::eShaderReadOnly,
                        PipelineStageBits::eColorAttachmentOutput,
                        PipelineStageBits::eFragmentShader,
                        AccessBits::eColorAttachmentWrite,
                        AccessBits::eShaderRead
                    });
                    commandBuffer.end();
                }
                {
                    ColorAttachmentInfo imguiColorAttachment;
                    imguiColorAttachment.image = context.getPresentImage();
                    imguiColorAttachment.clearColor = {0.f, 0.f, 0.f, 1.f};

                    RenderingInfo imguiRenderingInfo;
                    imguiRenderingInfo.pColorAttachment = &imguiColorAttachment;

                    imguiCommandBuffer.begin();
                    imguiCommandBuffer.transitionImages(ImageTransitionInfo{
                        .image = context.getPresentImage(),
                        .oldLayout = ImageLayout::eUndefined,
                        .newLayout = ImageLayout::eColorAttachment,
                        .srcStageMask = PipelineStageBits::eTopOfPipe,
                        .dstStageMask = PipelineStageBits::eColorAttachmentOutput,
                        .srcAccessMask = AccessBits::eNone,
                        .dstAccessMask = AccessBits::eColorAttachmentWrite
                    });
                    imguiCommandBuffer.beginRendering(imguiRenderingInfo);
                    ImguiContext::Render(imguiCommandBuffer);
                    imguiCommandBuffer.endRendering();
                    imguiCommandBuffer.transitionImages(ImageTransitionInfo{
                        .image = context.getPresentImage(),
                        .oldLayout = ImageLayout::eColorAttachment,
                        .newLayout = ImageLayout::ePresentSrc,
                        .srcStageMask = PipelineStageBits::eColorAttachmentOutput,
                        .dstStageMask = PipelineStageBits::eTopOfPipe,
                        .srcAccessMask = AccessBits::eColorAttachmentWrite,
                        .dstAccessMask = AccessBits::eNone
                    });
                    imguiCommandBuffer.end();
                }
            }
            context.endFrame({ commandBuffer, imguiCommandBuffer });
        }
    }

    pipeline.destroy();
    renderAttachment.free();
    ImguiContext::Terminate();
}