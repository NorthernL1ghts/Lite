#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Components.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
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

	inline constexpr float kPhysicsStep = 1.0f / 60.0f;

	inline const char* PlaybackName(ScenePlayback playback)
	{
		switch (playback)
		{
			case ScenePlayback::Playing: return "playing";
			case ScenePlayback::Paused: return "paused";
			default: return "stopped";
		}
	}

	class Scene;
	class ComponentStorage;

	struct RayHit
	{
		uint32_t Entity = 0;
		Vec2 Point {};
		Vec2 Normal {};
	};

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
		bool Has() const;

		template<typename T>
		const T* Get() const;

		template<typename T>
		T* Get();

		template<typename T>
		T& Add();

		template<typename T>
		void Remove();

		Transform WorldTransform() const;

	private:
		Scene* m_Scene = nullptr;
		uint32_t m_Id = 0;
	};

	struct ComponentEntry
	{
		const char* Name = nullptr;
		const char* (*Label)(Entity entity) = nullptr;
		void (*Add)(Entity entity) = nullptr;
	};

	LITE_API const ComponentEntry* ComponentCatalog(size_t& count);

	class LITE_API Scene
	{
	public:
		explicit Scene(std::string name);
		~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		const std::string& GetName() const { return m_Name; }
		void SetName(std::string name);
		const std::string& GetPath() const { return m_Path; }
		bool IsDirty() const { return m_Dirty; }
		void MarkDirty();
		ScenePlayback GetPlayback() const { return m_Playback; }

		struct Plane
		{
			uint32_t Id = 0;
			int Order = 0;
			std::string Name;
		};

		uint32_t CreatePlane(std::string name);
		uint32_t AddPlane();
		uint32_t FindPlane(std::string_view name) const;
		bool SetPlaneName(uint32_t id, std::string name);
		void SetPlaneOrder(uint32_t id, int order);

		enum class PlaneRemove
		{
			Removed,
			MovedToWorld,
			HasEntities,
			LastPlane,
			Missing
		};

		PlaneRemove RemovePlane(uint32_t id);
		const std::vector<Plane>& GetPlanes() const { return m_Planes; }

		Entity CreateEntity(std::string name);
		Entity CreateEntity(std::string name, uint32_t plane);
		Entity DuplicateEntity(uint32_t id);

		enum class EntityDestroy
		{
			Missing,
			Removed,
			RemovedChildren,
			DetachedChildren
		};

		EntityDestroy DestroyEntity(uint32_t id, bool detachChildren);
		uint32_t GetParent(uint32_t id) const;
		bool SetParent(uint32_t id, uint32_t parent, bool keepWorld);
		std::vector<Entity> GetChildren(uint32_t id);
		int CountDescendants(uint32_t id) const;
		Transform WorldTransform(uint32_t id) const;
		std::string GetTag(uint32_t id) const;
		void SetTag(uint32_t id, std::string tag);
		std::vector<Entity> FindTagged(std::string_view tag);
		std::string GetPrefab(uint32_t id) const;
		void SetPrefab(uint32_t id, std::string path);
		bool SavePrefab(uint32_t id, const std::filesystem::path& path) const;
		Entity PlacePrefab(const std::filesystem::path& path, std::optional<Vec2> position);
		bool SaveMaterial(uint32_t id, const std::filesystem::path& path) const;
		bool ApplyMaterial(uint32_t id, const std::filesystem::path& path);
		Entity Find(std::string_view name);
		Entity GetEntity(uint32_t id);
		Entity GetPrimaryCamera();
		std::vector<Entity> GetEntities();
		std::vector<Entity> GetEntities(uint32_t plane);
		void SetPrimaryCamera(uint32_t id);

		void Create();
		void Load();
		void Play();
		void Pause();
		void Stop();
		void Restart();
		void RefreshPhysics(uint32_t entityId);
		void ApplyForce(uint32_t id, Vec2 force);
		void ApplyLinearImpulse(uint32_t id, Vec2 impulse);
		std::optional<RayHit> Raycast(Vec2 origin, Vec2 translation) const;
		void RequestScene(std::string_view path);
		void Update(float seconds);
		void Render() const;
		Mat4 ViewProjection(float aspect, const Transform& fallbackTransform, const CameraComponent& fallbackCamera) const;
		uint32_t Pick(const Mat4& viewProjection, float mouseX, float mouseY, float windowW, float windowH);
		uint32_t NextEntity(uint32_t id) const;

		struct TextureAssign
		{
			bool Applied = false;
			bool Loaded = false;
			std::string Path;
		};

		TextureAssign AssignTexture(uint32_t entityId, const std::string& path);

		struct SpritePlacement
		{
			uint32_t Entity = 0;
			bool Background = false;
			bool Loaded = false;
		};

		SpritePlacement PlaceSprite(const std::string& texturePath, Vec2 world, Vec2 viewCenter, float viewSize, float aspect);

		static void Close(Scope<Scene>& scene);

		std::string Snapshot() const;
		bool Restore(std::string_view document);

		bool Save();
		bool SaveAs(std::string_view path);
		static std::string Locate(std::string_view relativeToProject);
		static Scope<Scene> Open(std::string_view path);
		static std::vector<std::string> List();

		bool IsLoaded() const { return m_Loaded; }
		bool IsPlaying() const { return m_Playback == ScenePlayback::Playing; }

		static void SetActive(Scene* scene);
		static Scene* GetActive();

		void* AddComponent(ComponentId id, uint32_t entity);
		void* GetComponent(ComponentId id, uint32_t entity);
		const void* GetComponent(ComponentId id, uint32_t entity) const;
		void RemoveComponent(ComponentId id, uint32_t entity);

	private:
		friend class Entity;
		friend struct SceneFile;

		struct Record
		{
			uint32_t Id = 0;
			uint32_t Plane = 0;
			uint32_t Parent = 0;
			std::string Name;
			std::string Tag;
			std::string Prefab;
		};

		struct DrawEntry
		{
			uint32_t Entity = 0;
			const MaterialComponent* Material = nullptr;
			const MeshComponent* Mesh = nullptr;
			int Plane = 0;
			int Order = 0;
			Transform Drawn {};
		};

		std::string UniqueEntityName(std::string name);
		std::string UniquePlaneName(std::string name) const;
		void FitLocalToWorld(uint32_t id, const Transform& world);
		void SetLocalPose(uint32_t id, float x, float y, float rotation);
		int Depth(uint32_t id) const;
		void DestroyRecord(uint32_t id);

		Record* FindRecord(uint32_t id);
		const Record* FindRecord(uint32_t id) const;
		int PlaneOrder(uint32_t planeId) const;
		const Record* FindPrimaryCameraRecord() const;
		bool Read(std::istream& input);
		void Write(std::ostream& output) const;
		void ResolveTextures();
		void StartPhysics();
		void StopPhysics();
		void SyncPhysics();
		void CaptureMotion();
		void StepPhysics(float seconds);
		Transform DrawTransform(uint32_t id) const;

		struct MotionSample
		{
			uint32_t Entity = 0;
			float X = 0.0f;
			float Y = 0.0f;
			float Angle = 0.0f;
		};

		std::vector<MotionSample> m_Motion;
		float m_StepRemainder = 0.0f;
		void MarkStructure();
		void RetargetJoints(const std::string& previous, const std::string& name);
		void ApplyRequestedScene();
		void RebuildDrawList() const;
		struct PhysicsBodyRef;
		void SpawnPhysicsBody(Entity entity, bool preserveSnapshot);
		void RemovePhysicsBody(uint32_t entityId);
		bool TakeBody(uint32_t entityId, PhysicsBodyRef& body) const;
		void DetachDistanceJoints(uint32_t entityId, const std::string& name);
		void DetachRevoluteJoints(uint32_t entityId, const std::string& name);
		void DestroyJoint(uint32_t owner, ComponentId kind);
		void DestroyDistanceJoint(uint32_t owner);
		void DestroyRevoluteJoint(uint32_t owner);
		void SpawnDistanceJoint(uint32_t owner);
		void SpawnRevoluteJoint(uint32_t owner);
		void SpawnDistanceJoints();
		void SpawnRevoluteJoints();
		void RespawnDistanceJoints(const std::vector<uint32_t>& ids);
		void RespawnRevoluteJoints(const std::vector<uint32_t>& ids);

		struct PhysicsStorage;
		struct PhysicsDeleter
		{
			void operator()(PhysicsStorage* storage) const;
		};
		std::unique_ptr<PhysicsStorage, PhysicsDeleter> m_Physics;
		Scope<ComponentStorage> m_Components;

		std::string m_Name;
		std::string m_Path;
		std::string m_PlaySnapshot;
		std::string m_PlayPath;
		std::string m_RequestedScene;
		std::vector<Plane> m_Planes;
		std::vector<Record> m_Records;
		mutable std::vector<DrawEntry> m_DrawList;
		uint32_t m_NextPlane = 1;
		uint32_t m_NextId = 1;
		uint32_t m_StructureRevision = 1;
		mutable uint32_t m_DrawRevision = 0;
		bool m_Loaded = false;
		bool m_Dirty = false;
		bool m_SuppressDirty = false;
		bool m_Swapped = false;
		ScenePlayback m_Playback = ScenePlayback::Stopped;
	};

	template<typename T>
	bool Entity::Has() const
	{
		return Get<T>() != nullptr;
	}

	template<typename T>
	const T* Entity::Get() const
	{
		if (m_Scene == nullptr)
			return nullptr;
		return static_cast<const T*>(static_cast<const Scene*>(m_Scene)->GetComponent(ComponentTraits<T>::Id, m_Id));
	}

	template<typename T>
	T* Entity::Get()
	{
		if (m_Scene == nullptr)
			return nullptr;
		return static_cast<T*>(m_Scene->GetComponent(ComponentTraits<T>::Id, m_Id));
	}

	template<typename T>
	T& Entity::Add()
	{
		return *static_cast<T*>(m_Scene->AddComponent(ComponentTraits<T>::Id, m_Id));
	}

	template<typename T>
	void Entity::Remove()
	{
		if (m_Scene != nullptr)
			m_Scene->RemoveComponent(ComponentTraits<T>::Id, m_Id);
	}

	inline Transform Entity::WorldTransform() const
	{
		if (m_Scene == nullptr)
			return {};
		return m_Scene->WorldTransform(m_Id);
	}

}
