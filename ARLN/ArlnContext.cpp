#define VMA_IMPLEMENTATION
#include "ArlnContext.hpp"
#include "ArlnTypes.hpp"
#include "ArlnCommandBuffer.hpp"
#include <algorithm>
#include <cstring>
#include <algorithm>

static VkBool32 VKAPI_CALL debugReportCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData)
{
    std::printf("\x1B[31m[VALIDATION LAYER]\t%s\033[0m\n", pMessage);

    return VK_FALSE;
}

namespace arln {

    Context* Context::s_currentContext = nullptr;

    void Context::beginFrame() noexcept
    {
        m_frame.beginFrame();
    }

    void Context::endFrame(std::vector<CommandBufferHandle> const& t_commandBuffers) noexcept
    {
        m_frame.endFrame(t_commandBuffers);
    }

    auto Context::canRender() noexcept -> bool
    {
        if (getWindowWidth() == 0 || getWindowHeight() == 0)
        {
            return false;
        }

        return true;
    }

    void Context::setResizeCallback(std::function<void(u32, u32)> const& t_function) noexcept
    {
        m_resizeCallback = t_function;
    }

    void Context::immediateSubmit(std::function<void(VkCommandBuffer)>&& t_function) noexcept
    {
        vkResetFences(m_device, 1, &m_immediateFence);
        vkResetCommandPool(m_device, m_immediateCommandPool, 0);

        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        beginInfo.pNext = nullptr;
        beginInfo.flags = 0;

        vkBeginCommandBuffer(m_immediateCommandBuffer, &beginInfo);
        {
            t_function(m_immediateCommandBuffer);
        }
        vkEndCommandBuffer(m_immediateCommandBuffer);

        VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_immediateCommandBuffer;

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_immediateFence);
        vkWaitForFences(m_device, 1, &m_immediateFence, false, UINT64_MAX);
    }

    auto Context::isPresentModeSupported(PresentMode t_presentMode) noexcept -> bool
    {
        u32 modeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &modeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &modeCount, presentModes.data());

        return std::ranges::any_of(
            presentModes,
            [&](auto const& t_p){ return t_p == static_cast<VkPresentModeKHR>(t_presentMode); }
        );
    }

    auto Context::allocateCommandBuffer() noexcept -> CommandBuffer
    {
        return m_frame.allocateCommandBuffers();
    }

    auto Context::createGraphicsPipeline(GraphicsPipelineInfo const& t_pipelineInfo) noexcept -> Pipeline
    {
        return Pipeline(t_pipelineInfo);
    }

    auto Context::createComputePipeline(ComputePipelineInfo const& t_pipelineInfo) noexcept -> Pipeline
    {
        return Pipeline(t_pipelineInfo);
    }

    auto Context::allocateBuffer(BufferUsage t_bufferUsage, MemoryType t_memoryType, size_t t_sizeInBytes) noexcept -> Buffer
    {
        return { t_bufferUsage, t_memoryType, t_sizeInBytes };
    }

    auto Context::allocateImage(u32 t_width, u32 t_height, Format t_format, ImageUsage t_usage, MemoryType t_memoryType) noexcept -> Image
    {
        return { t_width, t_height, t_format, t_usage, t_memoryType };
    }

    auto Context::createDescriptorPool() noexcept -> DescriptorPool
    {
        return { {} };
    }

    auto Context::createSampler(SamplerOptions const& t_options) noexcept -> Sampler
    {
        return Sampler{ t_options };
    }

    Context::Context(ContextCreateInfo const& t_createInfo) noexcept
        : m_surfacePresentMode{ t_createInfo.presentMode }
        , m_deviceExtensions{ t_createInfo.deviceExtensions }
        , m_getWidthFunc{ t_createInfo.getWindowWidthFunc }
        , m_getHeightFunc{ t_createInfo.getWindowHeightFunc }
        , m_resizeCallback{ [](i32, i32){} }
        , m_infoCallback{ t_createInfo.infoCallback }
        , m_errorCallback{ t_createInfo.errorCallback }
    {
        SetCurrentContext(*this);

        if (volkInitialize() != VK_SUCCESS)
        {
            m_errorCallback("Vulkan functions loading failed");
        }

        if (volkGetInstanceVersion() < VK_API_VERSION_1_3)
        {
            m_errorCallback("Vulkan 1.3 not supported");
        }

        std::vector<const char*> layers     = t_createInfo.layers;
        std::vector<const char*> extensions = t_createInfo.extensions;

        bool useValidation = std::find_if(layers.begin(), layers.end(),
        [](const char* layerName)
        {
            return std::strcmp(layerName, "VK_LAYER_KHRONOS_validation") == 0;
        }) != layers.end();

        VkApplicationInfo applicationInfo;
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pNext = nullptr;
        applicationInfo.apiVersion = VK_API_VERSION_1_3;
        applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        applicationInfo.pEngineName = t_createInfo.engineName.c_str();
        applicationInfo.pApplicationName = t_createInfo.applicationName.c_str();

        if (useValidation)
        {
            extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        VkValidationFeatureEnableEXT enabledValidationFeatures[] =
        {
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };

        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = sizeof(enabledValidationFeatures) / sizeof(enabledValidationFeatures[0]);
        validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

        VkInstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.flags = 0;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = static_cast<u32>(layers.size());
        instanceCreateInfo.ppEnabledLayerNames = layers.data();
        instanceCreateInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        if (useValidation)
        {
            instanceCreateInfo.pNext = &validationFeatures;
        }

        checkLayersSupport(layers);
        checkExtensionsSupport(extensions);

        if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            m_errorCallback("Failed to create vulkan instance");
        }
        volkLoadInstance(m_instance);

        if (useValidation)
        {
            VkDebugReportCallbackCreateInfoEXT reportCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
            reportCallbackInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
            reportCallbackInfo.pfnCallback = debugReportCallback;

            if (vkCreateDebugReportCallbackEXT(m_instance, &reportCallbackInfo, nullptr, &m_debugCallback) != VK_SUCCESS)
            {
                m_errorCallback("Failed to create vulkan debug messenger");
            }

            m_infoCallback("Validation Layers are enabled");
        }

        m_infoCallback("Created vulkan instance");

        m_surface = t_createInfo.surfaceCreation(m_instance);

        if (!m_surface)
        {
            m_errorCallback("Failed to create vulkan surface");
        }

        this->selectPhysicalDevice();

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &m_surfaceCapabilities);

        m_infoCallback(std::string("Selected queue family index: " + std::to_string(m_queueFamilyIndex)));
        m_infoCallback(std::string("Selected physical device: ") + m_physicalDeviceProperties.properties.deviceName);
        m_infoCallback(std::string("API Version: ")
            + std::to_string(m_physicalDeviceProperties.properties.apiVersion >> 22u) + std::string(".")
            + std::to_string((m_physicalDeviceProperties.properties.apiVersion >> 12u) & 0x3FFU) + std::string(".")
            + std::to_string(m_physicalDeviceProperties.properties.apiVersion & 0xFFFU)
        );
        m_infoCallback(std::string("Driver Version: ")
            + std::to_string(m_physicalDeviceProperties.properties.driverVersion >> 22u) + std::string(".")
            + std::to_string((m_physicalDeviceProperties.properties.driverVersion >> 12u) & 0x3FFU) + std::string(".")
            + std::to_string(m_physicalDeviceProperties.properties.driverVersion & 0xFFFU)
        );

        this->createLogicalDevice();
        this->createAllocator();

        vkGetDeviceQueue(m_device, m_queueFamilyIndex, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndex, 0, &m_presentQueue);

        VkCommandPoolCreateInfo commandPoolCreateInfo;
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = nullptr;
        commandPoolCreateInfo.flags = 0;
        commandPoolCreateInfo.queueFamilyIndex = m_queueFamilyIndex;

        if (vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_immediateCommandPool) != VK_SUCCESS)
        {
            m_errorCallback("Failed to create vulkan command pool");
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = nullptr;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = m_immediateCommandPool;

        if (vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &m_immediateCommandBuffer) != VK_SUCCESS)
        {
            m_errorCallback("Failed to create vulkan command buffer");
        }

        VkFenceCreateInfo fenceCreateInfo;
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = nullptr;
        fenceCreateInfo.flags = 0;

        if (vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immediateFence) != VK_SUCCESS)
        {
            m_errorCallback("Failed to create vulkan fence");
        }

        auto surfaceFormat = [&]()
        {
            uint32_t count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(CurrentContext()->getPhysicalDevice(), CurrentContext()->getSurface(), &count, nullptr);
            std::vector<VkSurfaceFormatKHR> availableFormats(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(CurrentContext()->getPhysicalDevice(), CurrentContext()->getSurface(), &count, availableFormats.data());

            if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
            {
                m_colorFormat = Format::eR8G8B8A8Unorm;
                return VkSurfaceFormatKHR{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            }
            for (const auto& currentFormat : availableFormats)
            {
                if (currentFormat.format == VK_FORMAT_R8G8B8A8_UNORM && currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    m_colorFormat = Format::eR8G8B8A8Unorm;
                    return currentFormat;
                }
            }
            for (const auto& currentFormat : availableFormats)
            {
                if (currentFormat.format == VK_FORMAT_B8G8R8A8_UNORM && currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    m_colorFormat = Format::eB8G8R8A8Unorm;
                    return currentFormat;
                }
            }
            return availableFormats[0];
        }();

        m_colorFormat = static_cast<Format>(surfaceFormat.format);
        m_depthFormat = findSupportedFormat({
            Format::eD16Unorm, Format::eD32Sfloat, Format::eD32SfloatS8Uint, Format::eD24UnormS8Uint
            },
            ImageTiling::eOptimal,
            FormatFeaturesBits::eDepthStencilAttachment
        );
        m_swapchain.create();
        m_frame.create();

        m_infoCallback("Created vulkan context");
    }

    Context::~Context() noexcept
    {
        vkDeviceWaitIdle(m_device);

        m_swapchain.teardown();
        m_frame.teardown();

        if (m_allocator)               vmaDestroyAllocator(m_allocator);
        if (m_immediateFence)          vkDestroyFence(m_device, m_immediateFence, nullptr);
        if (m_immediateCommandPool)    vkDestroyCommandPool(m_device, m_immediateCommandPool, nullptr);
        if (m_device)                  vkDestroyDevice(m_device, nullptr);
        if (m_surface)                 vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        if (m_debugCallback)           vkDestroyDebugReportCallbackEXT(m_instance, m_debugCallback, nullptr);
        if (m_instance)                vkDestroyInstance(m_instance, nullptr);

        m_infoCallback("Destroyed vulkan context");
    }

    void Context::checkLayersSupport(std::span<const char*> t_layerNames) noexcept
    {
        m_infoCallback("Enumerating instance layers:");

        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for (const char* layerName : t_layerNames)
        {
            m_infoCallback("\t- " + std::string(layerName));

            auto layerIt = std::find_if(layers.begin(), layers.end(),
            [&layerName](VkLayerProperties const& layer)
            {
                return std::strcmp(layer.layerName, layerName) == 0;
            });

            if (layerIt == layers.end())
            {
                m_errorCallback("Cannot enable instance layer: " + std::string(layerName));
                return;
            }
        }
    }

    void Context::checkExtensionsSupport(std::span<const char*> t_extensionNames) noexcept
    {
        m_infoCallback("Enumerating instance extensions:");

        u32 extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        for (const char* extensionName : t_extensionNames)
        {
            m_infoCallback("\t- " + std::string(extensionName));

            auto layerIt = std::find_if(extensions.begin(), extensions.end(),
            [extensionName](VkExtensionProperties const& extension)
            {
                return std::strcmp(extension.extensionName, extensionName) == 0;
            });

            if (layerIt == extensions.end())
            {
                m_errorCallback("Cannot enable instance extension: " + std::string(extensionName));
                return;
            }
        }
    }

    auto Context::findQueueFamily(VkPhysicalDevice t_physicalDevice) noexcept -> u32
    {
        u32 propertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(t_physicalDevice, &propertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> properties(propertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(t_physicalDevice, &propertyCount, properties.data());

        for (u32 i = 0; i < propertyCount; ++i)
        {
            VkBool32 supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(t_physicalDevice, i, m_surface, &supported);

            if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
                supported)
            {
                return i;
            }
        }

        return ~0u;
    }

    void Context::selectPhysicalDevice() noexcept
    {
        u32 physicalDeviceCount;
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

        if (physicalDevices.empty())
        {
            m_errorCallback("No suitable physical devices");
        }

        for (auto const& gpu : physicalDevices)
        {
            VkPhysicalDeviceProperties2 gpuProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            VkPhysicalDeviceFeatures2 gpuFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            u32 queueFamilyIndex;

            vkGetPhysicalDeviceProperties2(gpu, &gpuProperties);
            vkGetPhysicalDeviceFeatures2(gpu, &gpuFeatures);

            if (gpuProperties.properties.apiVersion < VK_API_VERSION_1_3)
            {
                m_infoCallback(gpuProperties.properties.deviceName + std::string(": does not support Vulkan 1.3. Searching for another device"));
                continue;
            }

            if (gpuFeatures.features.geometryShader == false)
            {
                m_infoCallback(gpuProperties.properties.deviceName + std::string(": does not support geometry shader. Searching for another device"));
                continue;
            }

            queueFamilyIndex = findQueueFamily(gpu);
            if (~0u == queueFamilyIndex)
            {
                m_infoCallback(gpuProperties.properties.deviceName + std::string(": can not find sufficient queue family index. Searching for another device"));
                continue;
            }

            m_physicalDevice = gpu;
            m_physicalDeviceProperties = gpuProperties;
            m_physicalDeviceFeatures = gpuFeatures;
            m_queueFamilyIndex = queueFamilyIndex;
        }
    }

    void Context::createLogicalDevice() noexcept
    {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensionProperties(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, extensionProperties.data());

        for (auto& extension : extensionProperties)
        {
            if (std::strcmp(VK_EXT_MESH_SHADER_EXTENSION_NAME, extension.extensionName) == 0)
            {
                m_deviceExtensions.emplace_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
                m_meshShaderSupported = true;
            }
        }
        m_deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        const f32 priority = 0.f;

        VkDeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfo.flags = 0;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = m_queueFamilyIndex;
        queueCreateInfo.pQueuePriorities = &priority;

        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
        meshShaderFeatures.taskShader = true;
        meshShaderFeatures.meshShader = true;

        VkPhysicalDeviceVulkan11Features vulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        vulkan11Features.shaderDrawParameters     = true;
        vulkan11Features.multiview                = true;
        vulkan11Features.storageBuffer16BitAccess = true;
        if (m_meshShaderSupported)
        { 
            vulkan11Features.pNext = &meshShaderFeatures;
        }

        VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        vulkan12Features.pNext = &vulkan11Features;
        vulkan12Features.runtimeDescriptorArray                             = true;
        vulkan12Features.descriptorBindingVariableDescriptorCount           = true;
        vulkan12Features.descriptorBindingPartiallyBound                    = true;
        vulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing       = true;
        vulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing       = true;
        vulkan12Features.shaderUniformBufferArrayNonUniformIndexing         = true;
        vulkan12Features.shaderSampledImageArrayNonUniformIndexing          = true;
        vulkan12Features.shaderStorageBufferArrayNonUniformIndexing         = true;
        vulkan12Features.shaderStorageImageArrayNonUniformIndexing          = true;
        vulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing    = true;
        vulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing    = true;
        vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind      = true;
        vulkan12Features.descriptorBindingSampledImageUpdateAfterBind       = true;
        vulkan12Features.descriptorBindingStorageImageUpdateAfterBind       = true;
        vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind      = true;
        vulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind = true;
        vulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind = true;
        vulkan12Features.drawIndirectCount                                  = true;
        vulkan12Features.storageBuffer8BitAccess                            = true;
        vulkan12Features.uniformAndStorageBuffer8BitAccess                  = true;
        vulkan12Features.shaderFloat16                                      = true;
        vulkan12Features.shaderInt8                                         = true;
        vulkan12Features.samplerFilterMinmax                                = true;
        vulkan12Features.scalarBlockLayout                                  = true;
        vulkan12Features.bufferDeviceAddress                                = true;

        VkPhysicalDeviceVulkan13Features vulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        vulkan13Features.pNext = &vulkan12Features;
        vulkan13Features.synchronization2 = true;
        vulkan13Features.dynamicRendering = true;
        vulkan13Features.maintenance4     = true;

        VkPhysicalDeviceFeatures2 enabledFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        enabledFeatures.pNext = &vulkan13Features;
        enabledFeatures.features.fillModeNonSolid        = true;
        enabledFeatures.features.wideLines               = true;
        enabledFeatures.features.depthClamp              = true;
        enabledFeatures.features.multiDrawIndirect       = true;
        enabledFeatures.features.shaderInt16             = true;
        enabledFeatures.features.shaderInt64             = true;
        enabledFeatures.features.pipelineStatisticsQuery = true;
        enabledFeatures.features.samplerAnisotropy       = true;
        enabledFeatures.features.sampleRateShading       = true;

        VkDeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = (void*)&enabledFeatures;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;

        if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            m_errorCallback("Failed to create vulkan device");
        }
        volkLoadDevice(m_device);

        m_infoCallback("Created vulkan device");
    }

    void Context::createAllocator() noexcept
    {
        VmaVulkanFunctions functions;
        functions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
        functions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
        functions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
        functions.vkAllocateMemory                        = vkAllocateMemory;
        functions.vkFreeMemory                            = vkFreeMemory;
        functions.vkMapMemory                             = vkMapMemory;
        functions.vkUnmapMemory                           = vkUnmapMemory;
        functions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
        functions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
        functions.vkBindBufferMemory                      = vkBindBufferMemory;
        functions.vkBindImageMemory                       = vkBindImageMemory;
        functions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
        functions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
        functions.vkCreateBuffer                          = vkCreateBuffer;
        functions.vkDestroyBuffer                         = vkDestroyBuffer;
        functions.vkCreateImage                           = vkCreateImage;
        functions.vkDestroyImage                          = vkDestroyImage;
        functions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
        functions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
        functions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
        functions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
        functions.vkBindImageMemory2KHR                   = vkBindImageMemory2;
        functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
        functions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
        functions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = m_physicalDevice;
        allocatorCreateInfo.device = m_device;
        allocatorCreateInfo.instance = m_instance;
        allocatorCreateInfo.pVulkanFunctions = &functions;

        if (vmaCreateAllocator(&allocatorCreateInfo, &m_allocator) != VK_SUCCESS)
        {
            m_errorCallback("Failed to create vulkan memory allocator");
        }
        m_infoCallback("Created vulkan memory allocator");
    }

    auto Context::findSupportedFormat(const std::vector<Format>& t_formats, ImageTiling t_tiling, FormatFeatures t_features) noexcept -> Format
    {
        for (auto& format : t_formats)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, static_cast<VkFormat>(format), &properties);

            if (t_tiling == ImageTiling::eOptimal && (properties.optimalTilingFeatures & t_features) == t_features)
            {
                return format;
            }
            if (t_tiling == ImageTiling::eLinear && (properties.linearTilingFeatures & t_features) == t_features)
            {
                return format;
            }
        }
        return Format::eUndefined;
    }
}