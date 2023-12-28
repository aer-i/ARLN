#pragma once
#include "ArlnMath.hpp"
#include "ArlnTypes.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <array>
#include <span>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <string_view>
#include <thread>

namespace arln {

    class Buffer;
    class CommandBuffer;
    class Context;
    class Descriptor;
    class DescriptorPool;
    class Frame;
    class Image;
    class ImguiContext;
    class Pipeline;
    class Sampler;
    class Swapchain;
    class Window;

    using CommandBufferHandle = VkCommandBuffer;
    using DrawIndirectCommand = VkDrawIndirectCommand;
    using DrawIndexedIndirectCommand = VkDrawIndexedIndirectCommand;
    using DrawMeshTasksIndirectCommand = VkDrawMeshTasksIndirectCommandEXT;
    using ShaderStage = u32;
    using BufferUsage = u32;
    using ImageUsage = u32;
    using PipelineStage = u64;
    using Access = u64;

    struct ImageUsageBits
    {
        ImageUsageBits() = delete;
        ~ImageUsageBits() = delete;

        enum Bits : ImageUsage
        {
            eSampled = 0x00000004,
            eStorage = 0x00000008,
            eColorAttachment = 0x00000010,
            eDepthStencilAttachment = 0x00000020,
            eTransientAttachment = 0x00000040,
            eInputAttachment = 0x00000080
        };
    };

    struct BufferUsageBits
    {
        BufferUsageBits() = delete;
        ~BufferUsageBits() = delete;

        enum Bits : BufferUsage
        {
            eUniformTexel = 0x00000004,
            eStorageTexel = 0x00000008,
            eUniformBuffer = 0x00000010,
            eStorageBuffer = 0x00000020,
            eIndexBuffer = 0x00000040,
            eVertexBuffer = 0x00000080,
            eIndirectBuffer = 0x00000100
        };
    };

    struct PipelineStageBits
    {
        PipelineStageBits() = delete;
        ~PipelineStageBits() = delete;

        enum Bits : PipelineStage
        {
            eNone = 0ULL,
            eTopOfPipe = 0x00000001ULL,
            eDrawIndirect = 0x00000002ULL,
            eVertexInput = 0x00000004ULL,
            eVertexShader = 0x00000008ULL,
            eTessellationControlShader = 0x00000010ULL,
            eTessellationEvaluationShader = 0x00000020ULL,
            eGeometryShader = 0x00000040ULL,
            eFragmentShader = 0x00000080ULL,
            eEarlyFragmentTests = 0x00000100ULL,
            eLateFragmentTests = 0x00000200ULL,
            eColorAttachmentOutput = 0x00000400ULL,
            eComputeShader = 0x00000800ULL,
            eAllTransfer = 0x00001000ULL,
            eTransfer = 0x00001000ULL,
            eBottomOfPipe = 0x00002000ULL,
            eHost = 0x00004000ULL,
            eAllGraphics = 0x00008000ULL,
            eAllCommands = 0x00010000ULL,
            eCopy = 0x100000000ULL,
            eResolve = 0x200000000ULL,
            eBlit = 0x400000000ULL,
            eClear = 0x800000000ULL,
            eIndexInput = 0x1000000000ULL,
            eVertexAttributeInput = 0x2000000000ULL,
            ePreRasterizationShaders = 0x4000000000ULL
        };
    };

    struct AccessBits
    {
        AccessBits() = delete;
        ~AccessBits() = delete;

        enum Bits : Access
        {
            eNone = 0ULL,
            eIndirectCommandRead = 0x00000001ULL,
            eIndexRead = 0x00000002ULL,
            eVertexAttributeRead = 0x00000004ULL,
            eUniformRead = 0x00000008ULL,
            eInputAttachmentRead = 0x00000010ULL,
            eShaderRead = 0x00000020ULL,
            eShaderWrite = 0x00000040ULL,
            eColorAttachmentRead = 0x00000080ULL,
            eColorAttachmentWrite = 0x00000100ULL,
            eDepthStencilAttachmentRead = 0x00000200ULL,
            eDepthStencilAttachmentWrite = 0x00000400ULL,
            eTransferRead = 0x00000800ULL,
            eTransferWrite = 0x00001000ULL,
            eHostRead = 0x00002000ULL,
            eHostWrite = 0x00004000ULL,
            eMemoryRead = 0x00008000ULL,
            eMemoryWrite = 0x00010000ULL,
            eShaderSamplerRead = 0x100000000ULL,
            eShaderStorageRead = 0x200000000ULL,
            eShaderStorageWrite = 0x400000000ULL,
        };
    };

