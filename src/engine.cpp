#include "engine.h"

#include "error_checker.h"
#include "engine_logger.h"
#include "pipeline.h"
#include "shader.h"
#include "push_constant.h"
#include "parameters.h"

#include <SDL3/SDL_vulkan.h>

#include <fmt/core.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <thread>
#include <chrono>
#include <algorithm>

namespace srtv_engine {

void Engine::init()
{
	createSdlWindow();

	initVulkan();

	initAllocator();

	initSwapchain();

	initDrawImage();

	initBuffers();

	initCommands();

	initSyncStructures();

	initDescriptors();

	initPipelines();

	initImgui();

	initWorld();

	initWorldGenerationThread();

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

	// starting time
	auto currentTime = std::chrono::high_resolution_clock::now();

	// main loop
	while (!quit)
	{
		// do not draw if we are minimized
		if (_stopRendering) {
			// throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// start time of actual frame
		auto newTime = std::chrono::high_resolution_clock::now();
		// duration of precedent frame (start time of this frame - start time of precedent frame)
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		// update precedent frame start time for next iteration
		currentTime = newTime;

		while (SDL_PollEvent(&e)) {

			if (e.type == SDL_EVENT_KEY_DOWN) {
				//ENGINE_LOG_DEBUG(fmt::format("Key pressed : {}", SDL_GetKeyName(e.key.key)));
				if (e.key.key == SDLK_CAPSLOCK) {
					SDL_SetWindowRelativeMouseMode(_window, _relativeMode = !_relativeMode);
				}
			}
			else if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
			else if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
				_stopRendering = true;
			}
			else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
				_stopRendering = false;
			}

			// send SDL event to imgui for handling
			ImGui_ImplSDL3_ProcessEvent(&e);
		}

		// update camera by scanning keyboard input and mouse position
		_cameraController.update(frameTime);

		// put mouse cursor to cente of the screen
		if(_relativeMode)
			SDL_WarpMouseInWindow(_window, _windowExtent.width / 2.f, _windowExtent.height / 2.f);

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		// IMGUI window
		//ImGui::ShowDemoWindow();

		ImGui::Text("Simple ray traced voxel engine");
		ImGui::Spacing();

		ImGui::SeparatorText("Player datas");
		ImGui::Text("Player position : x = %0.2f ; y = %0.2f ; z = %0.2f", _cameraController._cameraPosition.x, _cameraController._cameraPosition.y, _cameraController._cameraPosition.z);

		ImGui::SeparatorText("Graphics parameters");

		int newViewDistance = _worldRenderingData._viewDistance;
		int newLodDistance2x2x2 = _lodDistance2x2x2;
		if (ImGui::InputInt("View distance", &newViewDistance, 1, 1)) {
			newViewDistance = std::clamp(newViewDistance, 1, MAX_VIEW_DISTANCE);
			changeViewDistance(newViewDistance);
		}
		if (ImGui::InputInt("LOD 2x2x2 distance", &newLodDistance2x2x2, 1, 1)) {
			newLodDistance2x2x2 = std::clamp(newLodDistance2x2x2, 0, newViewDistance);
			changeLodDistance2x2x2(newLodDistance2x2x2);
		}

		//make imgui calculate internal draw structures
		ImGui::Render();

		draw();

		if (_resize) {
			ENGINE_LOG_TRACE("Resize window...");
			resizeSwapchain();
		}
	}
}

void Engine::cleanup()
{

	ENGINE_LOG_TRACE("Cleanup engine...");

	// destroy in reversed creation order
	if (_isInitialized) {
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		// end the different threads
		_terminateWorldGenerationThread = true;
		_worldGenerationThread.join();

		// destroy ImGui implementation before destroying its allocated descriptors and pools
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();

		for (auto& frame : _frames) {
			vkDestroyCommandPool(_device, frame._commandPool, nullptr);

			vkDestroyFence(_device, frame._renderFence, nullptr);
			vkDestroySemaphore(_device, frame._acquireImageSemaphore, nullptr);

			// flush and destroy the frame-bound resources
			frame._frameResourceDeletor.flush(_allocator, _device);
		}

		// flush and destroy the global resources
		_mainResourceDeletor.flush(_allocator, _device);

		destroySwapchain();

		// destroy VMA allocator
		vmaDestroyAllocator(_allocator);

		vkDestroyDevice(_device, nullptr);

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);

		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}

	ENGINE_LOG_TRACE("Engine cleanup went fine");
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
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);

	if (!_window) {
		ENGINE_LOG_FATAL(fmt::format("Detected SDL fatal error: {}", SDL_GetError()));
	}

	// hide mouse cursor and constrain it to the window by default
	SDL_SetWindowRelativeMouseMode(_window, _relativeMode);
}

