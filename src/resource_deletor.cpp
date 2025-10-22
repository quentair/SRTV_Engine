#include "resource_deletor.h"

#include "error_checker.h"
#include "engine_logger.h"

namespace srtv_engine {

void ResourceDeletor::flush(VmaAllocator allocator, VkDevice device)
{
	// delete all recorded objects and clear the vectors
	for (auto& allocateImage : _allocatedImages) {
		vkDestroyImageView(device, allocateImage->imageView, nullptr);
		vmaDestroyImage(allocator, allocateImage->image, allocateImage->allocation);
	}

	_allocatedImages.clear();
}

void ResourceDeletor::registerImage(image::AllocatedImage* allocatedImage)
{
	// register image allocated with VMA and its imageview for further deletion
	_allocatedImages.push_back(allocatedImage);
}

} // namespace srtv_engine