    struct ShaderStageBits
    {
        ShaderStageBits() = delete;
        ~ShaderStageBits() = delete;

        enum Bits : ShaderStage
        {
            eVertex = 0x00000001,
            eTessellationControl = 0x00000002,
            eTessellationEvaluation = 0x00000004,
            eGeometry = 0x00000008,
            eFragment = 0x00000010,
            eCompute = 0x00000020,
            eAllGraphics = 0x0000001F,
            eAll = 0x7FFFFFFF,
        };
    };

    enum class ImageLayout : u32
    {
        eUndefined = 0,
        eGeneral = 1,
        eColorAttachment = 2,
        eDepthStencilAttachment = 3,
        eDepthStencilReadOnly = 4,
        eShaderReadOnly = 5,
        eTransferSrc = 6,
        eTransferDst = 7,
        ePreinitialized = 8,
        eDepthReadOnlyStencilAttachment = 1000117000,
        eDepthAttachmentStencilReadOnly = 1000117001,
        eDepthAttachment = 1000241000,
        eDepthReadOnly = 1000241001,
        eStencilAttachment = 1000241002,
        eStencilReadOnly = 1000241003,
        eReadOnly = 1000314000,
        eAttachment = 1000314001,
        ePresentSrc = 1000001002,
    };

    enum class MemoryType : u32
    {
        eGpu = 0,
        eGpuOnly = 1,
        eDedicated = 2,
        eCpu = 3
    };

    enum class DescriptorType : u32
    {
        eSampler = 0,
        eCombinedImageSampler = 1,
        eSampledImage = 2,
        eStorageImage = 3,
        eUniformTexelBuffer = 4,
        eStorageTexelBuffer = 5,
        eUniformBuffer = 6,
        eStorageBuffer = 7,
        eUniformBufferDynamic = 8,
        eStorageBufferDynamic = 9
    };

    enum class PresentMode : u32
    {
        eNoSync = 0,
        eTripleBuffering = 1,
        eVsync = 2,
        eVsyncRelaxed = 3
    };

    enum class Filter : u32
    {
        eNearest = 0,
        eLinear = 1
    };

    enum class SamplerAddressMode : u32
    {
        eRepeat = 0,
        eMirroredRepeat = 1,
        eClampToEdge = 2,
        eClampToBorder = 3,
        eMirrorClampToEdge = 4,
    };

    enum class FrontFace : u32
    {
        eCounterClockwise = 0x0,
        eClockwise = 0x1
    };

    enum class PolygonMode : u32
    {
        eFill  = 0x0,
        eLine  = 0x1,
        ePoint = 0x2
    };

    enum class CullMode : u32
    {
        eNone = 0x0,
        eFront = 0x1,
        eBack = 0x2,
        eFrontAndBack = 0x3,
    };

    enum class VertexInputRate : u32
    {
        eVertex = 0x0,
        eInstance = 0x1
    };

    enum class Topology : u32
    {
        ePointList = 0x0,
        eLineList = 0x1,
        eLineStrip = 0x2,
        eTriangleList = 0x3,
        eTriangleStrip = 0x4,
        eTriangleFan = 0x5
    };

