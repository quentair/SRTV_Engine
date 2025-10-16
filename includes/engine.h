#pragma once

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

#include <SDL3/SDL.h>

namespace srtv_engine
{

class Engine {

public:

	Engine() = default;

	inline bool isRunning() { return _isInitialized; }

	bool init();

	bool initVulkan();

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
	bool createSdlWindow();

	// surface
	VkSurfaceKHR _surface;

	// swapchain
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;
	bool initSwapchain();
	bool createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();
};

} // namespace srtv_engine