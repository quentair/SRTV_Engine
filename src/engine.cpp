#include "engine.h"

#include "error_checker.h"
#include "engine_logger.h"
#include "image.h"

#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>

#include <thread>
#include <iostream>

namespace srtv_engine {

void Engine::init()
{
	createSdlWindow();

	initVulkan();

	initSwapchain();

	initCommands();

	initSyncStructures();

	_isInitialized = true;
}

void Engine::initVulkan()
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
		else {
			draw();
		}
	}
}

void Engine::cleanup()
{
	// destroy in reversed creation order
	if (_isInitialized) {
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		for (auto& frame : _frames) {
			vkDestroyCommandPool(_device, frame._commandPool, nullptr);

			vkDestroyFence(_device, frame._renderFence, nullptr);
			vkDestroySemaphore(_device, frame._acquireImageSemaphore, nullptr);
		}

		for (auto& semaphore : _swapchain.getRenderSemaphores()) {
			vkDestroySemaphore(_device, semaphore, nullptr);
		}

		destroySwapchain();

		vkDestroyDevice(_device, nullptr);

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);

		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}
}

void Engine::createSdlWindow()
{
	// initialise SDL and create a SDL window

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		ENGINE_LOG_FATAL(fmt::format("Detected SDL fatal error: {}", SDL_GetError()));
	}

	_window = SDL_CreateWindow("SRTV Engine",
		_windowExtent.width,
		_windowExtent.height,
		SDL_WINDOW_VULKAN
	);

	if (!_window) {
		ENGINE_LOG_FATAL(fmt::format("Detected SDL fatal error: {}", SDL_GetError()));
	}
}

void Engine::initSwapchain()
{
	// initialise and create swapchain

	_swapchain.init(_chosenGPU, _device, _surface);
	_swapchain.create(_windowExtent.width, _windowExtent.height);
}

void Engine::resizeSwapchain()
{
	// destroy out of date swapchain and create a new one with the right size

	vkDeviceWaitIdle(_device);

	destroySwapchain();

	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	_windowExtent.width = w;
	_windowExtent.height = h;

	_swapchain.create(_windowExtent.width, _windowExtent.height);

	//resize_requested = false;
}

void Engine::destroySwapchain()
{
	_swapchain.destroy();
}

