#include <Arln.hpp>
#include <iostream>

struct Vertex
{
    arln::vec3 pos;
    alignas(16) arln::vec3 color;
};

struct UBO
{
    arln::mat4 proj, view, model;
} ubo;

Vertex vertices[] = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}}
};

arln::u32 indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

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

    Image storageImage;
    auto commandBuffer = context.allocateCommandBuffer();
    auto descriptorPool = context.createDescriptorPool();

    descriptorPool.addBinding(0, DescriptorType::eStorageImage, ShaderStageBits::eCompute);
    auto descriptor = descriptorPool.createDescriptor();

    ComputePipelineInfo pipelineInfo;
    pipelineInfo.compShaderPath = "shaders/main.comp.spv";
    pipelineInfo.descriptors << descriptor;
    auto computePipeline = context.createComputePipeline(pipelineInfo);

    auto onResize = [&](u32 t_width, u32 t_height)
    {
        storageImage.recreate(t_width, t_height, Format::eR16G16B16A16Sfloat, ImageUsageBits::eSampled | ImageUsageBits::eStorage, MemoryType::eDedicated);
        DescriptorWriter()
            .addImage(descriptor, storageImage, nullptr, 0, DescriptorType::eStorageImage)
            .write();
    };

    onResize(context.getCurrentExtent().x, context.getCurrentExtent().y);
    context.setResizeCallback(onResize);

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
                    .image = storageImage,
                    .oldLayout = ImageLayout::eUndefined,
                    .newLayout = ImageLayout::eGeneral,
                    .srcStageMask = PipelineStageBits::eTransfer,
                    .dstStageMask = PipelineStageBits::eComputeShader,
                    .srcAccessMask = AccessBits::eTransferRead,
                    .dstAccessMask = AccessBits::eShaderStorageRead | AccessBits::eShaderStorageWrite
                });

                commandBuffer.bindComputePipeline(computePipeline);
                commandBuffer.bindDescriptorCompute(computePipeline, descriptor);
                commandBuffer.dispatch(static_cast<u32>(std::ceil(f32(context.getCurrentExtent().x) / 16.f)),
                                       static_cast<u32>(std::ceil(f32(context.getCurrentExtent().y) / 16.f)), 1);

                commandBuffer.transitionImages({
                    ImageTransitionInfo{
                        .image = storageImage,
                        .oldLayout = ImageLayout::eGeneral,
                        .newLayout = ImageLayout::eTransferSrc,
                        .srcStageMask = PipelineStageBits::eComputeShader,
                        .dstStageMask = PipelineStageBits::eTransfer,
                        .srcAccessMask = AccessBits::eShaderStorageRead | AccessBits::eShaderStorageWrite,
                        .dstAccessMask = AccessBits::eTransferRead
                    },
                    ImageTransitionInfo{
                        .image = context.getPresentImage(),
                        .oldLayout = ImageLayout::eUndefined,
                        .newLayout = ImageLayout::eTransferDst,
                        .srcStageMask = PipelineStageBits::eTopOfPipe,
                        .dstStageMask = PipelineStageBits::eTransfer,
                        .srcAccessMask = AccessBits::eNone,
                        .dstAccessMask = AccessBits::eTransferWrite
                    }
                });

                commandBuffer.blitImage(storageImage, context.getPresentImage(), ImageBlit{
                    .srcLayout = ImageLayout::eTransferSrc,
                    .dstLayout = ImageLayout::eTransferDst,
                    .srcSize = { w, h, 1 },
                    .dstSize = { w, h, 1 },
                    .filter = Filter::eLinear
                });

                commandBuffer.transitionImages(ImageTransitionInfo{
                    .image = context.getPresentImage(),
                    .oldLayout = ImageLayout::eTransferDst,
                    .newLayout = ImageLayout::ePresentSrc,
                    .srcStageMask = PipelineStageBits::eTransfer,
                    .dstStageMask = PipelineStageBits::eBottomOfPipe,
                    .srcAccessMask = AccessBits::eTransferWrite,
                    .dstAccessMask = AccessBits::eNone
                });
            }
            commandBuffer.end();
            context.endFrame({ commandBuffer });
        }
    }

    computePipeline.destroy();
    descriptorPool.destroy();
    storageImage.free();
}