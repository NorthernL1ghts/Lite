#pragma once

#include "Lite/Assets/Texture.h"
#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Lite {

	enum class ScenePlayback
	{
		Stopped,
		Playing,
		Paused
	};

	enum class MeshType
	{
		Quad,
		Triangle,
		Sprite
	};

	struct TransformComponent
	{
		Transform Local;
	};

	struct CameraComponent
	{
		float Size = 2.0f;
		float Near = -1.0f;
		float Far = 1.0f;
		bool Primary = false;
	};

	struct MeshComponent
	{
		MeshType Type = MeshType::Quad;
	};

	struct MaterialComponent
	{
		std::string Shader = "Batch";
		Vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
		Vec4 Colors[4] {};
		bool UseVertexColors = false;
		Vec2 Tiling { 1.0f, 1.0f };
		std::string TexturePath;
		Ref<Texture> Texture;
	};

	struct SpinComponent
	{
		float Rate = 0.0f;
	};

	class Scene;

	class LITE_API Entity
	{
	public:
		Entity() = default;
		Entity(Scene* scene, uint32_t id);

		bool IsValid() const;
		explicit operator bool() const { return IsValid(); }

		uint32_t GetId() const { return m_Id; }
		std::string GetName() const;
		void SetName(std::string name);

		bool HasTransform();
		bool HasCamera();
		bool HasMesh();
		bool HasMaterial();
		bool HasSpin();

		TransformComponent* GetTransform();
		CameraComponent* GetCamera();
		MeshComponent* GetMesh();
		MaterialComponent* GetMaterial();
		SpinComponent* GetSpin();

		TransformComponent& AddTransform();
		CameraComponent& AddCamera();
		MeshComponent& AddMesh();
		MaterialComponent& AddMaterial();
		SpinComponent& AddSpin();

	private:
		Scene* m_Scene = nullptr;
		uint32_t m_Id = 0;
	};

	class LITE_API Scene
	{
	public:
		explicit Scene(std::string name);
		~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		const std::string& GetName() const { return m_Name; }
		void SetName(std::string name) { m_Name = std::move(name); }
		const std::string& GetPath() const { return m_Path; }
		ScenePlayback GetPlayback() const { return m_Playback; }

		Entity CreateEntity(std::string name);
		Entity Find(std::string_view name);
		Entity GetEntity(uint32_t id);
		Entity GetPrimaryCamera();
		std::vector<Entity> GetEntities();
		void SetPrimaryCamera(uint32_t id);

		void Create();
		void Load();
		void Play();
		void Pause();
		void Stop();
		void Update(float seconds);
		void Render() const;

		bool Save();
		bool SaveAs(std::string_view path);
		static Scope<Scene> Open(std::string_view path);
		static std::vector<std::string> List();

		bool IsLoaded() const { return m_Loaded; }
		bool IsPlaying() const { return m_Playback == ScenePlayback::Playing; }

		static void SetActive(Scene* scene);
		static Scene* GetActive();

	private:
		friend class Entity;

		enum class Section
		{
			None,
			Legacy,
			Transform,
			Camera,
			Mesh,
			Material,
			Spin
		};

		struct Record
		{
			uint32_t Id = 0;
			std::string Name;
			std::optional<TransformComponent> Transform;
			std::optional<CameraComponent> Camera;
			std::optional<MeshComponent> Mesh;
			std::optional<MaterialComponent> Material;
			std::optional<SpinComponent> Spin;
		};

		Record* FindRecord(uint32_t id);
		const Record* FindRecord(uint32_t id) const;
		bool Read(std::istream& input);
		void Write(std::ostream& output) const;
		void ResolveTextures();

		std::string m_Name;
		std::string m_Path;
		std::vector<Record> m_Records;
		uint32_t m_NextId = 1;
		bool m_Loaded = false;
		ScenePlayback m_Playback = ScenePlayback::Stopped;
	};

}
