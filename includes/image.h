#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace srtv_engine::image {

class AllocatedImage {
  public :
    VkImage _image;
    VkImageView _imageView;
    VmaAllocation _allocation;
    VkExtent3D _imageExtent;
    VkFormat _imageFormat;
};

VkImageCreateInfo defaultImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

VkImageViewCreateInfo defaultImageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

VkImageSubresourceRange createDefaultImageSubresourceRange(VkImageAspectFlags aspectMask);

void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

} // namespace srtv_engine::image