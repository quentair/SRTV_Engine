#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <span>
#include <deque>

namespace srtv_engine {

class PoolSizeRatio {
  public:
	VkDescriptorType _type;
	uint32_t _ratio;
};

class DescriptorLayoutBuilder {
  public:
    std::vector<VkDescriptorSetLayoutBinding> _bindings;

    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

// handles a bunch of descriptor bool
// whenever a pool fails to allocate, we create a new one
class DescriptorAllocatorGrowable {
public:

	void init(VkDevice device, uint32_t initialSetsCount, std::span<PoolSizeRatio> poolRatios);
	void clearPools(VkDevice device);
	void destroyPools(VkDevice device);

	VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);
private:
	VkDescriptorPool getPool(VkDevice device);
	VkDescriptorPool createPool(VkDevice device, uint32_t descriptorSetCount, std::span<PoolSizeRatio> poolRatios);

	std::vector<PoolSizeRatio> _ratios;
	std::vector<VkDescriptorPool> _fullPools; // pools we can't no more allocate from (full pools)
	std::vector<VkDescriptorPool> _readyPools; // pools that can still be used (available pools)
	uint32_t _setsPerPool; // controls how many VkDescriptorSets we can create from the pool in total

};

class DescriptorWriter {
  public:
	// use deque containers because they don't invalidate references to elements in them when doing emplace, push, resize and erase operations
	std::deque<VkDescriptorImageInfo> _imageInfos;
	std::deque<VkDescriptorBufferInfo> _bufferInfos;
	std::vector<VkWriteDescriptorSet> _writes;

	void writeImage(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
	void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

	void clear();
	void updateSet(VkDevice device, VkDescriptorSet set);
};

} // namespace srtv_engine