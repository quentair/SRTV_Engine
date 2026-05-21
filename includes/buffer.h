#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace srtv_engine::buffer {

class AllocatedBuffer {
  public:
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

VkBufferCreateInfo defaultBufferCreateInfo(size_t allocSize, VkBufferUsageFlags usage);

VkBufferCreateInfo stagingBufferCreateInfo(size_t allocSize);

void copyBuffer(VkBuffer srcBuffer, VmaAllocationInfo srcAllocInfo, VkBuffer dstBuffer, VmaAllocationInfo dstAllocInfo, VkQueue queue, VkCommandBuffer commandBuffer);

} // namespace srtv_engine::buffer