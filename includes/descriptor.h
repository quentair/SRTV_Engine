#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <span>
#include <deque>

namespace srtv_engine {

class PoolSizeRatio {
  public:
	VkDescriptorType type;
	float ratio;
};

class DescriptorLayoutbuilder {
  public:
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

// handles a bunch of descriptor bool
// whenever a pool fails to allocate, we create a new one
class DescriptorAllocatorGrowable {
public:

	void init(VkDevice device, uint32_t initialSetsCount, std::span<PoolSizeRatio> poolRatios);
	void clear_pools(VkDevice device);
	void destroy_pools(VkDevice device);

	VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);
private:
	VkDescriptorPool get_pool(VkDevice device);
	VkDescriptorPool create_pool(VkDevice device, uint32_t descriptorSetCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio> ratios;
	std::vector<VkDescriptorPool> fullPools; // pools we can't no more allocate from (full pools)
	std::vector<VkDescriptorPool> readyPools; // pools that can still be used (available pools)
	uint32_t setsPerPool; // controls how many VkDescriptorSets we can create from the pool in total

};

class DescriptorWriter {
  public:
	// use deque containers because they don't invalidate references to elements in them when doing emplace, push, resize and erase operations
	std::deque<VkDescriptorImageInfo> imageInfos;
	std::deque<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkWriteDescriptorSet> writes;

	void write_image(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
	void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

	void clear();
	void update_set(VkDevice device, VkDescriptorSet set);
};

} // namespace srtv_engine