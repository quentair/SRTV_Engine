#pragma once

#include <vulkan/vulkan.h>

namespace srtv_engine::image {
	void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

	VkImageSubresourceRange createImageSubresourceRange(VkImageAspectFlags aspectMask);
} // namespace srtv_engine::image