#pragma once

#include <Lite/Assets/Texture.h>
#include <Lite/Core/Base.h>
#include <Lite/Math/Math.h>

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

	inline const char* PlaybackName(ScenePlayback playback)
	{
		switch (playback)
		{
			case ScenePlayback::Playing: return "playing";
			case ScenePlayback::Paused: return "paused";
			default: return "stopped";
		}
	}

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

	inline constexpr float kDefaultFieldOfView = 60.0f * (3.14159265f / 180.0f);
	inline constexpr const char* kDefaultShader = "Batch";

	enum class CameraProjection
	{
		Orthographic,
		Perspective
	};

	struct CameraComponent
	{
		CameraProjection Projection = CameraProjection::Orthographic;
		float Size = 2.0f;
		float FieldOfView = kDefaultFieldOfView;
		float Near = -1.0f;
		float Far = 1.0f;
		bool Primary = false;
	};

	inline Mat4 CameraProjectionMatrix(const CameraComponent& camera, float aspect)
	{
		if (camera.Projection == CameraProjection::Perspective)
		{
			float zNear = camera.Near > 0.0f ? camera.Near : 0.1f;
			float zFar = camera.Far > zNear ? camera.Far : zNear + 100.0f;
			float fov = camera.FieldOfView > 0.0f ? camera.FieldOfView : kDefaultFieldOfView;
			return Mat4::Perspective(fov, aspect, zNear, zFar);
		}

		float halfHeight = camera.Size * 0.5f;
		float halfWidth = halfHeight * aspect;
		return Mat4::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, camera.Near, camera.Far);
	}

	struct MeshComponent
	{
		MeshType Type = MeshType::Quad;
	};

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

	struct SpinComponent
	{
		float Rate = 0.0f;
	};

	enum class BodyType
	{
		Static,
		Kinematic,
		Dynamic
	};

	struct Rigidbody2DComponent
	{
		BodyType Type = BodyType::Dynamic;
		float Mass = 1.0f;
		float GravityScale = 1.0f;
		Vec2 LinearVelocity {};
		float AngularVelocity = 0.0f;
		bool FreezeRotation = false;
	};

	struct BoxCollider2DComponent
	{
		Vec2 Size { 1.0f, 1.0f };
		Vec2 Offset {};
		bool IsTrigger = false;
	};

	struct CircleCollider2DComponent
	{
		float Radius = 0.5f;
		Vec2 Offset {};
		bool IsTrigger = false;
	};

	struct SortingComponent
	{
		int Order = 0;
	};

	enum class ComponentId : uint8_t
	{
		Transform,
		Camera,
		Mesh,
		Material,
		Spin,
		Rigidbody2D,
		BoxCollider2D,
		CircleCollider2D,
		Sorting
	};

	template<typename T>
	struct ComponentInfo;

	template<> struct ComponentInfo<TransformComponent> { static constexpr ComponentId Id = ComponentId::Transform; };
	template<> struct ComponentInfo<CameraComponent> { static constexpr ComponentId Id = ComponentId::Camera; };
	template<> struct ComponentInfo<MeshComponent> { static constexpr ComponentId Id = ComponentId::Mesh; };
	template<> struct ComponentInfo<MaterialComponent> { static constexpr ComponentId Id = ComponentId::Material; };
	template<> struct ComponentInfo<SpinComponent> { static constexpr ComponentId Id = ComponentId::Spin; };
	template<> struct ComponentInfo<Rigidbody2DComponent> { static constexpr ComponentId Id = ComponentId::Rigidbody2D; };
	template<> struct ComponentInfo<BoxCollider2DComponent> { static constexpr ComponentId Id = ComponentId::BoxCollider2D; };
	template<> struct ComponentInfo<CircleCollider2DComponent> { static constexpr ComponentId Id = ComponentId::CircleCollider2D; };
	template<> struct ComponentInfo<SortingComponent> { static constexpr ComponentId Id = ComponentId::Sorting; };

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

		template<typename T>
		bool Has();

		template<typename T>
		T* Get();

		template<typename T>
		T& Add();

		template<typename T>
		void Remove();

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
		void Restart();
		void Update(float seconds);
		void Render() const;
		Mat4 ViewProjection(float aspect, const Transform& fallbackTransform, const CameraComponent& fallbackCamera) const;

		static void Close(Scope<Scene>& scene);

		bool Save();
		bool SaveAs(std::string_view path);
		static std::string Locate(std::string_view relativeToProject);
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
			Spin,
			Rigidbody2D,
			BoxCollider2D,
			CircleCollider2D,
			Sorting
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
			std::optional<Rigidbody2DComponent> Rigidbody2D;
			std::optional<BoxCollider2DComponent> BoxCollider2D;
			std::optional<CircleCollider2DComponent> CircleCollider2D;
			std::optional<SortingComponent> Sorting;
		};

		Record* FindRecord(uint32_t id);
		const Record* FindRecord(uint32_t id) const;
		const Record* FindPrimaryCameraRecord() const;
		void* AddComponent(ComponentId id, uint32_t entity);
		void* GetComponent(ComponentId id, uint32_t entity);
		void RemoveComponent(ComponentId id, uint32_t entity);
		bool Read(std::istream& input);
		void Write(std::ostream& output) const;
		void ResolveTextures();
		void StartPhysics();
		void StopPhysics();
		void SyncPhysics();
		void StepPhysics(float seconds);

		struct PhysicsStorage;
		PhysicsStorage* m_Physics = nullptr;

		std::string m_Name;
		std::string m_Path;
		std::vector<Record> m_Records;
		uint32_t m_NextId = 1;
		bool m_Loaded = false;
		ScenePlayback m_Playback = ScenePlayback::Stopped;
	};

	template<typename T>
	bool Entity::Has()
	{
		return Get<T>() != nullptr;
	}

	template<typename T>
	T* Entity::Get()
	{
		if (m_Scene == nullptr)
			return nullptr;
		return static_cast<T*>(m_Scene->GetComponent(ComponentInfo<T>::Id, m_Id));
	}

	template<typename T>
	T& Entity::Add()
	{
		return *static_cast<T*>(m_Scene->AddComponent(ComponentInfo<T>::Id, m_Id));
	}

	template<typename T>
	void Entity::Remove()
	{
		if (m_Scene != nullptr)
			m_Scene->RemoveComponent(ComponentInfo<T>::Id, m_Id);
	}

}
