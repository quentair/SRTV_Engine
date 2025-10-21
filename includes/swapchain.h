#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace srtv_engine {

class Swapchain {
  public:

	  void init(VkPhysicalDevice chosenGPU, VkDevice device, VkSurfaceKHR surface);

	  void create(uint32_t width, uint32_t height);

	  const VkSwapchainKHR* getSwapchain() const { return &_swapchain; }

	  const int numberOfImages() const { return _swapchainImages.size(); }

	  VkImage getImageAt(int index) const { return _swapchainImages[index]; }

	  std::vector<VkSemaphore>& getRenderSemaphores() { return _renderSemaphores; }

	  void destroy();

	  void getNextImage(VkSemaphore acquireImageSemaphore, uint32_t* swapchainImageIndex);

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
	std::vector<VkSemaphore> _renderSemaphores;
};

} // namespace srtv_engine