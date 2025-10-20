#pragma once

#include <vulkan/vulkan.h>

namespace srtv_engine {

class FrameData {
  public:

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

} // namespace srtv_engine