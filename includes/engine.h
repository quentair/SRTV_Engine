#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include <SDL3/SDL.h>

namespace srtv_engine
{

class FrameData {
public :

	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

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
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;
	void initSwapchain();
	void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();

	// frame
	FrameData _frames[FRAME_OVERLAP];
	int _frameNumber{ 0 };
	FrameData& getCurrentFrame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

	// commands
	void init_commands();

	// queue
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
};

} // namespace srtv_engine