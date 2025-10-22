#pragma once

#include "resource_manager.h"

#include <vulkan/vulkan.h>

namespace srtv_engine {

class FrameData {
  public:

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
	VkSemaphore _acquireImageSemaphore;
	VkFence _renderFence;

	ResourceManager _frameResourceManager;
};

} // namespace srtv_engine