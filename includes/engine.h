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
#include "world_generator.h"

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
	VkExtent2D _windowExtent{ 1920 , 1080 };
	SDL_Window* _window;
	bool _relativeMode{ true };
	void createSdlWindow();

	// surface
	VkSurfaceKHR _surface;

	// IMGUI
	void initImgui();

	// IMGUI callbacks
	void changeViewDistance(int& newViewDistance);
	void changeLodDistance2x2x2(int& newLodDistance2x2x2);

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
	int _dirtyFrameChunks[FRAME_OVERLAP]; // indicates if all the chunks on our frames are out of date
	void markFramesChunksAsDirty() { for (int i = 0; i < FRAME_OVERLAP; i++) _dirtyFrameChunks[i] = 1; }

	// commands
	VkCommandPool _transferCommandPool;
	VkCommandBuffer _transferCommandBuffer;
	void initCommands();

	// queues
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	VkQueue _transferQueue;
	uint32_t _transferQueueFamily;
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
	void copyBuffers(int frameNumber);
	void readbackBuffers(int frameNumber);

	// world CPU side
	worldgen::World _world{};
	void initWorld();
	void initWorldGenerationThread();
	void worldGenerationThread();

	// world data for rendering
	worldgen::GpuWorld _worldRenderingData{};
	uint32_t _lodDistance2x2x2 = BASE_LOD_DISTANCE_2x2x2;
	glm::vec2 _gpuViewgridAnchorWorldPos; // world position of the bottom left chunk of our viewgrid

	// world generation
	worldgen::WorldGenerator _worldGenerator{};
};

} // namespace srtv_engine