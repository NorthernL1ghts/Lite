#include "ShaderLibrary.h"

#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Core/Assert.h"
#include "Lite/Core/Logger.h"
#include "Lite/Core/String.h"

#include <filesystem>

namespace Lite {

	std::string ShaderLibrary::Key(std::string_view name) const
	{
		return ToLower(name);
	}

	std::string ShaderLibrary::NameFromPath(std::string_view path) const
	{
		return std::filesystem::path(path).stem().string();
	}

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		LITE_CORE_ASSERT(shader, "Cannot add a null shader");
		std::string key = Key(name);
		LITE_CORE_ASSERT(!m_Shaders.contains(key), "Shader {} is already in the library", name);
		m_Shaders.emplace(key, shader);
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		Add(shader->GetName(), shader);
	}

	Ref<Shader> ShaderLibrary::Load(std::string_view path)
	{
		return Load(NameFromPath(path), path);
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, std::string_view path)
	{
		std::string key = Key(name);
		if (auto found = m_Shaders.find(key); found != m_Shaders.end())
			return found->second;

		auto shader = AssetRegistry::Get().Load<Shader>(path);
		if (!shader)
		{
			LITE_ERROR("Shader library failed to load {}", path);
			return nullptr;
		}

		m_Shaders.emplace(key, shader);
		LITE_INFO("Shader library loaded {} ({})", name, path);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(std::string_view name) const
	{
		auto found = m_Shaders.find(Key(name));
		LITE_CORE_ASSERT(found != m_Shaders.end(), "Shader {} is not in the library", name);
		if (found == m_Shaders.end())
			return nullptr;

		return found->second;
	}

	bool ShaderLibrary::Exists(std::string_view name) const
	{
		return m_Shaders.contains(Key(name));
	}

	void ShaderLibrary::Clear()
	{
		m_Shaders.clear();
	}

}
