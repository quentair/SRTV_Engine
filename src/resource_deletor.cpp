#include "resource_deletor.h"

namespace srtv_engine {

void ResourceDeletor::flush(VmaAllocator allocator, VkDevice device)
{
	// delete all recorded objects and clear the vectors
	for (auto& allocateImage : _allocatedImages) {
		vkDestroyImageView(device, allocateImage->_imageView, nullptr);
		vmaDestroyImage(allocator, allocateImage->_image, allocateImage->_allocation);
	}
	for (auto& descriptorAllocator : _descriptorAllocators) {
		descriptorAllocator->destroyPools(device);
	}
	for (auto& descriptorSetayout : _descriptorSetLayouts) {
		vkDestroyDescriptorSetLayout(device, *descriptorSetayout, nullptr);
	}

	_allocatedImages.clear();
}

void ResourceDeletor::registerImage(image::AllocatedImage* allocatedImage)
{
	// register image allocated with VMA and its imageview for further deletion
	_allocatedImages.push_back(allocatedImage);
}

void ResourceDeletor::registerDescriptorAllocatorGrowble(DescriptorAllocatorGrowable* descriptorAllocator)
{
	// register descriptor allocator for further deletion
	_descriptorAllocators.push_back(descriptorAllocator);
}

void ResourceDeletor::registerDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout)
{
	// register descriptor set layout for further deletion
	_descriptorSetLayouts.push_back(descriptorSetLayout);
}

} // namespace srtv_engine