#pragma once

#include "image.h"
#include "descriptor.h"

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

	void registerDescriptorAllocatorGrowble(DescriptorAllocatorGrowable* descriptorAllocator);

	void registerDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout);

	void registerPipelineLayout(VkPipelineLayout* pipelineLayout);

	void registerPipeline(VkPipeline* pipeline);

  private:

	// delete copy constructor and copy assignment operator
	ResourceDeletor(const ResourceDeletor&) = delete;
	ResourceDeletor& operator= (const ResourceDeletor&) = delete;

	std::vector<image::AllocatedImage*> _allocatedImages;

	std::vector<DescriptorAllocatorGrowable*> _descriptorAllocators;

	std::vector<VkDescriptorSetLayout*> _descriptorSetLayouts;

	std::vector<VkPipelineLayout*> _pipelineLayouts;

	std::vector<VkPipeline*> _pipelines;
};

} // namespace srtv_engine