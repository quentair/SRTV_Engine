#pragma once

#include "image.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <vector>

namespace srtv_engine
{

class ResourceDeletor {
  public:

	ResourceDeletor() = default;

	void flush(VmaAllocator allocator, VkDevice device);

	void registerImage(image::AllocatedImage* allocatedImage);

  private:

	// delete copy constructor and copy assignment operator
	ResourceDeletor(const ResourceDeletor&) = delete;
	ResourceDeletor& operator= (const ResourceDeletor&) = delete;

	std::vector<image::AllocatedImage*> _allocatedImages;
};

} // namespace srtv_engine