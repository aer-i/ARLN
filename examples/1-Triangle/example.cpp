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

    auto commandBuffer = context.allocateCommandBuffer();

    auto pipeline = context.createGraphicsPipeline({
        .vertShaderPath = "shaders/main.vert.spv",
        .fragShaderPath = "shaders/main.frag.spv"
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
            {
                ColorAttachmentInfo colorAttachmentInfo;
                colorAttachmentInfo.clearColor = { 0.25f, 0.25f, 0.25f, 1.f };
                colorAttachmentInfo.image = context.getPresentImage();

                RenderingInfo renderingInfo;
                renderingInfo.pColorAttachment = &colorAttachmentInfo;

                commandBuffer.begin();
                commandBuffer.transitionImages({
                    ImageTransitionInfo{
                        context.getPresentImage(),
                        ImageLayout::eUndefined,
                        ImageLayout::eColorAttachment,
                        PipelineStageBits::eColorAttachmentOutput,
                        PipelineStageBits::eColorAttachmentOutput,
                        0,
                        AccessBits::eColorAttachmentWrite
                    }
                });
                commandBuffer.beginRendering(renderingInfo);

                commandBuffer.setViewport(0, f32(h), f32(w), -f32(h));
                commandBuffer.setScissor(0, 0, w, h);
                commandBuffer.bindGraphicsPipeline(pipeline);
                commandBuffer.draw(3);

                commandBuffer.endRendering();
                commandBuffer.transitionImages({
                    ImageTransitionInfo{
                        context.getPresentImage(),
                        ImageLayout::eColorAttachment,
                        ImageLayout::ePresentSrc,
                        PipelineStageBits::eColorAttachmentOutput,
                        PipelineStageBits::eColorAttachmentOutput,
                        AccessBits::eColorAttachmentWrite,
                        0
                    }
                });
                commandBuffer.end();
            }
            context.endFrame({ commandBuffer });
        }
    }

    pipeline.destroy();
}