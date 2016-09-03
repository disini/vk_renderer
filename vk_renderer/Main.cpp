#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <iostream>
#include "VkEngine.h"

int main(int argc, char** argv)
{
	VkEngine* engine = new VkEngine();

	try
	{
		engine->init(argc, argv);
		engine->run();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	delete engine;

	return EXIT_SUCCESS;
}