#include "VulkanInstance.h"

#include <GLFW/glfw3.h>

#include <cstring>
#include <vector>

namespace {

	constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void*)
	{
		if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			LITE_ERROR("Vulkan: {}", data->pMessage);
		else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			LITE_WARN("Vulkan: {}", data->pMessage);

		return VK_FALSE;
	}

	bool HasValidationLayer()
	{
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		std::vector<VkLayerProperties> layers(count);
		vkEnumerateInstanceLayerProperties(&count, layers.data());

		for (const auto& layer : layers)
		{
			if (std::strcmp(layer.layerName, ValidationLayer) == 0)
				return true;
		}

		return false;
	}

	void FillDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& info)
	{
		info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.pfnUserCallback = DebugCallback;
	}

}

namespace Lite {

	bool VulkanInstance::Create()
	{
#ifndef NDEBUG
		m_Validation = HasValidationLayer();
		if (!m_Validation)
			LITE_WARN("Vulkan validation layers were not found");
#endif

		uint32_t glfwCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);
		if (!glfwExtensions)
		{
			LITE_ERROR("GLFW did not report Vulkan instance extensions");
			return false;
		}

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwCount);
		if (m_Validation)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		VkApplicationInfo application {};
		application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application.pApplicationName = "Lite";
		application.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		application.pEngineName = "Lite";
		application.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		application.apiVersion = VK_API_VERSION_1_3;

		VkDebugUtilsMessengerCreateInfoEXT debugInfo {};
		FillDebugInfo(debugInfo);

		VkInstanceCreateInfo instanceInfo {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &application;
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();
		if (m_Validation)
		{
			instanceInfo.enabledLayerCount = 1;
			instanceInfo.ppEnabledLayerNames = &ValidationLayer;
			instanceInfo.pNext = &debugInfo;
		}

		if (!CheckVk(vkCreateInstance(&instanceInfo, nullptr, &m_Instance), "create instance"))
			return false;

		if (!m_Validation)
		{
			LITE_INFO("Vulkan instance created");
			return true;
		}

		auto createDebug = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
		if (!createDebug || !CheckVk(createDebug(m_Instance, &debugInfo, nullptr, &m_DebugMessenger), "create debug messenger"))
			return false;

		LITE_INFO("Vulkan instance created");
		return true;
	}

	void VulkanInstance::Destroy()
	{
		if (m_DebugMessenger)
		{
			auto destroyDebug = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (destroyDebug)
				destroyDebug(m_Instance, m_DebugMessenger, nullptr);
			m_DebugMessenger = VK_NULL_HANDLE;
		}

		if (m_Instance)
		{
			vkDestroyInstance(m_Instance, nullptr);
			m_Instance = VK_NULL_HANDLE;
		}
	}

}
