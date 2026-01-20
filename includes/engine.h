#pragma once

#include "frame_data.h"
#include "swapchain.h"
#include "resource_deletor.h"
#include "image.h"
#include "descriptor.h"
#include "camera.h"
#include "camera_control.h"
#include "world.h"
#include "gpu_world.h"

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

	inline bool isRunning() const { return _isInitialized; }

	void init();

	void initVulkan();

	void run();

	void cleanup();

  private:

	bool _isInitialized{ false };
	bool _stopRendering{ false };
	bool _resize{ false };

	// threads
	std::thread _worldGenerationThread;
	std::atomic<bool> _terminateWorldGenerationThread = false;

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
	bool _relativeMode{ true };
	void createSdlWindow();

	// surface
	VkSurfaceKHR _surface;

	// IMGUI
	void initImgui();

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
	FrameData& getFrame(int frameNumber) { return _frames[frameNumber]; }

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
	float _renderScale = 1.f;
	image::AllocatedImage _drawImage;

	// rendering
	void draw();
	void drawBackground(VkCommandBuffer commandBuffer);
	void drawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView);
	void drawWorld(VkCommandBuffer commandBuffer);

	// descriptors
	DescriptorAllocatorGrowable _globalDescriptorAllocator;
	VkDescriptorSetLayout _drawImageDescriptorSetLayout;
	VkDescriptorSetLayout _worldgenDescriptorSetLayout;
	VkDescriptorSet _drawImageDescriptorSet;
	void initDescriptors();

	// pipelines
	VkPipeline _computePipeline;
	VkPipelineLayout _computePipelineLayout;
	void initPipelines();
	void initComputePipelines();

	// camera
	Camera _mainCamera{};
	CameraControl _cameraController{_mainCamera};

	// buffers
	void initBuffers();
	void readbackBuffers(int frameNumber);

	// world CPU side
	worldgen::World _world{};
	void initWorld();
	void initWorldGenerationThread();
	void worldGenerationThread();

	// world data for rendering
	worldgen::GpuWorld _worldRenderingData{};
};

} // namespace srtv_engine