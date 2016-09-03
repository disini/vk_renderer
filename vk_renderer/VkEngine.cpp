#include "VkEngine.h"


void VkEngine::init(int argc, char** argv)
{
	config.parseCmdLineArgs(argc, argv);
}

void VkEngine::run()
{
	initWindow();
	initVulkan();
	initCamera();
	setupInputCallbacks();
	mainLoop();
}

void VkEngine::initCamera()
{
	scene.getCamera().frame.origin = CAMERA_POSITION;
	scene.getCamera().frame = Frame::lookAtFrame(CAMERA_POSITION, CAMERA_TARGET, CAMERA_UP);
	scene.getCamera().aspectRatio = swapchainExtent.width / (float) swapchainExtent.height;
	scene.getCamera().target = CAMERA_TARGET;
	scene.getCamera().fovy = CAMERA_FOVY;
	scene.getCamera().movement = STILL;
}

void VkEngine::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(config.resolution.x, config.resolution.y, APPLICATION_NAME, nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, onWindowResized);
}

void VkEngine::setupInputCallbacks()
{
	glfwSetKeyCallback(window, keyboardFunc);
	glfwSetMouseButtonCallback(window, mouseKeyFunc);
	glfwSetCursorPosCallback(window, cursorPosFunc);
}

void VkEngine::keyboardFunc(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

void VkEngine::mouseKeyFunc(GLFWwindow* window, int button, int action, int mods)
{
	Camera camera = VkEngine::getInstance()->getScene().getCamera();
	if (action == GLFW_RELEASE)
	{
		camera.movement = STILL;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (mods & GLFW_MOD_ALT)
		{
			camera.movement = ROTATE;
		}
		else
		{
			camera.movement = ROTATE_AROUND_TARGET;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		camera.movement = PAN;
	}
}

void VkEngine::cursorPosFunc(GLFWwindow* window, double xpos, double ypos)
{
	glm::ivec2 oldMousePos = VkEngine::getInstance()->getOldMousePos();
	glm::vec2 deltaPos = { xpos - oldMousePos.x, ypos - oldMousePos.y };
	VkEngine::getInstance()->setOldMousePos({ xpos, ypos });

	Scene scene = VkEngine::getInstance()->getScene();
	switch (scene.getCamera().movement)
	{
	case ROTATE:
		scene.getCamera().rotate(CAMERA_ROT_SCALE * deltaPos);
		break;
	case ROTATE_AROUND_TARGET:
		scene.getCamera().rotateAroundTarget(CAMERA_ROT_SCALE * deltaPos);
		break;
	case PAN:
		scene.getCamera().pan(CAMERA_PAN_SCALE * deltaPos);
		break;
	case ZOOM:
		scene.getCamera().zoom(CAMERA_DOLLY_SCALE * deltaPos.y);
		break;
	case STILL:
	default:
		break;
	}
}



void VkEngine::onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	VkEngine* app = reinterpret_cast<VkEngine*>(glfwGetWindowUserPointer(window));
	app->recreateSwapchain();
}

void VkEngine::createInstance()
{
	if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Requested validation layers are not available!");
	}

	auto extensions = getRequiredExtensions();

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = APPLICATION_NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
}

void VkEngine::setupDebugCallback()
{
	if (!ENABLE_VALIDATION_LAYERS)
		return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT) debugCallback;

	VK_CHECK(CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback));
}

void VkEngine::createSurface()
{
	VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}

void VkEngine::initVulkan()
{
	createInstance();
	setupDebugCallback();
	createSurface();
	selectPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createCommandPool();
	createDescriptorPool();
	createSemaphores();

	initRenderPasses();

	/*
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandBuffers();
	*/
}

void VkEngine::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		updateBufferData();
		draw();
	}

	vkDeviceWaitIdle(device);
}

void VkEngine::selectPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const VkPhysicalDevice& device : devices)
	{
		if (isDeviceSuitable(device, surface))
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

void VkEngine::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilyIndices(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentationFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentationFamily, 0, &presentationQueue);
}

void VkEngine::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentationMode(swapChainSupport.presentationModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, config.resolution.x, config.resolution.y);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilyIndices(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { (uint32_t) indices.graphicsFamily, (uint32_t) indices.presentationFamily };

	if (indices.graphicsFamily != indices.presentationFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	VkSwapchainKHR oldSwapchain = swapchain;
	createInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR newSwapChain;
	VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapChain));

	*&swapchain = newSwapChain;

	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, (VkImage*)swapchainImages.data());

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;
}

void VkEngine::createImageViews()
{
	swapchainImageViews.resize(swapchainImages.size(), VkObjWrapper<VkImageView> {device, vkDestroyImageView});

	for (uint32_t i = 0; i < swapchainImages.size(); i++)
	{
		createImageView(device, swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, swapchainImageViews[i]);
	}
}

void VkEngine::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
}

void VkEngine::draw()
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		device, 
		swapchain, 
		std::numeric_limits<uint64_t>::max(), 
		imageAvailableSemaphore, 
		VK_NULL_HANDLE, 
		&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain();
		return;
	}
	else
	{
		VK_CHECK(result);
	}
	
	for (RenderPass renderPass : renderPasses)
	{
		VkResult result = renderPass.run();

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			recreateSwapchain();
		}
		else
		{
			VK_CHECK(result);
		}
	}
}

void VkEngine::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
	VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
}

void VkEngine::recreateSwapchain()
{
	vkDeviceWaitIdle(device);

	createSwapChain();
	createImageViews();

	initRenderPasses();
	
	/*
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createCommandBuffers();
	*/
}

void VkEngine::updateBufferData()
{
	scene.getCamera().updateMatrices();

	for (RenderPass renderPass : renderPasses)
	{
		renderPass.updateData();
	}
}

void VkEngine::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
}

void VkEngine::initRenderPasses()
{

}

VK_WRAP(VkDevice) VkEngine::getDevice() { return device; }
VK_WRAP(VkPhysicalDevice) VkEngine::getPhysicalDevice() { return physicalDevice; }
VK_WRAP(VkCommandPool) VkEngine::getCommandPool() { return commandPool; }
VkQueue VkEngine::getGraphicsQueue() { return graphicsQueue; }
VkQueue VkEngine::getPresentationQueue() { return presentationQueue; }
VkFormat VkEngine::getSwapchainImageFormat() { return swapchainImageFormat; }
VkExtent2D VkEngine::getSwapchainExtent() { return swapchainExtent; }
VK_VEC_WRAP(VkImageView)& VkEngine::getSwapchainImageViews() { return swapchainImageViews; }
size_t VkEngine::getNumSwapchains() { return swapchainImageViews.size(); }
Scene& VkEngine::getScene() { return scene; }
VK_WRAP(VkSwapchainKHR)& VkEngine::getSwapchain() { return swapchain; }
VK_WRAP(VkSemaphore)& VkEngine::getImageAvailableSemaphore() { return imageAvailableSemaphore; }
VK_WRAP(VkSemaphore)& VkEngine::getRenderFinishedSemaphore() { return renderFinishedSemaphore; }
VK_WRAP(VkDescriptorPool)& VkEngine::getDescriptorPool() { return descriptorPool; }