void Engine::initImgui()
{
	// create descriptor pool for ImGUI
	VkDescriptorPool imguiPool;
	{

		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
		};
		VkDescriptorPoolCreateInfo imguiPoolInfo = {};
		imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		imguiPoolInfo.maxSets = 0;
		for (VkDescriptorPoolSize& poolSize : poolSizes)
			imguiPoolInfo.maxSets += poolSize.descriptorCount;
		imguiPoolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
		imguiPoolInfo.pPoolSizes = poolSizes;
		CHECK_VK_FATAL_ERROR(vkCreateDescriptorPool(_device, &imguiPoolInfo, nullptr, &imguiPool));
	}

	ImGui::CreateContext();

	// setup Dear ImGui style
	ImGui::StyleColorsDark();

	// setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForVulkan(_window);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.ApiVersion = VK_API_VERSION_1_3;
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.QueueFamily = _graphicsQueueFamily;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = _swapchain.numberOfImages();
	init_info.ImageCount = _swapchain.numberOfImages();
	init_info.UseDynamicRendering = true;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	// dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	auto temp = _swapchain.getImageFormat();
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &temp;
	
	ImGui_ImplVulkan_Init(&init_info);

	// register ImGUI descrptor pool for further deletion
	_mainResourceDeletor.registerDescriptorPool(imguiPool);
}

void Engine::changeViewDistance(int& newViewDistance)
{
	_worldRenderingData.changeViewDistance(newViewDistance);
	_worldGenerator.changeViewDistance(newViewDistance);
}

void Engine::changeLodDistance2x2x2(int& newLodDistance2x2x2)
{
	_lodDistance2x2x2 = newLodDistance2x2x2;
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

	// wait for GPU to finish any ongoing operation
	vkDeviceWaitIdle(_device);

	destroySwapchain();

	int w, h;
	SDL_GetWindowSize(_window, &w, &h);
	_windowExtent.width = w;
	_windowExtent.height = h;

	_swapchain.create(_windowExtent.width, _windowExtent.height);

	_resize = false;
}

void Engine::destroySwapchain()
{
	_swapchain.destroy();
}

void Engine::initAllocator()
{
	ENGINE_LOG_TRACE("Building VMA allocator...");

	VmaAllocatorCreateInfo vmaAllocatorCreateInfo = {};
	vmaAllocatorCreateInfo.instance = _instance;
	vmaAllocatorCreateInfo.physicalDevice = _chosenGPU;
	vmaAllocatorCreateInfo.device = _device;
	vmaAllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

	CHECK_VK_FATAL_ERROR(vmaCreateAllocator(&vmaAllocatorCreateInfo, &_allocator));

	ENGINE_LOG_TRACE("VMA allocator creation went fine");
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

void Engine::initDrawImage()
{
	ENGINE_LOG_TRACE("Allocate draw image with VMA...");

	// draw image size will match the window
	VkExtent3D drawImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	// hardcoding the draw format to 32 bit float
	_drawImage._imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage._imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo allocationInfo = {};
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkImageCreateInfo imageCreateInfo = image::defaultImageCreateInfo(_drawImage._imageFormat, drawImageUsages, _drawImage._imageExtent);

	// allocate the draw image with VMA
	CHECK_VK_FATAL_ERROR(vmaCreateImage(_allocator, &imageCreateInfo, &allocationInfo, &_drawImage._image, &_drawImage._allocation, nullptr));

	// build a image-view for the new image to use for rendering
	VkImageViewCreateInfo imageViewCreateInfo = image::defaultImageviewCreateInfo(_drawImage._imageFormat, _drawImage._image, VK_IMAGE_ASPECT_COLOR_BIT);

	CHECK_VK_FATAL_ERROR(vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &_drawImage._imageView));

	// register in main resource deletor
	_mainResourceDeletor.registerImage(&_drawImage);

	ENGINE_LOG_TRACE("Draw image allocation went fine");
}

