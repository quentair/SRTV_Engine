#include "resource_deletor.h"

namespace srtv_engine {

void ResourceDeletor::flush(VmaAllocator allocator, VkDevice device)
{
	// delete all recorded objects and clear the vectors
	for (auto& allocatedImage : _allocatedImages) {
		vkDestroyImageView(device, allocatedImage->_imageView, nullptr);
		vmaDestroyImage(allocator, allocatedImage->_image, allocatedImage->_allocation);
	}
	for (auto& allocatedBuffer : _allocatedBuffers) {
		vmaDestroyBuffer(allocator, allocatedBuffer->_buffer, allocatedBuffer->_allocation);
	}
	for (auto& descriptorAllocator : _descriptorAllocators) {
		descriptorAllocator->destroyPools(device);
	}
	for (auto decriptorPool : _descriptorPools) {
		vkDestroyDescriptorPool(device, decriptorPool, nullptr);
	}
	for (auto descriptorSetLayout : _descriptorSetLayouts) {
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	}
	// destroy pipeline layout first, then pipeline
	for (auto pipelineLayout : _pipelineLayouts) {
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	}
	for (auto pipeline : _pipelines) {
		vkDestroyPipeline(device, pipeline, nullptr);
	}

	_allocatedImages.clear();
}

void ResourceDeletor::registerImage(image::AllocatedImage* allocatedImage)
{
	// register image allocated with VMA and its imageview for further deletion
	_allocatedImages.push_back(allocatedImage);
}

void ResourceDeletor::registerBuffer(buffer::AllocatedBuffer* allocatedBuffer)
{
	// register buffer allocated with VMA for further deletion
	_allocatedBuffers.push_back(allocatedBuffer);
}

void ResourceDeletor::registerDescriptorAllocatorGrowable(DescriptorAllocatorGrowable* descriptorAllocator)
{
	// register descriptor allocator for further deletion
	_descriptorAllocators.push_back(descriptorAllocator);
}

void ResourceDeletor::registerDescriptorPool(VkDescriptorPool descriptorPool)
{
	_descriptorPools.push_back(descriptorPool);
}

void ResourceDeletor::registerDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
	// register descriptor set layout for further deletion
	_descriptorSetLayouts.push_back(descriptorSetLayout);
}

void ResourceDeletor::registerPipelineLayout(VkPipelineLayout pipelineLayout)
{
	// register pipeline layout for further deletion
	_pipelineLayouts.push_back(pipelineLayout);
}

void ResourceDeletor::registerPipeline(VkPipeline pipeline)
{
	// register pipeline for further deletion
	_pipelines.push_back(pipeline);
}

} // namespace srtv_engine