#include "RenderContext.h"

#include "Lite/Platform/Vulkan/VulkanContext.h"

namespace Lite {

	std::unique_ptr<RenderContext> RenderContext::Create()
	{
		return std::make_unique<VulkanContext>();
	}

}