    enum class Format : u32
    {
        eUndefined = 0,
        eR8Unorm = 9,
        eR8Snorm = 10,
        eR8Uscaled = 11,
        eR8Sscaled = 12,
        eR8Uint = 13,
        eR8Sint = 14,
        eR8Srgb = 15,
        eR8G8Unorm = 16,
        eR8G8Snorm = 17,
        eR8G8Uscaled = 18,
        eR8G8Sscaled = 19,
        eR8G8Uint = 20,
        eR8G8Sint = 21,
        eR8G8Srgb = 22,
        eR8G8B8Unorm = 23,
        eR8G8B8Snorm = 24,
        eR8G8B8Uscaled = 25,
        eR8G8B8Sscaled = 26,
        eR8G8B8Uint = 27,
        eR8G8B8Sint = 28,
        eR8G8B8Srgb = 29,
        eB8G8R8Unorm = 30,
        eB8G8R8Snorm = 31,
        eB8G8R8Uscaled = 32,
        eB8G8R8Sscaled = 33,
        eB8G8R8Uint = 34,
        eB8G8R8Sint = 35,
        eB8G8R8Srgb = 36,
        eR8G8B8A8Unorm = 37,
        eR8G8B8A8Snorm = 38,
        eR8G8B8A8Uscaled = 39,
        eR8G8B8A8Sscaled = 40,
        eR8G8B8A8Uint = 41,
        eR8G8B8A8Sint = 42,
        eR8G8B8A8Srgb = 43,
        eB8G8R8A8Unorm = 44,
        eB8G8R8A8Snorm = 45,
        eB8G8R8A8Uscaled = 46,
        eB8G8R8A8Sscaled = 47,
        eB8G8R8A8Uint = 48,
        eB8G8R8A8Sint = 49,
        eB8G8R8A8Srgb = 50,
        eR16Unorm = 70,
        eR16Snorm = 71,
        eR16Uscaled = 72,
        eR16Sscaled = 73,
        eR16Uint = 74,
        eR16Sint = 75,
        eR16Sfloat = 76,
        eR16G16Unorm = 77,
        eR16G16Snorm = 78,
        eR16G16Uscaled = 79,
        eR16G16Sscaled = 80,
        eR16G16Uint = 81,
        eR16G16Sint = 82,
        eR16G16Sfloat = 83,
        eR16G16B16Unorm = 84,
        eR16G16B16Snorm = 85,
        eR16G16B16Uscaled = 86,
        eR16G16B16Sscaled = 87,
        eR16G16B16Uint = 88,
        eR16G16B16Sint = 89,
        eR16G16B16Sfloat = 90,
        eR16G16B16A16Unorm = 91,
        eR16G16B16A16Snorm = 92,
        eR16G16B16A16Uscaled = 93,
        eR16G16B16A16Sscaled = 94,
        eR16G16B16A16Uint = 95,
        eR16G16B16A16Sint = 96,
        eR16G16B16A16Sfloat = 97,
        eR32Uint = 98,
        eR32Sint = 99,
        eR32Sfloat = 100,
        eR32G32Uint = 101,
        eR32G32Sint = 102,
        eR32G32Sfloat = 103,
        eR32G32B32Uint = 104,
        eR32G32B32Sint = 105,
        eR32G32B32Sfloat = 106,
        eR32G32B32A32Uint = 107,
        eR32G32B32A32Sint = 108,
        eR32G32B32A32Sfloat = 109,
        eR64Uint = 110,
        eR64Sint = 111,
        eR64Sfloat = 112,
        eR64G64Uint = 113,
        eR64G64Sint = 114,
        eR64G64Sfloat = 115,
        eR64G64B64Uint = 116,
        eR64G64B64Sint = 117,
        eR64G64B64Sfloat = 118,
        eR64G64B64A64Uint = 119,
        eR64G64B64A64Sint = 120,
        eR64G64B64A64Sfloat = 121,
        eD16Unorm = 124,
        eD32Sfloat = 126,
        eS8Uint = 127,
        eD16UnormS8Uint = 128,
        eD24UnormS8Uint = 129,
        eD32SfloatS8Uint = 130
    };

    struct BindingDescription
    {
        u32 binding;
        u32 stride;
        VertexInputRate vertexInputRate;
    };

    struct AttributeDescription
    {
        u32 location;
        u32 binding;
        Format format;
        u32 offset;
    };

    struct PushConstantRange
    {
        ShaderStage stage;
        u32 size;
        u32 offset;
    };

    struct BufferImageCopy
    {
        size_t bufferOffset;
        u32    bufferRowLength;
        u32    bufferImageHeight;
        ivec3  imageOffset;
        uvec3  imageExtent;
        ImageLayout imageLayout;
    };

    struct ImageCopy
    {
        ImageLayout srcLayout;
        ImageLayout dstLayout;
        ivec3       srcOffset;
        ivec3       dstOffset;
        uvec3       extent;
    };

    struct ImageBlit
    {
        ImageLayout srcLayout;
        ImageLayout dstLayout;
        ivec3       srcSize;
        ivec3       dstSize;
        ivec3       srcOffset;
        ivec3       dstOffset;
        Filter      filter;
    };

    struct GraphicsPipelineInfo
    {
        struct Bindings
        {
            std::vector<BindingDescription> bindingDescriptions;
            inline Bindings& operator<<(BindingDescription&& t_bindingDescription)
            {
                bindingDescriptions.emplace_back(t_bindingDescription);
                return *this;
            }
        } bindings;

