#include "ArlnPipeline.hpp"
#include "ArlnContext.hpp"
#include <fstream>

namespace arln {

    static std::vector<char> readFile(std::string_view t_filepath) noexcept
    {
        if (t_filepath.empty())
        {
            return {};
        }

        std::ifstream file(t_filepath.data(), std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            return {};
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), (std::streamsize)fileSize);

        file.close();

        return buffer;
    }

    Pipeline::Pipeline(GraphicsPipelineInfo const& t_info) noexcept
    {
        std::vector<VkPushConstantRange> pushConstantRanges(t_info.pushConstants.pushConstantRanges.size());
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

        for (size_t i = pushConstantRanges.size(); i--; )
        {
            pushConstantRanges[i].size = t_info.pushConstants.pushConstantRanges[i].size;
            pushConstantRanges[i].offset = t_info.pushConstants.pushConstantRanges[i].offset;
            pushConstantRanges[i].stageFlags = static_cast<VkShaderStageFlags>(t_info.pushConstants.pushConstantRanges[i].stage);
        }

        for (auto descriptorSetLayout : t_info.descriptors.layouts)
        {
            descriptorSetLayouts.emplace_back(descriptorSetLayout->getLayout());
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<u32>(pushConstantRanges.size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

        if (vkCreatePipelineLayout(CurrentContext()->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_layout) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create pipeline layout");
        }


        VkShaderModule vertShaderModule{ nullptr },
                       fragShaderModule{ nullptr },
                       meshShaderModule{ nullptr },
                       taskShaderModule{ nullptr };

        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;

        if (!t_info.vertShaderPath.empty())
        {
            const auto vertShaderCode = readFile(t_info.vertShaderPath);
            const auto fragShaderCode = readFile(t_info.fragShaderPath);

            VkShaderModuleCreateInfo vertShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            vertShaderModuleCreateInfo.codeSize = vertShaderCode.size();
            vertShaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(vertShaderCode.data());

            VkShaderModuleCreateInfo fragShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            fragShaderModuleCreateInfo.codeSize = fragShaderCode.size();
            fragShaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(fragShaderCode.data());

            vkCreateShaderModule(CurrentContext()->getDevice(), &vertShaderModuleCreateInfo, nullptr, &vertShaderModule);
            vkCreateShaderModule(CurrentContext()->getDevice(), &fragShaderModuleCreateInfo, nullptr, &fragShaderModule);

            shaderStageCreateInfos.resize(2);
            for (u32 i = 2; i--; )
            {
                shaderStageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                shaderStageCreateInfos[i].pNext = nullptr;
                shaderStageCreateInfos[i].flags = 0;
                shaderStageCreateInfos[i].stage = (i == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT);
                shaderStageCreateInfos[i].module = (i == 0 ? vertShaderModule : fragShaderModule);
                shaderStageCreateInfos[i].pName = "main";
                shaderStageCreateInfos[i].pSpecializationInfo = nullptr;
            }
        }
        else if (!t_info.meshShaderPath.empty())
        {
            const auto meshShaderCode = readFile(t_info.meshShaderPath);
            const auto fragShaderCode = readFile(t_info.fragShaderPath);

            if (!t_info.taskShaderPath.empty())
            {
                const auto taskShaderCode = readFile(t_info.taskShaderPath);
                VkShaderModuleCreateInfo taskShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
                taskShaderModuleCreateInfo.codeSize = taskShaderCode.size();
                taskShaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(taskShaderCode.data());
                vkCreateShaderModule(CurrentContext()->getDevice(), &taskShaderModuleCreateInfo, nullptr, &taskShaderModule);
            }

            VkShaderModuleCreateInfo meshShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            meshShaderModuleCreateInfo.codeSize = meshShaderCode.size();
            meshShaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(meshShaderCode.data());

            VkShaderModuleCreateInfo fragShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            fragShaderModuleCreateInfo.codeSize = fragShaderCode.size();
            fragShaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(fragShaderCode.data());

            vkCreateShaderModule(CurrentContext()->getDevice(), &meshShaderModuleCreateInfo, nullptr, &meshShaderModule);
            vkCreateShaderModule(CurrentContext()->getDevice(), &fragShaderModuleCreateInfo, nullptr, &fragShaderModule);

            shaderStageCreateInfos.emplace_back(VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_MESH_BIT_EXT,
                .module = meshShaderModule,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            });

            if (taskShaderModule)
            {
                shaderStageCreateInfos.emplace_back(VkPipelineShaderStageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_TASK_BIT_EXT,
                    .module = taskShaderModule,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
                });
            }

            shaderStageCreateInfos.emplace_back(VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragShaderModule,
                .pName = "main",
                .pSpecializationInfo = nullptr,
            });
        }
        else
        {
            CurrentContext()->getErrorCallback()("Could not find any shader file while creating vulkan pipeline");
        }

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount  = 1;

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        std::vector<VkVertexInputBindingDescription> bindingDescriptions(t_info.bindings.bindingDescriptions.size());
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(t_info.attributes.attributeDescriptions.size());

        for (size_t i = 0; i < bindingDescriptions.size(); ++i)
        {
            bindingDescriptions[i].binding = t_info.bindings.bindingDescriptions[i].binding;
            bindingDescriptions[i].stride = t_info.bindings.bindingDescriptions[i].stride;
            bindingDescriptions[i].inputRate = static_cast<VkVertexInputRate>(t_info.bindings.bindingDescriptions[i].vertexInputRate);
        }

        for (size_t i = 0; i < attributeDescriptions.size(); ++i)
        {
            attributeDescriptions[i].location = t_info.attributes.attributeDescriptions[i].location;
            attributeDescriptions[i].binding = t_info.attributes.attributeDescriptions[i].binding;
            attributeDescriptions[i].format = static_cast<VkFormat>(t_info.attributes.attributeDescriptions[i].format);
            attributeDescriptions[i].offset = t_info.attributes.attributeDescriptions[i].offset;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<u32>(bindingDescriptions.size());
        vertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        std::vector<VkFormat> colorFormats;
        if (!t_info.colorFormats.empty())
        {
            colorFormats.reserve(t_info.colorFormats.size());

            for (auto colorFormat : t_info.colorFormats)
            {
                colorFormats.emplace_back(static_cast<VkFormat>(colorFormat));
            }
        }
        else
        {
            colorFormats.emplace_back(CurrentContext()->getSwapchain().getFormat());
        }

        VkPipelineRenderingCreateInfo renderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        renderingCreateInfo.colorAttachmentCount = static_cast<u32>(colorFormats.size());
        renderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
        renderingCreateInfo.depthAttachmentFormat = static_cast<VkFormat>(t_info.depthFormat);
        renderingCreateInfo.stencilAttachmentFormat = static_cast<VkFormat>(t_info.stencilFormat);

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssemblyStateCreateInfo.topology = static_cast<VkPrimitiveTopology>(t_info.topology);

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizationStateCreateInfo.cullMode = static_cast<u32>(t_info.cullMode);
        rasterizationStateCreateInfo.frontFace = static_cast<VkFrontFace>(t_info.frontFace);
        rasterizationStateCreateInfo.polygonMode = static_cast<VkPolygonMode>(t_info.polygonMode);
        rasterizationStateCreateInfo.lineWidth = t_info.lineWidth;
        rasterizationStateCreateInfo.depthClampEnable = true;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.blendEnable = t_info.colorBlending;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &blendAttachmentState;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencilStateCreateInfo.depthTestEnable = t_info.depthStencil;
        depthStencilStateCreateInfo.depthWriteEnable = t_info.depthStencil;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = false;
        depthStencilStateCreateInfo.stencilTestEnable = false;

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        graphicsPipelineCreateInfo.pNext = (void*)&renderingCreateInfo;
        graphicsPipelineCreateInfo.stageCount = static_cast<u32>(shaderStageCreateInfos.size());
        graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos.data();
        graphicsPipelineCreateInfo.layout = m_layout;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        if (t_info.meshShaderPath.empty())
        {
            graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
            graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        }
        if (vkCreateGraphicsPipelines(CurrentContext()->getDevice(), nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_handle) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create graphics pipeline");
        }

        if (vertShaderModule) vkDestroyShaderModule(CurrentContext()->getDevice(), vertShaderModule, nullptr);
        if (fragShaderModule) vkDestroyShaderModule(CurrentContext()->getDevice(), fragShaderModule, nullptr);
        if (taskShaderModule) vkDestroyShaderModule(CurrentContext()->getDevice(), taskShaderModule, nullptr);
        if (meshShaderModule) vkDestroyShaderModule(CurrentContext()->getDevice(), meshShaderModule, nullptr);
    }

    Pipeline::Pipeline(ComputePipelineInfo const& t_info) noexcept
    {
        const auto compShaderCode = readFile(t_info.compShaderPath);

        if (compShaderCode.empty())
        {
            CurrentContext()->getErrorCallback()(std::string("Failed to open shader file: ") + t_info.compShaderPath.data());
        }

        std::vector<VkPushConstantRange> pushConstantRanges(t_info.pushConstants.pushConstantRanges.size());
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

        for (size_t i = pushConstantRanges.size(); i--; )
        {
            pushConstantRanges[i].size = t_info.pushConstants.pushConstantRanges[i].size;
            pushConstantRanges[i].offset = t_info.pushConstants.pushConstantRanges[i].offset;
            pushConstantRanges[i].stageFlags = static_cast<VkShaderStageFlags>(t_info.pushConstants.pushConstantRanges[i].stage);
        }

        for (auto descriptorSetLayout : t_info.descriptors.layouts)
        {
            descriptorSetLayouts.emplace_back(descriptorSetLayout->getLayout());
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<u32>(pushConstantRanges.size());
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

        if (vkCreatePipelineLayout(CurrentContext()->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_layout) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create pipeline layout");
        }

        VkShaderModuleCreateInfo compShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        compShaderModuleCreateInfo.codeSize = compShaderCode.size();
        compShaderModuleCreateInfo.pCode = reinterpret_cast<const u32*>(compShaderCode.data());

        VkShaderModule compShaderModule;
        vkCreateShaderModule(CurrentContext()->getDevice(), &compShaderModuleCreateInfo, nullptr, &compShaderModule);

        VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.pNext = nullptr;
        shaderStageCreateInfo.flags = 0;
        shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageCreateInfo.module = compShaderModule;
        shaderStageCreateInfo.pName = "main";
        shaderStageCreateInfo.pSpecializationInfo = nullptr;

        VkComputePipelineCreateInfo computePipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        computePipelineCreateInfo.layout = m_layout;
        computePipelineCreateInfo.stage = shaderStageCreateInfo;

        if (vkCreateComputePipelines(CurrentContext()->getDevice(), nullptr, 1, &computePipelineCreateInfo, nullptr, &m_handle) != VK_SUCCESS)
        {
            CurrentContext()->getErrorCallback()("Failed to create compute pipeline");
        }

        vkDestroyShaderModule(CurrentContext()->getDevice(), compShaderModule, nullptr);
    }

    void Pipeline::destroy() noexcept
    {
        CurrentContext()->getFrame().addPipelineToDestroy(*this);

        m_layout = nullptr;
        m_handle = nullptr;
    }
}