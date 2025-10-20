#pragma once

#include <vulkan/vulkan.h>

namespace srtv_engine {

class FrameData {
  public:

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
	VkSemaphore _acquireImageSemaphore, _renderSemaphore;
	VkFence _renderFence;
};

} // namespace srtv_engine