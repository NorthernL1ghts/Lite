#include <Lite/Renderer/Renderer2D.h>

#include <Lite/Renderer/Material.h>
#include <Lite/Renderer/RendererAPI.h>
#include <Lite/Renderer/Resources/VertexArray.h>
#include <Lite/Renderer/Resources/VertexLayout.h>
#include <Lite/Renderer/Vulkan/VulkanSync.h>

#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Profile/Profiler.h>
#include <Lite/Renderer/MeshShape.h>

#include <array>
#include <span>
#include <vector>

namespace Lite {

	namespace {

		constexpr uint32_t MaxTextures = 16;
		constexpr uint32_t MaxQuads = 4096;
		constexpr uint32_t MaxVertices = MaxQuads * 4;
		constexpr uint32_t MaxIndices = MaxQuads * 6;

		struct BatchVertex
		{
			float x = 0.0f;
			float y = 0.0f;
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			float u = 0.0f;
			float v = 0.0f;
			float textureIndex = 0.0f;
		};

		ShaderLibrary s_ShaderLibrary;
		std::vector<BatchVertex> s_Vertices;
		std::vector<uint16_t> s_Indices;
		Ref<Texture> s_White;
		std::array<Ref<Texture>, MaxTextures> s_Slots;
		uint32_t s_SlotCount = 0;
		Ref<Material> s_Material;
		std::array<VertexArray, VulkanSync::FramesInFlight> s_Meshes;
		std::array<VkDescriptorSet, VulkanSync::FramesInFlight> s_TextureSets {};
		VkDescriptorSetLayout s_TextureLayout = VK_NULL_HANDLE;
		VkDescriptorPool s_TexturePool = VK_NULL_HANDLE;
		VkDevice s_Device = VK_NULL_HANDLE;
		bool s_Ready = false;

		VertexLayout BatchLayout()
		{
			VertexLayout layout;
			layout.Stride = sizeof(BatchVertex);
			layout.Count = 4;
			layout.Attributes[0] = { 0, VertexFormat::Float2, 0 };
			layout.Attributes[1] = { 1, VertexFormat::Float4, sizeof(float) * 2 };
			layout.Attributes[2] = { 2, VertexFormat::Float2, sizeof(float) * 6 };
			layout.Attributes[3] = { 3, VertexFormat::Float, sizeof(float) * 8 };
			return layout;
		}

		void ResetSlots()
		{
			s_Slots.fill({});
			s_Slots[0] = s_White;
			s_SlotCount = s_White ? 1u : 0u;
		}

