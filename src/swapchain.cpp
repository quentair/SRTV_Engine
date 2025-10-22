#include "swapchain.h"

#include "error_checker.h"
#include "engine_logger.h"

#include <VkBootstrap.h>

void srtv_engine::Swapchain::init(VkPhysicalDevice chosenGPU, VkDevice device, VkSurfaceKHR surface)
{
	_chosenGPU = chosenGPU;
	_device = device;
	_surface = surface;
}

void srtv_engine::Swapchain::create(uint32_t width, uint32_t height)
{
	// create the vulkan swapchain with vk-bootstrap

	ENGINE_LOG_TRACE("Building Vulkan Swapchain with SDL...");

	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
	VkSurfaceFormatKHR surfaceFormat{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	auto swapchainBuilderReturn = swapchainBuilder
		.set_desired_format(surfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();

	CHECK_VKB_FATAL_ERROR(swapchainBuilderReturn);

	vkb::Swapchain vkbSwapchain = swapchainBuilderReturn.value();

	_swapchainExtent = vkbSwapchain.extent;

	// store swapchain, its related images and their semaphores for sync
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();
	_renderSemaphores = std::vector<VkSemaphore>(numberOfImages());

	ENGINE_LOG_TRACE("Vulkan Swapchain creation went fine");
}

void srtv_engine::Swapchain::destroy()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}

void srtv_engine::Swapchain::getNextImage(VkSemaphore acquireImageSemaphore, uint32_t *swapchainImageIndex)
{
	// get next available image from the swapchain
	vkAcquireNextImageKHR(_device, _swapchain, 1000000000, acquireImageSemaphore, nullptr, swapchainImageIndex);
}
