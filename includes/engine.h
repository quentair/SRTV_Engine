#pragma once

#include "frame_data.h"
#include "swapchain.h"
#include "resource_deletor.h"
#include "image.h"
#include "descriptor.h"

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include <SDL3/SDL.h>

#include <vector>

namespace srtv_engine
{
	constexpr unsigned int FRAME_OVERLAP = 2;

class Engine {

public:

	Engine() = default;

	inline bool isRunning() { return _isInitialized; }

	void init();

	void initVulkan();

	void run();

	void cleanup();

  private:

	bool _isInitialized{ false };
	bool _stopRendering{ false };

	// instance
	bool _useValidationLayers{ true };
	VkInstance _instance;

	// logger
	VkDebugUtilsMessengerEXT _debugMessenger;

	// GPU and logical device (interface with GPU)
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	// window
	VkExtent2D _windowExtent{ 1700 , 900 };
	SDL_Window* _window;
	void createSdlWindow();

	// surface
	VkSurfaceKHR _surface;

	// swapchain
	Swapchain _swapchain;
	void initSwapchain();
	void resizeSwapchain();
	void destroySwapchain();

	// resource allocation
	VmaAllocator _allocator;
	void initAllocator();

	// resource deletion
	ResourceDeletor _mainResourceDeletor;

	// frame
	FrameData _frames[FRAME_OVERLAP];
	int _frameNumber{ 0 };
	FrameData& getCurrentFrame() { return _frames[_frameNumber]; }

	// commands
	void initCommands();

	// queue
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	void initDrawImage();

	// synchronisation structures
	void initSyncStructures();

	// rendering resources
	VkExtent2D _drawExtent;
	image::AllocatedImage _drawImage;

	// rendering
	void draw();
	void drawBackground(VkCommandBuffer commandBuffer);

	// descriptors
	DescriptorAllocatorGrowable _globalDescriptorAllocator;
	VkDescriptorSetLayout _drawImageDescriptorSetLayout;
	VkDescriptorSet _drawImageDescriptorSet;
	void initDescriptors();

	// pipelines
	VkPipeline _computePipeline;
	VkPipelineLayout _computePipelineLayout;
	void initPipelines();
	void initBackgroundPipelines();
};

} // namespace srtv_engine