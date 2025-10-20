#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace srtv_engine {

class Swapchain {
  public:

	  void init(VkPhysicalDevice chosenGPU, VkDevice device, VkSurfaceKHR surface);

	  void create(uint32_t width, uint32_t height);

	  void destroy();

  private:

	// GPU and logical device (interface with GPU)
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	// surface
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;
};

} // namespace srtv_engine