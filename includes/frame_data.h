#pragma once

#include "resource_deletor.h"

#include <vulkan/vulkan.h>

namespace srtv_engine {

class FrameData {
  public:

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
	VkSemaphore _acquireImageSemaphore;
	VkFence _renderFence;

	ResourceDeletor _frameResourceDeletor;
};

} // namespace srtv_engine