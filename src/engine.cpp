#include "engine.h"

#include <iostream>

#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>
#include <thread>

#include "error_checker.h"
#include "engine_logger.h"

namespace srtv_engine {

bool Engine::init()
{
	if (!createSdlWindow()) {
		ENGINE_LOG_ERROR("Failed to initialise SDL window.");
		return false;
	}

	if (!initVulkan()) {
		ENGINE_LOG_ERROR("Failed to initialise Vulkan.");
		return false;
	}

	if (!initSwapchain()) {
		ENGINE_LOG_ERROR("Failed to create swapchain.");
		return false;
	}
	_isInitialized = true;

	return true;
}

bool Engine::initVulkan()
{
	// create the vulkan instance with vk-bootstrap

	ENGINE_LOG_TRACE("Building Vulkan Instance...");

	vkb::InstanceBuilder instanceBuilder;

	auto instanceReturn = instanceBuilder.set_app_name("SRTV_Engine")
		.set_engine_name("SRTV_Engine")
		.request_validation_layers(_useValidationLayers)				// use validation layers ?
		.use_default_debug_messenger()				// TODO : change for custom logger ?
		.require_api_version(1, 3, 0)
		.build();

	CHECK_VKB_FATAL_ERROR(instanceReturn);

	vkb::Instance vkbInstance = instanceReturn.value();

	// get the VkInstance handle used in the rest of a vulkan application
	_instance = vkbInstance.instance;
	_debugMessenger = vkbInstance.debug_messenger;

	ENGINE_LOG_TRACE("Vulkan Instance creation went fine");

	// create a VkSurface for the SDL window

	ENGINE_LOG_TRACE("Building Vulkan Surface with SDL...");

	SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

	ENGINE_LOG_TRACE("Vulkan Surface creation went fine");

	// select a physical device (GPU) that is suitable for the engine

	ENGINE_LOG_TRACE("Selecting Vulkan Device...");

	vkb::PhysicalDeviceSelector physicalDeviceSelector{vkbInstance};

	// Vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features vulkan13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	vulkan13Features.dynamicRendering = true;
	vulkan13Features.synchronization2 = true;

	// Vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features vulkan12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	vulkan12Features.bufferDeviceAddress = true;
	vulkan12Features.descriptorIndexing = true;

	auto physicalDeviceSelectorReturn = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.set_required_features_12(vulkan12Features)
		.set_required_features_13(vulkan13Features)
		.set_surface(_surface)
		.select();

	CHECK_VKB_FATAL_ERROR(physicalDeviceSelectorReturn);
	vkb::PhysicalDevice VkbPhysicalDevice = physicalDeviceSelectorReturn.value();

	// optional extensions
	//bool supported = phys_device.enable_extension_if_present("VK_KHR_timeline_semaphore");
	//if (supported) {
		// allows easy feedback whether an extension is supported or not.
	//}

	// get the VkPhysicalDevice handle used in the rest of a vulkan application
	_chosenGPU = VkbPhysicalDevice.physical_device;

	ENGINE_LOG_TRACE("Vulkan Device selection went fine");

	// create the vulkan device with vk-bootstrap

	ENGINE_LOG_TRACE("Building Vulkan Device...");

	vkb::DeviceBuilder deviceBuilder{VkbPhysicalDevice};

	// automatically propagate needed data from instance & physical device
	auto deviceReturn = deviceBuilder.build();

	CHECK_VKB_FATAL_ERROR(deviceReturn);

	vkb::Device vkbDevice = deviceReturn.value();
	// get the VkDevice handle used in the rest of a vulkan application
	_device = vkbDevice.device;

	ENGINE_LOG_TRACE("Vulkan Device creation went fine");

	// get a queue for graphics from our device

	ENGINE_LOG_TRACE("Getting Vulkan Queue...");

	auto queueReturn = vkbDevice.get_queue(vkb::QueueType::graphics);

	CHECK_VKB_FATAL_ERROR(queueReturn);

	_graphicsQueue = queueReturn.value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	ENGINE_LOG_TRACE("Vulkan Queue acquisition went fine");

	return true;
}

void Engine::run()
{
	SDL_Event e;
	bool quit = false;

	// main loop
	while (!quit)
	{
		SDL_PollEvent(&e);

		switch (e.type) {
		case SDL_EVENT_KEY_DOWN :
			ENGINE_LOG_TRACE(fmt::format("Key pressed : {}", SDL_GetKeyName(e.key.key)));
			break;
		case SDL_EVENT_QUIT :
			quit = true;
			break;
		case SDL_EVENT_WINDOW_MINIMIZED :
			_stopRendering = true;
			break;
		case SDL_EVENT_WINDOW_RESTORED:
			_stopRendering = false;
			break;
		default:
			break;
		}

		// do not draw if we are minimized
		if (_stopRendering) {
			// throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
	}
}

void Engine::cleanup()
{
	// destroy in reversed creation order
	if (_isInitialized) {
		destroySwapchain();

		vkDestroyDevice(_device, nullptr);

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);

		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}
}

bool Engine::createSdlWindow()
{
	// initialise SDL and create a SDL window

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
		return false;
	}

	_window = SDL_CreateWindow("SRTV Engine",
		_windowExtent.width,
		_windowExtent.height,
		SDL_WINDOW_VULKAN
	);

	if (!_window) {
		SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
		return false;
	}

	return true;
}

bool Engine::initSwapchain()
{
	return createSwapchain(_windowExtent.width, _windowExtent.height);
}

bool Engine::createSwapchain(uint32_t width, uint32_t height)
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

	// store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	ENGINE_LOG_TRACE("Vulkan Swapchain creation went fine");

	return true;
}

void Engine::destroySwapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}

void Engine::init_commands()
{

}

} // namespace srtv_engine