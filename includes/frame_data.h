#pragma once

#include "resource_deletor.h"
#include "buffer.h"

#include <vulkan/vulkan.h>

namespace srtv_engine {

struct FrameData {
  public:

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
	VkSemaphore _acquireImageSemaphore;
	VkFence _renderFence;

	// SSBOs
	buffer::AllocatedBuffer _worldDataBuffer;
	buffer::AllocatedBuffer _viewDistanceGridBuffer;
	buffer::AllocatedBuffer _chunksDataBuffer;

	// buffers allocations
	VmaAllocationInfo _worldDataBufferAllocInfo;
	VmaAllocationInfo _viewDistanceGridAllocInfo;
	VmaAllocationInfo _chunksDataBufferAllocInfo;

	// descriptors sets
	VkDescriptorSet _worldgenDescriptorSet;

	ResourceDeletor _frameResourceDeletor;
};

} // namespace srtv_engine