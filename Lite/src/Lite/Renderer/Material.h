#pragma once

#include "Lite/Assets/Texture.h"
#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "ShaderProgram.h"

namespace Lite {

	class Shader;

	class LITE_API Material
	{
	public:
		static Ref<Material> Create(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, const Ref<Texture>& texture = {});

		~Material();

		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;
		Material(Material&&) = delete;
		Material& operator=(Material&&) = delete;

		void SetColor(const Vec4& color);
		const Vec4& GetColor() const { return m_Color; }

		void Bind();

	private:
		Material() = default;

		bool Init(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, const Ref<Texture>& texture);
		void Destroy();

		ShaderProgram m_Shader;
		UniformBuffer m_Uniforms;
		Ref<Texture> m_Texture;
		Vec4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

}
