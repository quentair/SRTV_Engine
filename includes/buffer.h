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

} // namespace srtv_engine::buffer