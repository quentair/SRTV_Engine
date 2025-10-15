#include "engine.h"

#include <iostream>

#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>
#include <thread>

bool Engine::createSdlWindow()
{
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

	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
	VkSurfaceFormatKHR surfaceFormat{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	auto swapchainBuilderReturn = swapchainBuilder
		.set_desired_format(surfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();

	if (!swapchainBuilderReturn) {
		std::cout << swapchainBuilderReturn.error().message() << std::endl;
		return false;
	}
	vkb::Swapchain vkbSwapchain = swapchainBuilderReturn.value();

	_swapchainExtent = vkbSwapchain.extent;

	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

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

bool Engine::init()
{
	if (!createSdlWindow()) {
		std::cerr << "Failed to initialise SDL window." << std::endl;
		return false;
	}

	if (!initVulkan()) {
		std::cerr << "Failed to initialise Vulkan." << std::endl;
		return false;
	}

	if (!initSwapchain()) {
		std::cerr << "Failed to create swapchain." << std::endl;
		return false;
	}
	_isInitialized = true;

	return true;
}

bool Engine::initVulkan()
{
	// create the vulkan instance with vk-bootstrap

	vkb::InstanceBuilder instanceBuilder;

	auto instanceReturn = instanceBuilder.set_app_name("SRTV_Engine")
		.set_engine_name("SRTV_Engine")
		.request_validation_layers(useValidationLayers)				// use validation layers
		.use_default_debug_messenger()				// TODO : change for custom logger ?
		.require_api_version(1, 3, 0)
		.build();

	// simple error checking and helpful error messages
	if (!instanceReturn) {
		std::cerr << "Failed to create Vulkan instance. Error: " << instanceReturn.error().message() << std::endl;
		return false;
	}
	vkb::Instance vkbInstance = instanceReturn.value();

	// get the VkInstance handle used in the rest of a vulkan application
	_instance = vkbInstance.instance;
	_debugMessenger = vkbInstance.debug_messenger;

	// create VkSurface from SDL window

	SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

	// select a physical device (GPU) that is suitable for the engine

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

	if (!physicalDeviceSelectorReturn) {
		std::cerr << "Failed to select Vulkan Physical Device. Error: " << physicalDeviceSelectorReturn.error().message() << std::endl;
		if (physicalDeviceSelectorReturn.error() == vkb::PhysicalDeviceError::no_suitable_device) {
			const auto& detailed_reasons = physicalDeviceSelectorReturn.detailed_failure_reasons();
			if (!detailed_reasons.empty()) {
				std::cerr << "GPU Selection failure reasons: " << std::endl;
				for (const std::string& reason : detailed_reasons) {
					std::cerr << reason << std::endl;
				}
			}
		}
		return false;
	}
	vkb::PhysicalDevice VkbPhysicalDevice = physicalDeviceSelectorReturn.value();

	// optional extensions
	//bool supported = phys_device.enable_extension_if_present("VK_KHR_timeline_semaphore");
	//if (supported) {
		// allows easy feedback whether an extension is supported or not.
	//}

	// get the VkPhysicalDevice handle used in the rest of a vulkan application
	_chosenGPU = VkbPhysicalDevice.physical_device;

	// create the vulkan device with vk-bootstrap

	vkb::DeviceBuilder deviceBuilder{VkbPhysicalDevice};

	// automatically propagate needed data from instance & physical device
	auto deviceReturn = deviceBuilder.build();
	if (!deviceReturn) {
		std::cerr << "Failed to create Vulkan device. Error: " << deviceReturn.error().message() << std::endl;
		return false;
	}
	vkb::Device vkbDevice = deviceReturn.value();
	// get the VkDevice handle used in the rest of a vulkan application
	_device = vkbDevice.device;

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
			fmt::print("Key pressed : {}\n", SDL_GetKeyName(e.key.key));
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
			//SDL_Log("Unhandled Event!");
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
