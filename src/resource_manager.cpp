#include "resource_manager.h"

#include "error_checker.h"
#include "engine_logger.h"

void srtv_engine::ResourceManager::init(VkInstance instance, VkPhysicalDevice chosenGPU, VkDevice device)
{
	ENGINE_LOG_TRACE("Building VMA allocator...");

	VmaAllocatorCreateInfo vmaAllocatorCreateInfo = {};
	vmaAllocatorCreateInfo.instance = instance;
	vmaAllocatorCreateInfo.physicalDevice = chosenGPU;
	vmaAllocatorCreateInfo.device = device;
	vmaAllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

	CHECK_VK_FATAL_ERROR(vmaCreateAllocator(&vmaAllocatorCreateInfo, &_allocator));

	_device = device;

	ENGINE_LOG_TRACE("VMA allocator creation went fine");
}

void srtv_engine::ResourceManager::flush()
{
	for (auto& allocateImage : _allocatedImages) {
		vkDestroyImageView(_device, allocateImage.imageView, nullptr);
		vmaDestroyImage(_allocator, allocateImage.image, allocateImage.allocation);
	}

	_allocatedImages.clear();
}

void srtv_engine::ResourceManager::registerImage(image::AllocatedImage allocatedImage)
{
	_allocatedImages.push_back(allocatedImage);
}
