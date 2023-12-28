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

    if (!context.isMeshShaderSupported())
    {
        errorCallback("Selected graphics card does not support mesh shader extension");
    }

    auto commandBuffer = context.allocateCommandBuffer();

    auto pipeline = context.createGraphicsPipeline(GraphicsPipelineInfo{
        .fragShaderPath = "shaders/main.frag.spv",
        .meshShaderPath = "shaders/main.mesh.spv",
        .taskShaderPath = "shaders/main.task.spv"
    });

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

        auto w = window.getWidth();
        auto h = window.getHeight();

        if (context.canRender())
        {
            context.beginFrame();
            commandBuffer.begin();
            {
                commandBuffer.transitionImages(ImageTransitionInfo{
                    .image = context.getPresentImage(),
                    .oldLayout = ImageLayout::eUndefined,
                    .newLayout = ImageLayout::eColorAttachment,
                    .srcStageMask = PipelineStageBits::eTopOfPipe,
                    .dstStageMask = PipelineStageBits::eColorAttachmentOutput,
                    .srcAccessMask = 0,
                    .dstAccessMask = AccessBits::eColorAttachmentWrite
                });

                ColorAttachmentInfo colorAttachmentInfo = {
                    .image = context.getPresentImage()
                };

                commandBuffer.beginRendering(RenderingInfo{
                    .pColorAttachment = &colorAttachmentInfo
                });

                commandBuffer.bindGraphicsPipeline(pipeline);
                commandBuffer.setViewport(0, f32(h), f32(w), -f32(h));
                commandBuffer.setScissor(0, 0, w, h);
                commandBuffer.drawMeshTask(1, 1, 1);

                commandBuffer.endRendering();
                commandBuffer.transitionImages(ImageTransitionInfo{
                    .image = context.getPresentImage(),
                    .oldLayout = ImageLayout::eColorAttachment,
                    .newLayout = ImageLayout::ePresentSrc,
                    .srcStageMask = PipelineStageBits::eColorAttachmentOutput,
                    .dstStageMask = PipelineStageBits::eBottomOfPipe,
                    .srcAccessMask = AccessBits::eColorAttachmentWrite,
                    .dstAccessMask = 0
                });
            }
            commandBuffer.end();
            context.endFrame({ commandBuffer });
        }
    }

    pipeline.destroy();
}