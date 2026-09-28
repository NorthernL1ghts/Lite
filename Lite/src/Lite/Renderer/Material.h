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
		[[nodiscard]] static Ref<Material> Create(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, const Ref<Texture>& texture = {});

		~Material();

		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;
		Material(Material&&) = delete;
		Material& operator=(Material&&) = delete;

		void SetColor(const Vec4& color);
		const Vec4& GetColor() const { return m_Color; }
		void SetTiling(const Vec2& tiling);
		const Vec2& GetTiling() const { return m_Tiling; }
		void SetUniform(const MaterialUniform& uniform);
		MaterialUniform GetUniform() const;
		void SetTexture(const Ref<Texture>& texture);
		const Ref<Texture>& GetTexture() const { return m_Texture; }

		void Bind();

	private:
		Material() = default;

		bool Init(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, const Ref<Texture>& texture);
		void Destroy();

		ShaderProgram m_Shader;
		UniformBuffer m_Uniforms;
		Ref<Texture> m_Texture;
		bool m_UsesTexture = false;
		Vec4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vec2 m_Tiling = { 1.0f, 1.0f };
	};

}
