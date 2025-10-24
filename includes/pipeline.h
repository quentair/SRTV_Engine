#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace srtv_engine {

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentFormat;

    PipelineBuilder() { clear(); }

    void clear();

    VkPipeline buildPipeline(VkDevice device);

    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setMultisamplingNone();

    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphablend();

    void setColorAttachmentFormat(VkFormat format);
    void setDepthFormat(VkFormat format);

    void disableDepthtest();
    void enableDepthtest(bool depthWriteEnable, VkCompareOp op);

    VkPipelineShaderStageCreateInfo defaultPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
};

} // namespace srtv_engine