        struct Attributes
        {
            std::vector<AttributeDescription> attributeDescriptions;
            inline Attributes& operator<<(AttributeDescription&& t_attributeDescription)
            {
                attributeDescriptions.emplace_back(t_attributeDescription);
                return *this;
            }
        } attributes;

        struct PushConstants
        {
            std::vector<PushConstantRange> pushConstantRanges;
            inline PushConstants& operator<<(PushConstantRange&& t_pushConstantRanges)
            {
                pushConstantRanges.emplace_back(t_pushConstantRanges);
                return *this;
            }
        } pushConstants;

        struct Descriptors
        {
            std::vector<Descriptor*> layouts;
            inline Descriptors& operator<<(Descriptor& t_descriptor)
            {
                layouts.emplace_back(std::addressof(t_descriptor));
                return *this;
            }
        } descriptors;

        std::string_view vertShaderPath;
        std::string_view fragShaderPath;
        std::string_view meshShaderPath;
        std::string_view taskShaderPath;
        std::vector<Format> colorFormats;
        Format depthFormat = Format::eD16Unorm;
        Format stencilFormat = Format::eUndefined;
        PolygonMode polygonMode = PolygonMode::eFill;
        FrontFace frontFace = FrontFace::eCounterClockwise;
        CullMode cullMode = CullMode::eNone;
        Topology topology = Topology::eTriangleList;
        f32 lineWidth = 1.0f;
        bool depthStencil = false;
        bool colorBlending = false;
    };

    struct ComputePipelineInfo
    {
        struct PushConstants
        {
            std::vector<PushConstantRange> pushConstantRanges;
            inline PushConstants& operator<<(PushConstantRange&& t_pushConstantRanges)
            {
                pushConstantRanges.emplace_back(t_pushConstantRanges);
                return *this;
            }
        } pushConstants;

        struct Descriptors
        {
            std::vector<Descriptor*> layouts;
            inline Descriptors& operator<<(Descriptor& t_descriptor)
            {
                layouts.emplace_back(std::addressof(t_descriptor));
                return *this;
            }
        } descriptors;

        std::string_view compShaderPath;
    };

    struct SamplerOptions
    {
        Filter magFilter = Filter::eLinear;
        Filter minFilter = Filter::eLinear;
        Filter mipmapMode = Filter::eNearest;
        SamplerAddressMode addressModeU = SamplerAddressMode::eRepeat;
        SamplerAddressMode addressModeV = SamplerAddressMode::eRepeat;
        SamplerAddressMode addressModeW = SamplerAddressMode::eRepeat;
        f32 minLod = 0.f;
        f32 maxLod = 0.f;
        bool unnormalizedCoordinates{ };
    };

    struct ContextCreateInfo
    {
        std::function<void(std::string_view)> errorCallback = [](std::string_view){};
        std::function<void(std::string_view)> infoCallback  = [](std::string_view){};
        std::function<u32()> getWindowWidthFunc;
        std::function<u32()> getWindowHeightFunc;
        std::function<VkSurfaceKHR(VkInstance)> surfaceCreation;
        std::vector<const char*> extensions;
        std::vector<const char*> layers;
        std::vector<const char*> deviceExtensions;
        std::string applicationName = "ARLN Application";
        std::string engineName = "ARLN";
        PresentMode presentMode = PresentMode::eNoSync;
    };

    struct ImageTransitionInfo
    {
        Image* image;
        ImageLayout oldLayout;
        ImageLayout newLayout;
        PipelineStage srcStageMask;
        PipelineStage dstStageMask;
        Access srcAccessMask;
        Access dstAccessMask;
    };

    struct ColorAttachmentInfo
    {
        std::array<f32, 4> clearColor{ 0.f, 0.f, 0.f, 1.f };
        Image* image = nullptr;
        bool late{ };
    };

    struct DepthAttachmentInfo
    {
        Image* image;
        u32 stencil = 0;
        f32 depth = 1.f;
        bool late{ };
    };

    struct RenderingInfo
    {
        ColorAttachmentInfo* pColorAttachment = nullptr;
        DepthAttachmentInfo* pDepthAttachment = nullptr;
        ivec2 size{ 0, 0 };
        ivec2 offset{ 0, 0 };
    };
}