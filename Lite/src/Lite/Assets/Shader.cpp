#include "Shader.h"

#include "Lite/Core/Logger.h"
#include "Lite/Renderer/Vulkan/VulkanUtils.h"

#include <cctype>
#include <fstream>

namespace {

	Lite::ShaderStage StageFromPath(const std::filesystem::path& path)
	{
		std::string name = path.filename().string();
		for (char& character : name)
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

		if (name.find(".vert") != std::string::npos)
			return Lite::ShaderStage::Vertex;
		if (name.find(".frag") != std::string::npos)
			return Lite::ShaderStage::Fragment;

		return Lite::ShaderStage::Unknown;
	}

	std::vector<uint32_t> ReadSpirv(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			LITE_ERROR("Failed to open shader {}", path.string());
			return {};
		}

		auto size = static_cast<std::streamsize>(file.tellg());
		if (size <= 0 || static_cast<size_t>(size) % sizeof(uint32_t) != 0)
		{
			LITE_ERROR("Shader {} is not valid SPIR-V", path.string());
			return {};
		}

		std::vector<uint32_t> code(static_cast<size_t>(size) / sizeof(uint32_t));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(code.data()), size);
		if (!file)
		{
			LITE_ERROR("Failed to read shader {}", path.string());
			return {};
		}

		if (code[0] != 0x07230203u)
		{
			LITE_ERROR("Shader {} is missing the SPIR-V header", path.string());
			return {};
		}

		return code;
	}

}

namespace Lite {

	bool Shader::LoadFromFile(const std::filesystem::path& path)
	{
		m_Code = ReadSpirv(path);
		if (m_Code.empty())
			return false;

		m_Stage = StageFromPath(path);
		SetIdentity(path.generic_string(), path.filename().string());
		m_Loaded = true;
		return true;
	}

	VkShaderModule Shader::CreateModule(VkDevice device) const
	{
		if (m_Code.empty())
			return VK_NULL_HANDLE;

		VkShaderModuleCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = m_Code.size() * sizeof(uint32_t);
		info.pCode = m_Code.data();

		VkShaderModule shader = VK_NULL_HANDLE;
		if (!CheckVk(vkCreateShaderModule(device, &info, nullptr, &shader), "create shader module"))
			return VK_NULL_HANDLE;

		return shader;
	}

	std::shared_ptr<Asset> ShaderHandler::Load(const std::filesystem::path& path)
	{
		auto shader = std::make_shared<Shader>();
		if (!shader->LoadFromFile(path))
			return nullptr;

		return shader;
	}

}
