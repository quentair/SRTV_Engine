#include "descriptor.h"

#include "error_checker.h"
#include "engine_logger.h"

namespace srtv_engine {

// *********************************** DESCRIPTOR LAYOUT BUILDER *********************************** //

void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBind = {};
    newBind.binding = binding;
    newBind.descriptorCount = 1;
    newBind.descriptorType = type;

    _bindings.push_back(newBind);
}

void DescriptorLayoutBuilder::clear()
{
    _bindings.clear();
}
    
VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& binding : _bindings) {
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;

    descriptorSetLayoutCreateInfo.pBindings = _bindings.data();
    descriptorSetLayoutCreateInfo.bindingCount = _bindings.size();
    descriptorSetLayoutCreateInfo.flags = flags;

    VkDescriptorSetLayout descriptorSetLayout;

    CHECK_VK_ERROR(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

    return descriptorSetLayout;
}

// *********************************** DESCRIPTOR ALLOCATOR GROWABLE *********************************** //

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t initialSetsCount, std::span<PoolSizeRatio> poolRatios)
{
    _ratios.clear();

    for (auto ratio : poolRatios) {
        _ratios.push_back(ratio);
    }

    VkDescriptorPool newPool = createPool(device, initialSetsCount, poolRatios);

    _setsPerPool = initialSetsCount * 1.5; //grow it next allocation

    _readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clearPools(VkDevice device)
{
    // reset all pools

    for (auto p : _readyPools) {
        CHECK_VK_ERROR(vkResetDescriptorPool(device, p, 0));
    }

    // all full pools are reset and so becomes ready to use again 
    // need also a clear this time because we are doing a copy with push back, and there is technically no more full pools
    for (auto pool : _fullPools) {
        CHECK_VK_ERROR(vkResetDescriptorPool(device, pool, 0));
        _readyPools.push_back(pool);
    }
    _fullPools.clear();
}

void DescriptorAllocatorGrowable::destroyPools(VkDevice device)
{
    // destroy all pools

    for (auto pool : _readyPools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    _readyPools.clear();

    for (auto pool : _fullPools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
    _fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    // get or create a pool to allocate from
    VkDescriptorPool poolToUse = getPool(device);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.pNext = pNext;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = poolToUse;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet);
    CHECK_VK_ERROR(result);

    // check if allocation failed, if so, so we try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

        // likely, the descriptor pool is already full, so we add it into the fullPools array
        _fullPools.push_back(poolToUse);

        // get or create another pool to allocate from
        poolToUse = getPool(device);
        descriptorSetAllocateInfo.descriptorPool = poolToUse;

        // if the allocation failed a second time, abort because there is something wrong
        CHECK_VK_FATAL_ERROR(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));
    }

    // add it back into the readyPools
    // if it is full, it will be detected the next time we try to allocate from it, so no need to check
    _readyPools.push_back(poolToUse);
    return descriptorSet;
}

VkDescriptorPool DescriptorAllocatorGrowable::getPool(VkDevice device)
{
    VkDescriptorPool newPool;

    if (_readyPools.size() != 0) {
        // borrow an available pool
        newPool = _readyPools.back();
        _readyPools.pop_back();
    }
    else {
        // no available pool
        // need to create a new pool
        newPool = createPool(device, _setsPerPool, _ratios);

        // mimic std::vector resize to increase the number of sets we can allocate
        _setsPerPool = _setsPerPool * 1.5;
        if (_setsPerPool > 4092) {
            _setsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::createPool(VkDevice device, uint32_t descriptorSetCount, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes;

    for (PoolSizeRatio ratio : poolRatios) {
        descriptorPoolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio._type,
            .descriptorCount = uint32_t(ratio._ratio * descriptorSetCount) // number of descriptors of that type to allocate from the pool (in total)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = descriptorSetCount; // how many VkDescriptorSets we can create from the pool in total
    pool_info.poolSizeCount = (uint32_t)descriptorPoolSizes.size();
    pool_info.pPoolSizes = descriptorPoolSizes.data();

    VkDescriptorPool newPool;
    CHECK_VK_ERROR(vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool));

    return newPool;
}

// *********************************** DESCRIPTOR WRITER *********************************** //

void DescriptorWriter::writeImage(int binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& descriptorImageInfo = _imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = sampler,
            .imageView = imageView,
            .imageLayout = layout
    });

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.pNext = nullptr;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.pImageInfo = &descriptorImageInfo;

    _writes.push_back(writeDescriptorSet);
}

void DescriptorWriter::writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& descriptorBufferInfo = _bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.dstSet = VK_NULL_HANDLE; // left empty for now until we need to write it
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    _writes.push_back(writeDescriptorSet);
}

void DescriptorWriter::clear()
{
    _imageInfos.clear();
    _writes.clear();
    _bufferInfos.clear();
}

void DescriptorWriter::updateSet(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : _writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, (uint32_t)_writes.size(), _writes.data(), 0, nullptr);
}

} // namespace srtv_engine