void Engine::initSyncStructures()
{
	// create semaphores and fences for our frames (number of frames in flight)
	// 1 fence to control when the gpu has finished rendering the frame
	// 1 semaphores to syncronize rendering with swapchain
	// we want the fence to start signalled so we don't wait on it on the first frame

	// also create one render semaphore per swapchain images to synchronize queues submit
	// when we signal this render semaphore on a queue submit, the presentation can happen and the fence is signaled, so the CPU can continue and draw the next frame
	// but NOTHING guarantees that the presentation is finished when we acquire the next image from our swapchain
	// this means that the next queue submit can use ressources that are still used by the precedent frame's presentation operation (like the pWaitSemaphores)
	// because by indexing rendering semaphores only by the index of frame in flight, we assume wich render semaphore we will use before actually getting the corresponding image from the swapchain
	// ie : "I just submitted image in flight 0, so image in flight 1 render semaphore will be available"
	// -> wrong because image in flight 1 render semaphore may still be used in presentation queue, and the image we acquired was using render semaphore from image in flight 0, but we take the one from image in flight 1 anyway, causing a bug
	// by allocating rendering semaphores based on the number of frames in the swapchain, and not the number of frames in flight, we can avoid this issue
	// so now we have : "I just submitted image in flight 0, so for image in flight 1, I grab the next available swapchain image, and because we aquired it, we can be sure that it has passed presentation operation and the render semaphore (and all ressources used by presentation) associated to it is truely available"

	ENGINE_LOG_TRACE("Building frame dependent semaphore and fence...");

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

	ENGINE_LOG_TRACE("Frame dependent semaphore and fence creation went fine");
}