		bool CreateTextureArray()
		{
			s_Device = Renderer::GetDevice();

			VkDescriptorSetLayoutBinding binding {};
			binding.binding = 0;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.descriptorCount = MaxTextures;
			binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutCreateInfo layoutInfo {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = &binding;
			if (!CheckVk(vkCreateDescriptorSetLayout(s_Device, &layoutInfo, nullptr, &s_TextureLayout), "create batch texture layout"))
				return false;

			VkDescriptorPoolSize poolSize {};
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = MaxTextures * VulkanSync::FramesInFlight;

			VkDescriptorPoolCreateInfo poolInfo {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.maxSets = VulkanSync::FramesInFlight;
			poolInfo.poolSizeCount = 1;
			poolInfo.pPoolSizes = &poolSize;
			if (!CheckVk(vkCreateDescriptorPool(s_Device, &poolInfo, nullptr, &s_TexturePool), "create batch texture pool"))
				return false;

			std::array<VkDescriptorSetLayout, VulkanSync::FramesInFlight> layouts {};
			layouts.fill(s_TextureLayout);

			VkDescriptorSetAllocateInfo allocateInfo {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = s_TexturePool;
			allocateInfo.descriptorSetCount = VulkanSync::FramesInFlight;
			allocateInfo.pSetLayouts = layouts.data();
			return CheckVk(vkAllocateDescriptorSets(s_Device, &allocateInfo, s_TextureSets.data()), "allocate batch texture sets");
		}

		void DestroyTextureArray()
		{
			if (!s_Device)
				return;

			vkDeviceWaitIdle(s_Device);
			if (s_TexturePool)
				vkDestroyDescriptorPool(s_Device, s_TexturePool, nullptr);
			if (s_TextureLayout)
				vkDestroyDescriptorSetLayout(s_Device, s_TextureLayout, nullptr);

			s_TextureSets.fill(VK_NULL_HANDLE);
			s_TexturePool = VK_NULL_HANDLE;
			s_TextureLayout = VK_NULL_HANDLE;
			s_Device = VK_NULL_HANDLE;
		}

		void WriteTextures(uint32_t frame)
		{
			std::array<VkDescriptorImageInfo, MaxTextures> images {};
			for (uint32_t index = 0; index < MaxTextures; ++index)
			{
				const Ref<Texture>& slot = index < s_SlotCount && s_Slots[index] ? s_Slots[index] : s_White;
				images[index].sampler = slot->GetSampler();
				images[index].imageView = slot->GetView();
				images[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			VkWriteDescriptorSet write {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = s_TextureSets[frame];
			write.dstBinding = 0;
			write.descriptorCount = MaxTextures;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.pImageInfo = images.data();
			vkUpdateDescriptorSets(s_Device, 1, &write, 0, nullptr);
		}

		int FindSlot(const Texture* texture)
		{
			for (uint32_t index = 0; index < s_SlotCount; ++index)
			{
				if (s_Slots[index].get() == texture)
					return static_cast<int>(index);
			}

			return -1;
		}

		float TextureSlot(const Ref<Texture>& texture)
		{
			const Texture* next = texture ? texture.get() : s_White.get();
			int found = FindSlot(next);
			if (found >= 0)
				return static_cast<float>(found);

			if (s_SlotCount >= MaxTextures)
			{
				Renderer2D::Flush();
				found = FindSlot(next);
				if (found >= 0)
					return static_cast<float>(found);
			}

			if (s_SlotCount >= MaxTextures)
				return 0.0f;

			s_Slots[s_SlotCount] = texture ? texture : s_White;
			return static_cast<float>(s_SlotCount++);
		}

		struct SpriteBasis
		{
			Vec2 Origin {};
			Vec2 AxisX {};
			Vec2 AxisY {};
		};

		SpriteBasis Basis(const Transform& transform)
		{
			const Quat rotation = transform.Rotation.Normalized();
			const Vec3 axisX = rotation.RotateUnit({ transform.Scale.x, 0.0f, 0.0f });
			const Vec3 axisY = rotation.RotateUnit({ 0.0f, transform.Scale.y, 0.0f });
			return {
				{ transform.Position.x, transform.Position.y },
				{ axisX.x, axisX.y },
				{ axisY.x, axisY.y }
			};
		}

		Vec2 Place(const SpriteBasis& basis, Vec2 corner)
		{
			return {
				basis.Origin.x + basis.AxisX.x * corner.x + basis.AxisY.x * corner.y,
				basis.Origin.y + basis.AxisX.y * corner.x + basis.AxisY.y * corner.y
			};
		}

		void Push(const Transform& transform, std::span<const Vec2> corners, std::span<const Vec4> colors, std::span<const Vec2> uvs, float textureIndex)
		{
			const SpriteBasis basis = Basis(transform);
			const uint16_t base = static_cast<uint16_t>(s_Vertices.size());
			for (size_t corner = 0; corner < corners.size(); ++corner)
			{
				const Vec2 world = Place(basis, corners[corner]);
				const Vec4& color = colors[corner];
				const Vec2 uv = corner < uvs.size() ? uvs[corner] : Vec2 {};
				s_Vertices.push_back({
					world.x, world.y,
					color.x, color.y, color.z, color.w,
					uv.x, uv.y,
					textureIndex
				});
			}

			if (corners.size() == 4)
			{
				s_Indices.push_back(base + 0);
				s_Indices.push_back(base + 1);
				s_Indices.push_back(base + 2);
				s_Indices.push_back(base + 2);
				s_Indices.push_back(base + 3);
				s_Indices.push_back(base + 0);
				return;
			}

			for (size_t corner = 0; corner < corners.size(); ++corner)
				s_Indices.push_back(base + static_cast<uint16_t>(corner));
		}

	}

	void Renderer2D::Init(void* window)
	{
		s_ShaderLibrary.Clear();
		s_Ready = false;
		Renderer::Init(window);
		if (!Renderer::GetDevice())
			return;

		const uint8_t whitePixel[4] = { 255, 255, 255, 255 };
		s_White = Texture::Create(1, 1, whitePixel);
		ResetSlots();

		Ref<Shader> vertex = s_ShaderLibrary.Load("assets/shaders/Batch.vert.spv");
		Ref<Shader> fragment = s_ShaderLibrary.Load("assets/shaders/Batch.frag.spv");
		if (!s_White || !vertex || !fragment || !CreateTextureArray())
		{
			LITE_ERROR("Renderer2D batch shaders or textures failed to load");
			DestroyTextureArray();
			s_White.reset();
			return;
		}

		std::vector<BatchVertex> vertices(MaxVertices);
		std::vector<uint16_t> indices(MaxIndices);
		for (VertexArray& mesh : s_Meshes)
		{
			if (!mesh.Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(BatchVertex)), indices.data(), MaxIndices))
			{
				LITE_ERROR("Renderer2D batch mesh failed to allocate");
				DestroyTextureArray();
				return;
			}
		}

		s_Material = Material::Create(*vertex, *fragment, BatchLayout(), true, {}, s_TextureLayout);
		if (!s_Material)
		{
			DestroyTextureArray();
			return;
		}

		s_Vertices.reserve(MaxVertices);
		s_Indices.reserve(MaxIndices);
		s_Ready = true;
		LITE_INFO("Renderer2D batch ready ({} quads, {} textures)", MaxQuads, MaxTextures);
	}

	ShaderLibrary& Renderer2D::GetShaderLibrary()
	{
		return s_ShaderLibrary;
	}

	void Renderer2D::SetViewProjection(const Mat4& viewProjection)
	{
		Renderer::SetViewProjection(viewProjection);
	}

	void Renderer2D::Shutdown()
	{
		s_Ready = false;
		s_Vertices.clear();
		s_Indices.clear();
		s_Slots.fill({});
		s_SlotCount = 0;
		s_Material.reset();
		s_White.reset();
		for (VertexArray& mesh : s_Meshes)
			mesh.Destroy();
		DestroyTextureArray();
		s_ShaderLibrary.Clear();
		Renderer::Shutdown();
	}

	void Renderer2D::BeginFrame()
	{
		Renderer::BeginFrame();
		s_Vertices.clear();
		s_Indices.clear();
		ResetSlots();
	}

	void Renderer2D::DrawQuad(const Transform& transform, const Vec4& color)
	{
		if (!s_Ready || !Renderer::IsFrameActive())
			return;

		if (s_Vertices.size() + 4 > MaxVertices || s_Indices.size() + 6 > MaxIndices)
			Flush();

		const Vec4 colors[4] = { color, color, color, color };
		Push(transform, kQuadCorners, colors, {}, TextureSlot({}));
	}

	void Renderer2D::DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& tint)
	{
		const Vec4 colors[4] = { tint, tint, tint, tint };
		DrawQuad(transform, texture, tiling, colors[0], colors[1], colors[2], colors[3]);
	}

	void Renderer2D::DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& bottomLeft, const Vec4& bottomRight, const Vec4& topRight, const Vec4& topLeft)
	{
		if (!s_Ready || !Renderer::IsFrameActive())
			return;

		if (s_Vertices.size() + 4 > MaxVertices || s_Indices.size() + 6 > MaxIndices)
			Flush();

		const Vec4 colors[4] = { bottomLeft, bottomRight, topRight, topLeft };
		const Vec2 uvs[4] = {
			{ 0.0f, 0.0f },
			{ tiling.x, 0.0f },
			{ tiling.x, tiling.y },
			{ 0.0f, tiling.y }
		};
		Push(transform, kQuadCorners, colors, uvs, TextureSlot(texture));
	}

	void Renderer2D::DrawTriangle(const Transform& transform, const Vec4& first, const Vec4& second, const Vec4& third)
	{
		if (!s_Ready || !Renderer::IsFrameActive())
			return;

		if (s_Vertices.size() + 3 > MaxVertices || s_Indices.size() + 3 > MaxIndices)
			Flush();

		const Vec4 colors[3] = { first, second, third };
		Push(transform, kTriangleCorners, colors, {}, TextureSlot({}));
	}

	void Renderer2D::Flush()
	{
		LITE_PROFILE_SCOPE("Batch Flush");
		if (!s_Ready || !Renderer::IsFrameActive() || s_Vertices.empty() || !s_Material)
			return;

		uint32_t frame = Renderer::GetFrameIndex();
		if (frame >= s_Meshes.size())
			return;

		if (!s_Meshes[frame].Upload(
			s_Vertices.data(),
			static_cast<uint32_t>(s_Vertices.size() * sizeof(BatchVertex)),
			s_Indices.data(),
			static_cast<uint32_t>(s_Indices.size())))
			return;

		WriteTextures(frame);
		Renderer::SetModel(Mat4::Identity());
		s_Material->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		s_Material->SetTiling({ 1.0f, 1.0f });
		s_Material->Bind();
		vkCmdBindDescriptorSets(Renderer::GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, s_Material->GetLayout(), 2, 1, &s_TextureSets[frame], 0, nullptr);
		RendererAPI::DrawIndexed(s_Meshes[frame]);

		s_Vertices.clear();
		s_Indices.clear();
		ResetSlots();
	}

	void Renderer2D::EndFrame()
	{
		if (Renderer::IsFrameActive())
			Flush();

		Renderer::EndFrame();
	}

	void Renderer2D::OnResize(int width, int height)
	{
		Renderer::OnResize(width, height);
	}

	bool Renderer2D::IsFrameActive()
	{
		return Renderer::IsFrameActive();
	}

}
