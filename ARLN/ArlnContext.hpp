#pragma once
#include "ArlnUtility.hpp"
#include "ArlnSwapchain.hpp"
#include "ArlnFrame.hpp"
#include "ArlnPipeline.hpp"
#include "ArlnBuffer.hpp"
#include "ArlnImage.hpp"
#include "ArlnDescriptor.hpp"

namespace arln {

    class Context
    {
    public:
        Context() = delete;
        explicit Context(ContextCreateInfo const& t_createInfo) noexcept;
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept;
        static Context* s_currentContext;

        void beginFrame() noexcept;
        void endFrame(std::vector<CommandBufferHandle> const& t_commandBuffers) noexcept;
        auto canRender() noexcept -> bool;
        void setResizeCallback(std::function<void(u32, u32)> const& t_function) noexcept;
        void immediateSubmit(std::function<void(VkCommandBuffer)>&& t_function) noexcept;
        auto isPresentModeSupported(PresentMode t_presentMode) noexcept -> bool;
        auto allocateCommandBuffer() noexcept -> CommandBuffer;
        auto createGraphicsPipeline(GraphicsPipelineInfo const& t_pipelineInfo) noexcept -> Pipeline;
        auto createComputePipeline(ComputePipelineInfo const& t_pipelineInfo) noexcept -> Pipeline;
        auto allocateBuffer(BufferUsage t_bufferUsage, MemoryType t_memoryType, size_t t_sizeInBytes) noexcept -> Buffer;
        auto allocateImage(u32 t_width, u32 t_height, Format t_format, ImageUsage t_usage, MemoryType t_memoryType) noexcept -> Image;
        auto createDescriptorPool() noexcept -> DescriptorPool;
        auto createSampler(SamplerOptions const& t_options = {}) noexcept -> Sampler;
        auto findSupportedFormat(const std::vector<Format>& t_formats, ImageTiling t_tiling, FormatFeatures t_features) noexcept -> Format;

        inline auto  getAllocator()               const noexcept { return m_allocator;            }
        inline auto  getInstance()                const noexcept { return m_instance;             }
        inline auto  getSurface()                 const noexcept { return m_surface;              }
        inline auto  getDebugCallback()           const noexcept { return m_debugCallback;        }
        inline auto  getPhysicalDevice()          const noexcept { return m_physicalDevice;       }
        inline auto  getDevice()                  const noexcept { return m_device;               }
        inline auto  getGraphicsQueue()           const noexcept { return m_graphicsQueue;        }
        inline auto  getPresentQueue()            const noexcept { return m_presentQueue;         }
        inline auto  getSurfacePresentMode()      const noexcept { return m_surfacePresentMode;   }
        inline auto  getQueueIndex()              const noexcept { return m_queueFamilyIndex;     }
        inline auto  getDefaultColorFormat()      const noexcept { return m_colorFormat;          }
        inline auto  getDefaultDepthFormat()      const noexcept { return m_depthFormat;          }
        inline auto  getHeight()                  const noexcept { return m_getHeightFunc();      }
        inline auto  getWidth()                   const noexcept { return m_getWidthFunc();       }
        inline auto  isMeshShaderSupported()      const noexcept { return m_meshShaderSupported;  }
        inline auto& getSurfaceCapabilities()     const noexcept { return m_surfaceCapabilities;  }
        inline auto& getResizeCallback()          const noexcept { return m_resizeCallback;       }
        inline auto& getInfoCallback()            const noexcept { return m_infoCallback;         }
        inline auto& getErrorCallback()           const noexcept { return m_errorCallback;        }
        inline auto& getPresentImage()                  noexcept { return m_swapchain.getImage(); }
        inline auto& getSwapchain()                     noexcept { return m_swapchain;            }
        inline auto& getFrame()                         noexcept { return m_frame;                }

    private:
        void checkLayersSupport(std::span<const char*> t_layerNames) noexcept;
        void checkExtensionsSupport(std::span<const char*> t_extensionNames) noexcept;
        auto findQueueFamily(VkPhysicalDevice t_physicalDevice) noexcept -> u32;
        void selectPhysicalDevice() noexcept;
        void createLogicalDevice() noexcept;
        void createAllocator() noexcept;

    private:
        arln::Swapchain                       m_swapchain               { };
        arln::Frame                           m_frame                   { };
        VmaAllocator                          m_allocator               { };
        VkInstance                            m_instance                { };
        VkSurfaceKHR                          m_surface                 { };
        VkDebugReportCallbackEXT              m_debugCallback           { };
        VkPhysicalDevice                      m_physicalDevice          { };
        VkDevice                              m_device                  { };
        VkQueue                               m_graphicsQueue           { };
        VkQueue                               m_presentQueue            { };
        VkQueue                               m_computeQueue            { }; // TODO
        VkQueue                               m_transferQueue           { }; // TODO
        VkCommandPool                         m_immediateCommandPool    { };
        VkCommandBuffer                       m_immediateCommandBuffer  { };
        VkFence                               m_immediateFence          { };
        VkSurfaceCapabilitiesKHR              m_surfaceCapabilities     { };
        PresentMode                           m_surfacePresentMode      { };
        Format                                m_colorFormat             { };
        Format                                m_depthFormat             { };
        VkPhysicalDeviceProperties2           m_physicalDeviceProperties{ };
        VkPhysicalDeviceFeatures2             m_physicalDeviceFeatures  { };
        u32                                   m_queueFamilyIndex        { };
        std::vector<const char*>              m_deviceExtensions        { };
        std::function<u32()>                  m_getWidthFunc            { };
        std::function<u32()>                  m_getHeightFunc           { };
        std::function<void(u32, u32)>         m_resizeCallback          { };
        std::function<void(std::string_view)> m_infoCallback            { };
        std::function<void(std::string_view)> m_errorCallback           { };
        bool                                  m_meshShaderSupported     { };
    };

    inline void SetCurrentContext(Context& t_context) noexcept
    {
        Context::s_currentContext = std::addressof(t_context);
    }

    inline auto CurrentContext() noexcept -> Context*
    {
        return Context::s_currentContext;
    }
}