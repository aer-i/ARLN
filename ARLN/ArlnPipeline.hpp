#pragma once
#include "ArlnUtility.hpp"
#include "ArlnDescriptor.hpp"

namespace arln {

    class Pipeline
    {
    private:
        friend class Context;
        explicit Pipeline(GraphicsPipelineInfo const& t_info) noexcept;
        explicit Pipeline(ComputePipelineInfo const& t_info) noexcept;

    public:
        Pipeline() = default;
        Pipeline(Pipeline const&) = default;
        Pipeline(Pipeline&&) = default;
        Pipeline& operator=(Pipeline const&) = default;
        Pipeline& operator=(Pipeline&&) = default;
        ~Pipeline() = default;

        void destroy() noexcept;

        inline auto& getHandle() const noexcept { return m_handle; }
        inline auto& getLayout() const noexcept { return m_layout; }

    private:
        VkPipeline m_handle;
        VkPipelineLayout m_layout;
    };
}