void Engine::initCommands()
{
	// create a command pool for commands submitted to the graphics queue
	// we also want to reset individual command buffers if they are coming from the frames command pool

	ENGINE_LOG_TRACE("Building Vulkan Command Pool...");

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = _graphicsQueueFamily;

	ENGINE_LOG_TRACE("Vulkan Command Pool creation went fine");

	ENGINE_LOG_TRACE("Building Vulkan Commands (double buffering)...");

	for (auto& frame : _frames) {
		CHECK_VK_FATAL_ERROR(vkCreateCommandPool(
			_device,
			&commandPoolCreateInfo,
			nullptr,
			&frame._commandPool));

		// allocate the defaults command buffers that we will use for rendering

		VkCommandBufferAllocateInfo cmdAllocInfo = {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = frame._commandPool;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdAllocInfo.commandBufferCount = 1;

		CHECK_VK_FATAL_ERROR(vkAllocateCommandBuffers(
			_device,
			&cmdAllocInfo,
			&frame._commandBuffer));
	}

	ENGINE_LOG_TRACE("Vulkan Commands creation went fine");
}

void Engine::initSyncStructures()
{
	// create semaphores and fences for our frames (number of frames in flight)
	// 1 fence to control when the gpu has finished rendering the frame
	// 1 semaphores to syncronize rendering with swapchain
	// we want the fence to start signalled so we don't wait on it on the first frame

	// also create one semaphore per swapchain images to synchronize queues submit
	// this is because when we signal this semaphore on a queue submit, the presentation can happen and the fence is signaled, so the CPU can continue and draw the next frame
	// but NOTHING guarantees that the presentation is finished when we acquire the next image from our swapchain
	// this means that the next queue submit can use ressources that are still used by the precedent frame's presentation operation (like the pWaitSemaphores)
	// by allocating rendering semaphores based on the number of frames in the swapchain, and not the number of frames in flight, we can avoid this issue

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (auto& frame : _frames) {
		CHECK_VK_FATAL_ERROR(vkCreateFence(_device,
			&fenceCreateInfo,
			nullptr,
			&frame._renderFence));

		CHECK_VK_FATAL_ERROR(vkCreateSemaphore(
			_device,
			&semaphoreCreateInfo,
			nullptr,
			&frame._acquireImageSemaphore));
	}

	for (auto& semaphore : _swapchain.getRenderSemaphores()) {
		CHECK_VK_FATAL_ERROR(vkCreateSemaphore(
			_device,
			&semaphoreCreateInfo,
			nullptr,
			&semaphore));
	}
}

void Engine::draw()
{
	// wait for precedent frame to be drawn (1 second timeout)
	CHECK_VK_ERROR(vkWaitForFences(_device, 1, &getCurrentFrame()._renderFence, true, 1000000000));
	CHECK_VK_ERROR(vkResetFences(_device, 1, &getCurrentFrame()._renderFence));

	// acquire next image (by index) from the swapchain (1 second timeout)
	// use semaphore to make sure we have an image from the swapchain to draw onto for the next operations
	// the semaphore will be signaled when we can use the image
	uint32_t swapchainImageIndex;
	_swapchain.getNextImage(getCurrentFrame()._acquireImageSemaphore, &swapchainImageIndex);

	// retrieve the render semaphore corresponding to the swapchain image we retrieved
	VkSemaphore renderSemaphore = _swapchain.getRenderSemaphores()[swapchainImageIndex];

	// commands recording with command buffer

	VkCommandBuffer commandBuffer = getCurrentFrame()._commandBuffer;

	// reset the command buffer to begin recording again (fence usage makes this operation safe because the precedent frame was drawn)
	CHECK_VK_ERROR(vkResetCommandBuffer(commandBuffer, 0));

	// we will use this command buffer only once, so we set this flag in the creation info
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// begin command buffer recording
	CHECK_VK_ERROR(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	// commands that we record in the buffer

	// make the swapchain image into writeable mode before rendering
	image::transitionImage(commandBuffer, _swapchain.getImageAt(swapchainImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// make a clear-color from frame number. This will flash every frame.
	VkClearColorValue clearValue;
	clearValue = { { 0.0f, 0.0f, float(_frameNumber), 1.0f } };

	VkImageSubresourceRange clearRange = image::createImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	// clear image
	vkCmdClearColorImage(commandBuffer, _swapchain.getImageAt(swapchainImageIndex), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	// make the swapchain image into presentable mode
	image::transitionImage(commandBuffer, _swapchain.getImageAt(swapchainImageIndex), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// end the command buffer (can't add commands to it, but it can now be executed)
	CHECK_VK_ERROR(vkEndCommandBuffer(commandBuffer));

	// submit to queue

	// we want to wait on the _acquireImageSemaphore, because that semaphore is signaled when the swapchain gave us an image to draw onto
	// we also want to signal the renderSemaphore when submitted queue operations (placed in the defined command buffer) have completed execution
	VkCommandBufferSubmitInfo commandBufferSubmitInfo = {};
	commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	commandBufferSubmitInfo.pNext = nullptr;
	commandBufferSubmitInfo.commandBuffer = commandBuffer;
	commandBufferSubmitInfo.deviceMask = 0;

	VkSemaphoreSubmitInfo waitSemaphoresInfo = {};
	waitSemaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitSemaphoresInfo.pNext = nullptr;
	waitSemaphoresInfo.semaphore = getCurrentFrame()._acquireImageSemaphore;
	waitSemaphoresInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
	waitSemaphoresInfo.deviceIndex = 0;
	waitSemaphoresInfo.value = 1;

	VkSemaphoreSubmitInfo signalSemaphoresInfo = {};
	signalSemaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalSemaphoresInfo.pNext = nullptr;
	signalSemaphoresInfo.semaphore = renderSemaphore;
	signalSemaphoresInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
	signalSemaphoresInfo.deviceIndex = 0;
	signalSemaphoresInfo.value = 1;

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.pNext = nullptr;
	submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &waitSemaphoresInfo;
	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &signalSemaphoresInfo;
	submitInfo.signalSemaphoreInfoCount = 1;

	// submit command buffer to the queue and execute it
	// once the commands registered in the buffer finish execution, _renderFence will be signaled and the next draw() can proceed
	CHECK_VK_ERROR(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, getCurrentFrame()._renderFence));

	// present swapchain image

	// put the image we drawed onto into the window
	// we want to wait on the renderSemaphore, 
	// because it is necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = _swapchain.getSwapchain();
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

	CHECK_VK_ERROR(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	// update the number of frame
	_frameNumber = (_frameNumber+1) % FRAME_OVERLAP;
}

} // namespace srtv_engine