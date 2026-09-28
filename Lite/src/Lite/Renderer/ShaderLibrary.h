#pragma once

#include "Lite/Assets/Shader.h"
#include "Lite/Core/Base.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace Lite {

	class LITE_API ShaderLibrary
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);

		[[nodiscard]] Ref<Shader> Load(std::string_view path);
		[[nodiscard]] Ref<Shader> Load(const std::string& name, std::string_view path);
		[[nodiscard]] Ref<Shader> Get(std::string_view name) const;
		bool Exists(std::string_view name) const;

		void Clear();

	private:
		std::string Key(std::string_view name) const;
		std::string NameFromPath(std::string_view path) const;

		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}
