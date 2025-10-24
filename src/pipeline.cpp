#include "pipeline.h"

#include "error_checker.h"
#include "engine_logger.h"

namespace srtv_engine {

void PipelineBuilder::clear()
{
	// clear all of the structs we need back to 0 with their correct sType

	_inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

	_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	_colorBlendAttachment = {};

	_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	_pipelineLayout = {};

	_depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	_renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

	_shaderStages.clear();
}

VkPipeline PipelineBuilder::buildPipeline(VkDevice device)
{
	// make viewport state from our stored viewport and scissor
	// at the moment we won't support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// setup dummy colod blending 
	// because we aren't using transparent object yet, the blending is just "no blend"
	// but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorblending = {};
	colorblending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorblending.pNext = nullptr;

	colorblending.logicOpEnable = VK_FALSE;
	colorblending.logicOp = VK_LOGIC_OP_COPY;
	colorblending.attachmentCount = 1;
	colorblending.pAttachments = &_colorBlendAttachment;

	// completly clear the VertexInputStateCreateInfo, as we have no need for it
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	// build the actual pipeline
	// we now use all of the info structs we have been writing into this one
	// to create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	// connect the renderInfo to the pNext extension mechanism
	pipelineInfo.pNext = &_renderInfo;

	pipelineInfo.stageCount = (uint32_t)_shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorblending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;

	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2;

	pipelineInfo.pDynamicState = &dynamicInfo;

	// it is easy to error out on create graphics pipeline 
	// so we handle it a bit better  than the common VK_CHECK case
	VkPipeline newPipeline;
	auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline);
	CHECK_VK_ERROR(result);
	if (result != VK_SUCCESS) {
		ENGINE_LOG_ERROR("Failed to create pipeline");
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	else {
		return newPipeline;
	}
}

void PipelineBuilder::setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
	_shaderStages.clear();

	_shaderStages.push_back(defaultPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

	_shaderStages.push_back(defaultPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::setInputTopology(VkPrimitiveTopology topology)
{
	_inputAssembly.topology = topology;
	// we are not going to use primitive restart on the entire tutorial so leave it on false
	_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::setPolygonMode(VkPolygonMode mode)
{
	_rasterizer.polygonMode = mode;
	_rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
	_rasterizer.cullMode = cullMode;
	_rasterizer.frontFace = frontFace;
}

void PipelineBuilder::setMultisamplingNone()
{
	_multisampling.sampleShadingEnable = VK_FALSE;
	//multisampling defaulted to no multisampling (1 sample per pixel)
	_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	_multisampling.minSampleShading = 1.0f;
	_multisampling.pSampleMask = nullptr;
	// no alpha to coverage either
	_multisampling.alphaToCoverageEnable = VK_FALSE;
	_multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::disableBlending()
{
	// default write mask
	_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// no blending
	_colorBlendAttachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::enableBlendingAdditive()
{
	_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_colorBlendAttachment.blendEnable = VK_TRUE;
	_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::enableBlendingAlphablend()
{
	_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_colorBlendAttachment.blendEnable = VK_TRUE;
	_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::setColorAttachmentFormat(VkFormat format)
{
	_colorAttachmentFormat = format;
	// connect the format to the renderInfo structure
	_renderInfo.colorAttachmentCount = 1;
	_renderInfo.pColorAttachmentFormats = &_colorAttachmentFormat;
}

void PipelineBuilder::setDepthFormat(VkFormat format)
{
	_renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::disableDepthtest()
{
	_depthStencil.depthTestEnable = VK_FALSE;
	_depthStencil.depthWriteEnable = VK_FALSE;
	_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	_depthStencil.depthBoundsTestEnable = VK_FALSE;
	_depthStencil.stencilTestEnable = VK_FALSE;
	_depthStencil.front = {};
	_depthStencil.back = {};
	_depthStencil.minDepthBounds = 0.f;
	_depthStencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::enableDepthtest(bool depthWriteEnable, VkCompareOp op)
{
	_depthStencil.depthTestEnable = VK_TRUE;
	_depthStencil.depthWriteEnable = depthWriteEnable;
	_depthStencil.depthCompareOp = op;
	_depthStencil.depthBoundsTestEnable = VK_FALSE;
	_depthStencil.stencilTestEnable = VK_FALSE;
	_depthStencil.front = {};
	_depthStencil.back = {};
	_depthStencil.minDepthBounds = 0.f;
	_depthStencil.maxDepthBounds = 1.f;
}

VkPipelineShaderStageCreateInfo PipelineBuilder::defaultPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry)
{
	VkPipelineShaderStageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = nullptr;

	// shader stage
	info.stage = stage;
	// module containing the code for this shader stage
	info.module = shaderModule;
	// the entry point of the shader (function name)
	info.pName = entry;
	return info;
}

} // namespace srtv_engine