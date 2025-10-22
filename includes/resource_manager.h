#pragma once

#include "image.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <vector>

namespace srtv_engine
{
class ResourceManager {
  public:
	
	ResourceManager() {
		_allocator = nullptr;
	}

	void init(VkInstance instance, VkPhysicalDevice chosenGPU, VkDevice device);

	void flush();

	void registerImage(image::AllocatedImage allocatedImage);

	void destroy() {
		if (_allocator != nullptr) {
			flush();
			vmaDestroyAllocator(_allocator);
		}
	}

  private:

	// delete copy constructor and copy assignment operator
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator= (const ResourceManager&) = delete;

	// vma allocator to manage ressources (mainly creation and deletion)
	VmaAllocator _allocator;

	// device handle
	VkDevice _device;

	std::vector<image::AllocatedImage> _allocatedImages;
};
} // namespace srtv_engine