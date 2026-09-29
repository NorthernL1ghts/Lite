#pragma once

#include <Lite/Assets/Texture.h>
#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Component.h>

#include <string>

namespace Lite {

	inline constexpr const char* kDefaultShader = "Batch";

	struct MaterialComponent
	{
		std::string Shader = kDefaultShader;
		Vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
		Vec4 Colors[4] {};
		bool UseVertexColors = false;
		Vec2 Tiling { 1.0f, 1.0f };
		std::string TexturePath;
		Ref<Texture> Texture;
	};

	template<>
	struct ComponentTraits<MaterialComponent>
	{
		static constexpr ComponentId Id = ComponentId::Material;
	};

}