void Engine::draw()
{
	// wait for precedent associated frame in our in-flight frames (so 2 frame before) to be drawn (1 second timeout)
	CHECK_VK_ERROR(vkWaitForFences(_device, 1, &getCurrentFrame()._renderFence, true, 1000000000));
	CHECK_VK_ERROR(vkResetFences(_device, 1, &getCurrentFrame()._renderFence));

	// readback of SSBO of precedent asscociated frame to prepare the next frame datas
	readbackBuffers(_frameNumber);

	getCurrentFrame()._frameResourceDeletor.flush(_allocator, _device);

	// acquire next image (by index) from the swapchain (1 second timeout)
	// use semaphore to make sure we have an image from the swapchain to draw onto for the next operations
	// the semaphore will be signaled when we can use the image
	uint32_t swapchainImageIndex;
	if (_swapchain.getNextImage(getCurrentFrame()._acquireImageSemaphore, &swapchainImageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
		// the out of date error may signal that the window was resized
		//  so we stop rendering and handle it
		_resize = true;
		return;
	}

	// retrieve the render semaphore corresponding to the swapchain image we retrieved
	VkSemaphore renderSemaphore = _swapchain.getRenderSemaphores()[swapchainImageIndex];

	// commands recording with command buffer

	VkCommandBuffer commandBuffer = getCurrentFrame()._commandBuffer;

	// reset the command buffer to begin recording again (fence usage makes this operation safe because the precedent associated frame was drawn)
	CHECK_VK_ERROR(vkResetCommandBuffer(commandBuffer, 0));

	// we will use this command buffer only once, so we set this flag in the creation info
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	_drawExtent.width = std::min(_swapchain.getExtent().width, _drawImage._imageExtent.width) * _renderScale;
	_drawExtent.height = std::min(_swapchain.getExtent().height, _drawImage._imageExtent.height) * _renderScale;

	// begin command buffer recording
	CHECK_VK_ERROR(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	// commands that we record in the buffer

	// transition our main draw image into general layout so we can write into it
	// we will overwrite it all so we dont care about what was the older layout
	image::transitionImage(commandBuffer, _drawImage._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// draw the background
	//drawBackground(commandBuffer);

	drawWorld(commandBuffer);

	// transition the draw image and the swapchain image into their correct transfer layouts
	image::transitionImage(commandBuffer, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	image::transitionImage(commandBuffer, _swapchain.getImageAt(swapchainImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	image::copyImageToImage(commandBuffer, _drawImage._image, _swapchain.getImageAt(swapchainImageIndex), _drawExtent, _swapchain.getExtent());

	// set swapchain image to present layout so we can show it on the screen
	image::transitionImage(commandBuffer, _swapchain.getImageAt(swapchainImageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	//draw imgui into the swapchain image
	drawImgui(commandBuffer, _swapchain.getImageViewAt(swapchainImageIndex));

	// set swapchain image layout to Present so we can draw it
	image::transitionImage(commandBuffer, _swapchain.getImageAt(swapchainImageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	auto presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
	CHECK_VK_ERROR(presentResult);

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		// the out of date error may signal that the window was resized, so we handle it
		_resize = true;
	}

	// update the number of frame
	_frameNumber = (_frameNumber+1) % FRAME_OVERLAP;
}

void Engine::drawBackground(VkCommandBuffer commandBuffer)
{
	//make a clear-color from frame number. This will flash every frame
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(_frameNumber));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	//VkImageSubresourceRange clearRange = image::createDefaultImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	//vkCmdClearColorImage(commandBuffer, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipelineLayout, 0, 1, &_drawImageDescriptorSet, 0, nullptr);

	ComputeShaderPushConstant pc{};
	pc._cameraPosition = glm::vec4(_cameraController._cameraPosition, 1);
	pc._cameraFront = glm::vec4(_cameraController._cameraFront, 0);
	pc._cameraUp = glm::vec4(_cameraController._cameraUp, 0);
	pc._cameraRight = glm::vec4(_cameraController._cameraRight, 0);
	pc._viewDistance = MAX_VIEW_DISTANCE;
	pc._lodDistance2x2x2 = _lodDistance2x2x2;

	vkCmdPushConstants(commandBuffer, _computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeShaderPushConstant), &pc);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(commandBuffer, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

void Engine::drawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
{
	VkClearValue* clear = nullptr;

	VkRenderingAttachmentInfo colorAttachment = {};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.pNext = nullptr;
	colorAttachment.imageView = targetImageView;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	if (clear) {
		colorAttachment.clearValue = *clear;
	}

	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.pNext = nullptr;

	renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, _swapchain.getExtent()};
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachment;
	renderInfo.pDepthAttachment = nullptr;
	renderInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(commandBuffer, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRendering(commandBuffer);
}

void Engine::drawWorld(VkCommandBuffer commandBuffer)
{
	// update chunks to render
	
	_worldRenderingData.resetChunks();

	// lock generated region vector access and read it, we will write on it on the world generator thread after the read operation
	{
		const std::lock_guard<std::mutex> lock(_world._registeredRegionsMutex);
		_worldRenderingData.loadRegions(_world._registeredRegions, _cameraController._cameraPosition);
	}

	// write into each world generation SSBO
	worldgen::WorldGpuData* worldDataSSBO = (worldgen::WorldGpuData*)getCurrentFrame()._worldDataBufferAllocInfo.pMappedData;
	uint32_t* viewDistanceGridSSBO = (uint32_t*)getCurrentFrame()._viewDistanceGridAllocInfo.pMappedData;
	worldgen::ChunksColumnGpuData* chunksColumnDataSSBO = (worldgen::ChunksColumnGpuData*)getCurrentFrame()._chunksDataBufferAllocInfo.pMappedData;

	*worldDataSSBO = _worldRenderingData._worldData;

	for (int i = 0; i < _worldRenderingData._viewDistanceGrid.size(); i++)
	{
		viewDistanceGridSSBO[i] = _worldRenderingData._viewDistanceGrid[i];
	}

	for (int i = 0; i < _worldRenderingData._chunksDataColumns.size(); i++)
	{
		chunksColumnDataSSBO[i] = _worldRenderingData._chunksDataColumns[i];
	}
	
	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipelineLayout, 0, 1, &_drawImageDescriptorSet, 0, nullptr);

	// same for SSBOs (set at set 1)
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipelineLayout, 1, 1, &getCurrentFrame()._worldgenDescriptorSet, 0, nullptr);

	ComputeShaderPushConstant pc{};
	pc._cameraPosition = glm::vec4(_cameraController._cameraPosition, 1);
	pc._cameraFront = glm::vec4(_cameraController._cameraFront, 0);
	pc._cameraUp = glm::vec4(_cameraController._cameraUp, 0);
	pc._cameraRight = glm::vec4(_cameraController._cameraRight, 0);
	pc._viewDistance = MAX_VIEW_DISTANCE;
	pc._lodDistance2x2x2 = _lodDistance2x2x2;

	vkCmdPushConstants(commandBuffer, _computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeShaderPushConstant), &pc);

	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(commandBuffer, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);

}

void Engine::initDescriptors()
{
	ENGINE_LOG_TRACE("Building Vulkan Descriptor Pool for the compute stage...");

	// first, create a descriptor pool
	
	// our descriptor pool can allocate up to (ratios * number of sets) of each descriptor type (here, 1 * number of sets storage image)
	std::vector<PoolSizeRatio> ratios = {
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},

		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
	};

	// our descriptor pool can allocate up to 10 sets (so 10 storage image in total)
	_globalDescriptorAllocator.init(_device, 10, ratios);

	ENGINE_LOG_TRACE("Vulkan Descriptor Pool creation went fine");

	ENGINE_LOG_TRACE("Building Vulkan Descriptor Layout for the compute stage...");

	// create the descriptor set layout for our compute draw (it needs to reflect our pool, so only storage image)
	{
		DescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // at set 0 binding 0 in the shader, we must have a storage image
		_drawImageDescriptorSetLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT); // the layout is made for compute shader stage
	}
	// with a layout of 1 storage image binding, we can allocate up to 10 sets from our pool

	{
		DescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // at set 1 binding 0 in the shader, we must have a storage buffer
		builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // at set 1 binding 1 in the shader, we must have a storage buffer
		builder.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // at set 1 binding 2 in the shader, we must have a storage buffer
		_worldgenDescriptorSetLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT); // the layout is made for compute shader stage
	}

	ENGINE_LOG_TRACE("Vulkan Descriptor Layout creation went fine");

	ENGINE_LOG_TRACE("Allocating and binding Vulkan Descriptor Set for the compute stage...");

	// allocate the descriptor sets from our pool following our descriptor sets layouts
	_drawImageDescriptorSet = _globalDescriptorAllocator.allocate(_device, _drawImageDescriptorSetLayout);

	for (auto& frame : _frames) {
		frame._worldgenDescriptorSet = _globalDescriptorAllocator.allocate(_device, _worldgenDescriptorSetLayout);
	}

	// bind our draw image to the descriptor set (the image must a storage image situated in the GENERAL layout)
	DescriptorWriter imageWriter;
	imageWriter.writeImage(0, _drawImage._imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	imageWriter.updateSet(_device, _drawImageDescriptorSet);

	// bind our storage buffer the same manner
	for (auto& frame : _frames) {
		DescriptorWriter ssboWriter;
		ssboWriter.writeBuffer(0, frame._worldDataBuffer._buffer, sizeof(worldgen::WorldGpuData), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		ssboWriter.writeBuffer(1, frame._viewDistanceGridBuffer._buffer, sizeof(uint32_t) * MAX_VIEW_GRID_SIZE, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		ssboWriter.writeBuffer(2, frame._chunksDataBuffer._buffer, sizeof(worldgen::ChunksColumnGpuData) * MAX_VIEW_GRID_SIZE, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		ssboWriter.updateSet(_device, frame._worldgenDescriptorSet);
	}

	ENGINE_LOG_TRACE("Vulkan Descriptor Set allocation and binding went fine");

	// register global descriptor pool and drawImage descriptor layout for further deletion
	_mainResourceDeletor.registerDescriptorAllocatorGrowable(&_globalDescriptorAllocator);
	_mainResourceDeletor.registerDescriptorSetLayout(_drawImageDescriptorSetLayout);

	// same for ssbo descriptor layout
	_mainResourceDeletor.registerDescriptorSetLayout(_worldgenDescriptorSetLayout);
}

void Engine::initPipelines()
{
	initComputePipelines();
}

void Engine::initComputePipelines()
{
	ENGINE_LOG_TRACE("Building Vulkan Compute Pipeline...");

	// create a pipeline layout for our compute stage draw

	VkDescriptorSetLayout setLayouts[] = { _drawImageDescriptorSetLayout, _worldgenDescriptorSetLayout };

	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {};
	computePipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computePipelineLayoutCreateInfo.pNext = nullptr;
	computePipelineLayoutCreateInfo.pSetLayouts = setLayouts;
	computePipelineLayoutCreateInfo.setLayoutCount = 2;

	// push constants
	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputeShaderPushConstant);
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	computePipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;
	computePipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	CHECK_VK_FATAL_ERROR(vkCreatePipelineLayout(_device, &computePipelineLayoutCreateInfo, nullptr, &_computePipelineLayout));

	// load shader code

	VkShaderModule computeDrawShader;
	if (!loadShaderModule("../../shaders/voxel_test.comp.spv", _device, &computeDrawShader))
	{
		ENGINE_LOG_ERROR("Error when building the compute shader \n");
	}

	// create a pipeline for our compute stage draw

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.pNext = nullptr;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageCreateInfo.module = computeDrawShader;
	shaderStageCreateInfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = _computePipelineLayout;
	computePipelineCreateInfo.stage = shaderStageCreateInfo;

	CHECK_VK_FATAL_ERROR(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_computePipeline));

	// once the pipeline is built, shaders can be safely destroyed
	vkDestroyShaderModule(_device, computeDrawShader, nullptr);

	// regiser pipeline layout and pipeline for further deletion (delete pipelin layout first, then the pipeline)
	_mainResourceDeletor.registerPipelineLayout(_computePipelineLayout);
	_mainResourceDeletor.registerPipeline(_computePipeline);

	ENGINE_LOG_TRACE("Vulkan Compute Pipeline creation went fine");
}

void Engine::initBuffers()
{
	ENGINE_LOG_TRACE("Building Vulkan Buffers for GPU datas...");

	// world generation buffers send to GPU
	for (auto& frame : _frames) {
		VkBufferCreateInfo worldBufferCreateInfo = buffer::defaultBufferCreateInfo(sizeof(worldgen::WorldGpuData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		VkBufferCreateInfo viewDistanceGridBufferCreateInfo = buffer::defaultBufferCreateInfo(sizeof(uint32_t) * MAX_VIEW_GRID_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		VkBufferCreateInfo chunksDataBufferCreateInfo = buffer::defaultBufferCreateInfo(sizeof(worldgen::ChunksColumnGpuData) * MAX_VIEW_GRID_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		// buffers for data written by or transferred from the GPU that we want to read back on the CPU
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		// allocate buffers with VMA
		CHECK_VK_FATAL_ERROR(vmaCreateBuffer(_allocator, &worldBufferCreateInfo, &vmaallocInfo,
			&frame._worldDataBuffer._buffer,
			&frame._worldDataBuffer._allocation,
			&frame._worldDataBufferAllocInfo));

		CHECK_VK_FATAL_ERROR(vmaCreateBuffer(_allocator, &viewDistanceGridBufferCreateInfo, &vmaallocInfo,
			&frame._viewDistanceGridBuffer._buffer,
			&frame._viewDistanceGridBuffer._allocation,
			& frame._viewDistanceGridAllocInfo));

		CHECK_VK_FATAL_ERROR(vmaCreateBuffer(_allocator, &chunksDataBufferCreateInfo, &vmaallocInfo,
			&frame._chunksDataBuffer._buffer,
			&frame._chunksDataBuffer._allocation,
			&frame._chunksDataBufferAllocInfo));

		// register buffers for furhter deletion
		_mainResourceDeletor.registerBuffer(&frame._worldDataBuffer);
		_mainResourceDeletor.registerBuffer(&frame._viewDistanceGridBuffer);
		_mainResourceDeletor.registerBuffer(&frame._chunksDataBuffer);
	}

	ENGINE_LOG_TRACE("Creation of Vulkan Buffers went fine");
}

void Engine::readbackBuffers(int frameNumber)
{
	// read world data SSBO to retrieve brickmaps load queue
	worldgen::WorldGpuData* worldDataSSBO = (worldgen::WorldGpuData*)getFrame(frameNumber)._worldDataBufferAllocInfo.pMappedData;

	uint32_t numberOfBricksToLoad = std::min(worldDataSSBO->_brickLoadQueueCount, uint32_t(BRICKMAP_QUEUE_SIZE));

	if (numberOfBricksToLoad == 0)
		return;

	// retrieve each unloaded brick
	// their indices are available in the _chunksDataColumns, but not their data
	// so we can retrieve the indices from the _chunksDataColumns and use it to parse our chunks that were cached for the GPU (_gpuLoadedChunks)
	for (int i = 0; i < numberOfBricksToLoad; i++) {
		glm::ivec4 brickPositions = worldDataSSBO->_brickLoadQueue[i]; // from shaders, we know that it represents ivec3(chunkIndex, columnIndex, brickPosition)

		// retrieve brick index from _chunksDataColumns (contains infos about position of the brick's data inside the chunk vectors, state of the brick...)
		int intersectionData = _worldRenderingData._chunksDataColumns[brickPositions.x]._chunksInColumn[brickPositions.y]._brickmaps[brickPositions.z];
		int brickIndex = intersectionData & BRICKMAP_INDEX_BITS;

		// retieve chunk from cache, giving access to its bricks datas
		int loadedChunkIndex = brickPositions.x + brickPositions.y * (MAX_VIEW_DISTANCE * 2 + 1) * (MAX_VIEW_DISTANCE * 2 + 1);
		worldgen::BrickChunk* chunk = _worldRenderingData._gpuLoadedChunks[loadedChunkIndex];

		// stage loaded brick in CPU side, it will be upload in the SSBO on next frame (in drawWorld function)

		// get chunk data in the chunk column given its y position
		worldgen::ChunkGpuData* chunkData = &_worldRenderingData._chunksDataColumns[brickPositions.x]._chunksInColumn[brickPositions.y];

		// skip if already loaded by one of the precedent frames (because we have 2 frames with 2 different SSBOS that are not synced together on GPU side, but share the same CPU side datas)
		if (chunkData->_brickmaps[brickPositions.z] & BRICKMAP_LOADED_BIT)
			continue;

		// update chunk data that will be send to GPU
		chunkData->_brickmaps[brickPositions.z] = BRICKMAP_LOADED_BIT | (intersectionData & BRICKMAP_LOD_BITS) | (intersectionData & BRICKMAP_INDEX_BITS);
		chunkData->_brickmapsData[brickPositions.z] = chunk->_brickmaps[brickIndex];
	}

	// reset worlddata queues for next readback
	_worldRenderingData.resetQueues();
}

void Engine::initWorld()
{
	ENGINE_LOG_TRACE("Generating starting region...");

	// start time of world generation
	auto beginTime = std::chrono::high_resolution_clock::now();

	glm::ivec3 startingRegionPos{0,0,0};
	srtv_engine::worldgen::WorldRegion* startingRegion;

	_cameraController._cameraPosition = glm::vec3{ 0.f, 257.0f, 0.f };
	_world.generateRegion(_cameraController._cameraPosition.x, _cameraController._cameraPosition.y, _cameraController._cameraPosition.z, startingRegionPos.x, startingRegionPos.y, startingRegionPos.z, _worldGenerator, _terminateWorldGenerationThread);

	startingRegion = _world.getRegion(startingRegionPos.x, startingRegionPos.y, startingRegionPos.z);
	_worldRenderingData.loadChunks(*startingRegion, _cameraController._cameraPosition);

	// end time of world generation
	auto endtime = std::chrono::high_resolution_clock::now();
	// duration of world generation
	float worldGenTime = std::chrono::duration<float, std::chrono::seconds::period>(endtime - beginTime).count();

	ENGINE_LOG_TRACE(fmt::format("Starting region generation completed (took {} seconds)", worldGenTime));
}

void Engine::initWorldGenerationThread()
{
	ENGINE_LOG_TRACE("Starting world generation thread...");

	_worldGenerationThread = std::thread{ &Engine::worldGenerationThread, this };
}

void Engine::worldGenerationThread()
{
	// continuously generate regions around player position and save them in a vector
	// this vector will be used to retrieve the generated chunks from the different regions, and then send them to the GPU if they are in viewing range
	while (!(_terminateWorldGenerationThread.load())) {
		_world.generateRegionsAroundPlayerPosition(_cameraController._cameraPosition.x, _cameraController._cameraPosition.y, _cameraController._cameraPosition.z, _worldGenerator, _terminateWorldGenerationThread);
	}
	ENGINE_LOG_TRACE("Terminating world generation thread...");
}

} // namespace srtv_engine