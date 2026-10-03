#include <Lite/Scene/Scene.h>

#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Console.h>

#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Assets/Texture.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Core/String.h>
#include <Lite/Project/Project.h>
#include <Lite/Renderer/Renderer2D.h>
#include <Lite/Script/ScriptModule.h>

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace Lite {

	struct SceneFile
	{
		static YAML::Node WriteEntityNode(const Scene& scene, uint32_t id, bool includePrefab)
		{
			YAML::Node entity(YAML::NodeType::Map);
			const Scene::Record* record = scene.FindRecord(id);
			if (record == nullptr)
				return entity;

			entity["name"] = record->Name;
			if (includePrefab && !record->Prefab.empty())
				entity["prefab"] = record->Prefab;

			for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
			{
				const ComponentOps* ops = FindComponent(static_cast<ComponentId>(index));
				const void* data = scene.GetComponent(static_cast<ComponentId>(index), id);
				if (ops == nullptr || ops->Write == nullptr || ops->Section == nullptr || data == nullptr)
					continue;

				YAML::Node component(YAML::NodeType::Map);
				ops->Write(component, data);
				entity[ops->Section] = component;
			}

			return entity;
		}

		static void ReadEntityNode(Scene& scene, uint32_t id, const YAML::Node& node)
		{
			if (!node || !node.IsMap())
				return;

			for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
			{
				const std::string key = it->first.as<std::string>();
				if (key == "name" || key == "plane")
					continue;
				if (key == "prefab")
				{
					if (it->second && it->second.IsScalar())
						scene.SetPrefab(id, it->second.as<std::string>());
					continue;
				}

				const ComponentOps* ops = FindComponentSection(key);
				if (ops == nullptr || ops->Read == nullptr)
					continue;

				ops->Read(scene, id, it->second);
				if (ops->Finish == nullptr)
					continue;
				if (void* data = scene.GetComponent(ops->Id, id))
					ops->Finish(data);
			}
		}
	};

	namespace {

		Scene* s_Active = nullptr;

		std::filesystem::path SceneDirectory()
		{
			return FileSystem::ExecutableDirectory() / "assets" / "scenes";
		}

		std::filesystem::path ResolvePath(std::string_view path)
		{
			std::filesystem::path file(path);
			if (file.is_absolute())
				return file;
			return FileSystem::ExecutableDirectory() / file;
		}

		Vec4 WithOpacity(Vec4 color, float opacity)
		{
			color.w *= opacity;
			return color;
		}

		DrawSurface SurfaceOf(const MaterialComponent& material)
		{
			DrawSurface surface;
			surface.Roughness = material.Roughness;
			surface.Metallic = material.Metallic;
			surface.Emission = material.Emission;
			surface.Offset = material.Offset;
			return surface;
		}

		void SubmitMesh(const Transform& transform, const MaterialComponent& material, MeshType type)
		{
			const DrawSurface surface = SurfaceOf(material);
			const float opacity = material.Color.w;
			if (type == MeshType::Triangle)
			{
				const Vec4 first = material.UseVertexColors ? WithOpacity(material.Colors[0], opacity) : material.Color;
				const Vec4 second = material.UseVertexColors ? WithOpacity(material.Colors[1], opacity) : material.Color;
				const Vec4 third = material.UseVertexColors ? WithOpacity(material.Colors[2], opacity) : material.Color;
				if (material.Texture)
					Renderer2D::DrawTriangle(transform, material.Texture, material.Tiling, first, second, third, surface);
				else
					Renderer2D::DrawTriangle(transform, first, second, third, surface);
				return;
			}

			if (material.UseVertexColors)
			{
				Renderer2D::DrawQuad(transform, material.Texture, material.Tiling,
					WithOpacity(material.Colors[0], opacity),
					WithOpacity(material.Colors[1], opacity),
					WithOpacity(material.Colors[2], opacity),
					WithOpacity(material.Colors[3], opacity),
					surface);
				return;
			}

			if (material.Texture || type == MeshType::Sprite)
				Renderer2D::DrawQuad(transform, material.Texture, material.Tiling, material.Color, surface);
			else
				Renderer2D::DrawQuad(transform, material.Color, surface);
		}

	}

	Entity::Entity(Scene* scene, uint32_t id)
		: m_Scene(scene)
		, m_Id(id)
	{
	}

	bool Entity::IsValid() const
	{
		return m_Scene != nullptr && m_Scene->FindRecord(m_Id) != nullptr;
	}

	std::string Entity::GetName() const
	{
		const Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr;
		return record != nullptr ? record->Name : std::string{};
	}

	void Entity::SetName(std::string name)
	{
		if (Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr)
			record->Name = std::move(name);
	}

	void* Scene::AddComponent(ComponentId id, uint32_t entity)
	{
		if (FindRecord(entity) == nullptr || m_Components == nullptr)
			return nullptr;

		return m_Components->Emplace(id, entity);
	}

	void* Scene::GetComponent(ComponentId id, uint32_t entity)
	{
		return m_Components != nullptr ? m_Components->Find(id, entity) : nullptr;
	}

	const void* Scene::GetComponent(ComponentId id, uint32_t entity) const
	{
		return m_Components != nullptr ? m_Components->Find(id, entity) : nullptr;
	}

	void Scene::RemoveComponent(ComponentId id, uint32_t entity)
	{
		if (m_Components != nullptr)
			m_Components->Erase(id, entity);
	}

	Scene::Scene(std::string name)
		: m_Name(std::move(name))
		, m_Components(CreateScope<ComponentStorage>())
	{
	}

	Scene::~Scene()
	{
		Stop();
		m_Components.reset();
		if (s_Active == this)
			s_Active = nullptr;
	}

	Scene::Record* Scene::FindRecord(uint32_t id)
	{
		for (Record& record : m_Records)
		{
			if (record.Id == id)
				return &record;
		}

		return nullptr;
	}

	const Scene::Record* Scene::FindRecord(uint32_t id) const
	{
		for (const Record& record : m_Records)
		{
			if (record.Id == id)
				return &record;
		}

		return nullptr;
	}

	uint32_t Scene::GetParent(uint32_t id) const
	{
		const Record* record = FindRecord(id);
		if (record == nullptr || record->Parent == 0 || FindRecord(record->Parent) == nullptr)
			return 0;
		return record->Parent;
	}

	int Scene::Depth(uint32_t id) const
	{
		int depth = 0;
		for (uint32_t parent = GetParent(id); parent != 0 && depth < 64; parent = GetParent(parent))
			++depth;
		return depth;
	}

	Transform Scene::WorldTransform(uint32_t id) const
	{
		const Transform* chain[64] {};
		int count = 0;
		for (uint32_t cursor = id; cursor != 0 && count < 64; cursor = GetParent(cursor))
		{
			const auto* transform = static_cast<const TransformComponent*>(GetComponent(ComponentId::Transform, cursor));
			chain[count++] = transform != nullptr ? &transform->Local : nullptr;
		}

		Transform world;
		for (int index = count - 1; index >= 0; --index)
		{
			if (chain[index] == nullptr)
				continue;
			world = CombineTransforms(world, *chain[index]);
		}
		return world;
	}

	void Scene::FitLocalToWorld(uint32_t id, const Transform& world)
	{
		auto* transform = static_cast<TransformComponent*>(GetComponent(ComponentId::Transform, id));
		if (transform == nullptr)
			return;

		const uint32_t parent = GetParent(id);
		if (parent == 0)
		{
			transform->Local = world;
			return;
		}

		const Transform parentWorld = WorldTransform(parent);
		transform->Local.Position = parentWorld.InverseTransformPoint(world.Position);
		transform->Local.Rotation = parentWorld.Rotation.Normalized().Conjugate() * world.Rotation.Normalized();
		auto axis = [](float value, float basis)
		{
			return basis > 0.0001f || basis < -0.0001f ? value / basis : value;
		};
		transform->Local.Scale.x = axis(world.Scale.x, parentWorld.Scale.x);
		transform->Local.Scale.y = axis(world.Scale.y, parentWorld.Scale.y);
		transform->Local.Scale.z = axis(world.Scale.z, parentWorld.Scale.z);
	}

	void Scene::SetLocalPose(uint32_t id, float x, float y, float rotation)
	{
		auto* transform = static_cast<TransformComponent*>(GetComponent(ComponentId::Transform, id));
		if (transform == nullptr)
			return;

		const uint32_t parent = GetParent(id);
		if (parent == 0)
		{
			transform->Local.Position.x = x;
			transform->Local.Position.y = y;
			transform->Local.SetRotationZ(rotation);
			return;
		}

		const Transform parentWorld = WorldTransform(parent);
		const Vec3 local = parentWorld.InverseTransformPoint({ x, y, 0.0f });
		transform->Local.Position.x = local.x;
		transform->Local.Position.y = local.y;
		const Quat worldRotation = Quat::FromAxisAngle({ 0.0f, 0.0f, 1.0f }, rotation);
		transform->Local.Rotation = parentWorld.Rotation.Normalized().Conjugate() * worldRotation;
	}

	bool Scene::SetParent(uint32_t id, uint32_t parent, bool keepWorld)
	{
		Record* record = FindRecord(id);
		if (record == nullptr)
			return false;
		if (parent == id)
			return false;
		if (parent != 0 && FindRecord(parent) == nullptr)
			return false;

		for (uint32_t cursor = parent; cursor != 0; cursor = GetParent(cursor))
		{
			if (cursor == id)
				return false;
		}

		if (record->Parent == parent)
			return false;

		Transform world;
		if (keepWorld)
			world = WorldTransform(id);

		record->Parent = parent;
		if (keepWorld)
			FitLocalToWorld(id, world);
		return true;
	}

	std::vector<Entity> Scene::GetChildren(uint32_t id)
	{
		std::vector<Entity> children;
		for (const Record& record : m_Records)
		{
			if (record.Parent == id)
				children.emplace_back(this, record.Id);
		}
		return children;
	}

	int Scene::CountDescendants(uint32_t id) const
	{
		int count = 0;
		std::vector<uint32_t> pending { id };
		for (size_t index = 0; index < pending.size(); ++index)
		{
			for (const Record& record : m_Records)
			{
				if (record.Parent != pending[index])
					continue;
				pending.push_back(record.Id);
				++count;
			}
		}
		return count;
	}

	uint32_t Scene::CreatePlane(std::string name)
	{
		if (name.empty())
			name = "Plane";

		Plane plane;
		plane.Id = m_NextPlane++;
		plane.Name = std::move(name);
		for (const Plane& existing : m_Planes)
			plane.Order = std::max(plane.Order, existing.Order + 1);
		m_Planes.push_back(std::move(plane));
		return m_Planes.back().Id;
	}

	std::string Scene::UniquePlaneName(std::string name) const
	{
		if (name.empty())
			name = "Plane";
		if (FindPlane(name) == 0)
			return name;

		for (int index = 2; index < 1000; ++index)
		{
			std::string candidate = std::format("{} {}", name, index);
			if (FindPlane(candidate) == 0)
				return candidate;
		}

		return name;
	}

	uint32_t Scene::AddPlane()
	{
		return CreatePlane(UniquePlaneName("Plane"));
	}

	bool Scene::SetPlaneName(uint32_t id, std::string name)
	{
		if (name.empty())
			return false;

		const uint32_t existing = FindPlane(name);
		if (existing != 0 && existing != id)
			return false;

		for (Plane& plane : m_Planes)
		{
			if (plane.Id != id)
				continue;

			plane.Name = std::move(name);
			return true;
		}

		return false;
	}

	Scene::PlaneRemove Scene::RemovePlane(uint32_t id)
	{
		auto found = std::find_if(m_Planes.begin(), m_Planes.end(), [&](const Plane& plane)
		{
			return plane.Id == id;
		});
		if (found == m_Planes.end())
			return PlaneRemove::Missing;
		if (m_Planes.size() <= 1)
			return PlaneRemove::LastPlane;

		int count = 0;
		for (const Record& record : m_Records)
		{
			if (record.Plane == id)
				++count;
		}

		const bool world = ToLower(found->Name) == "world";
		if (count > 0 && world)
			return PlaneRemove::HasEntities;

		if (count > 0)
		{
			uint32_t destination = FindPlane("World");
			if (destination == 0 || destination == id)
				destination = CreatePlane("World");

			for (Record& record : m_Records)
			{
				if (record.Plane == id)
					record.Plane = destination;
			}
		}

		found = std::find_if(m_Planes.begin(), m_Planes.end(), [&](const Plane& plane)
		{
			return plane.Id == id;
		});
		if (found != m_Planes.end())
			m_Planes.erase(found);

		return count > 0 ? PlaneRemove::MovedToWorld : PlaneRemove::Removed;
	}

	uint32_t Scene::FindPlane(std::string_view name) const
	{
		const std::string lower = ToLower(name);
		for (const Plane& plane : m_Planes)
		{
			if (ToLower(plane.Name) == lower)
				return plane.Id;
		}

		return 0;
	}

	void Scene::SetPlaneOrder(uint32_t id, int order)
	{
		for (Plane& plane : m_Planes)
		{
			if (plane.Id != id)
				continue;
			plane.Order = order;
			return;
		}
	}

	int Scene::PlaneOrder(uint32_t planeId) const
	{
		for (const Plane& plane : m_Planes)
		{
			if (plane.Id == planeId)
				return plane.Order;
		}

		return 0;
	}

	Entity Scene::CreateEntity(std::string name)
	{
		if (m_Planes.empty())
			CreatePlane("World");
		return CreateEntity(std::move(name), m_Planes.front().Id);
	}

	Entity Scene::CreateEntity(std::string name, uint32_t plane)
	{
		bool found = false;
		for (const Plane& item : m_Planes)
		{
			if (item.Id == plane)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			if (m_Planes.empty())
				plane = CreatePlane("World");
			else
				plane = m_Planes.front().Id;
		}

		Record record;
		record.Id = m_NextId++;
		record.Plane = plane;
		record.Name = std::move(name);
		m_Records.push_back(std::move(record));
		AddComponent(ComponentId::Transform, m_Records.back().Id);
		return Entity(this, m_Records.back().Id);
	}

	std::string Scene::UniqueEntityName(std::string name)
	{
		if (name.empty())
			name = "Entity";
		if (!Find(name))
			return name;

		for (int index = 2; index < 1000; ++index)
		{
			std::string candidate = std::format("{} {}", name, index);
			if (!Find(candidate))
				return candidate;
		}

		return name;
	}

	Entity Scene::DuplicateEntity(uint32_t id)
	{
		Record* source = FindRecord(id);
		if (source == nullptr)
			return {};

		const std::string name = source->Name.empty() ? std::string("Entity Copy") : source->Name + " Copy";
		const uint32_t plane = source->Plane;
		const uint32_t parent = source->Parent;
		YAML::Node node = SceneFile::WriteEntityNode(*this, id, true);

		Entity copy = CreateEntity(name, plane);
		SceneFile::ReadEntityNode(*this, copy.GetId(), node);

		if (CameraComponent* camera = copy.Get<CameraComponent>())
			camera->Primary = false;
		if (Record* copyRecord = FindRecord(copy.GetId()))
			copyRecord->Parent = parent;

		auto created = std::find_if(m_Records.begin(), m_Records.end(), [&](const Record& record)
		{
			return record.Id == copy.GetId();
		});
		auto origin = std::find_if(m_Records.begin(), m_Records.end(), [&](const Record& record)
		{
			return record.Id == id;
		});
		if (created != m_Records.end() && origin != m_Records.end() && created != origin + 1)
		{
			Record moved = std::move(*created);
			m_Records.erase(created);
			origin = std::find_if(m_Records.begin(), m_Records.end(), [&](const Record& record)
			{
				return record.Id == id;
			});
			m_Records.insert(origin + 1, std::move(moved));
		}

		RefreshPhysics(copy.GetId());
		return copy;
	}

	std::string Scene::GetPrefab(uint32_t id) const
	{
		const Record* record = FindRecord(id);
		return record != nullptr ? record->Prefab : std::string();
	}

	void Scene::SetPrefab(uint32_t id, std::string path)
	{
		if (Record* record = FindRecord(id))
			record->Prefab = std::move(path);
	}

	bool Scene::SavePrefab(uint32_t id, const std::filesystem::path& path) const
	{
		if (FindRecord(id) == nullptr || path.empty())
		{
			Console::Log("Failed to save prefab: nothing selected");
			return false;
		}

		std::filesystem::path full = path;
		if (!full.is_absolute())
			full = FileSystem::ExecutableDirectory() / full;
		if (full.extension().empty())
			full.replace_extension(".prefab");
		full = full.lexically_normal();

		std::error_code error;
		if (!full.parent_path().empty())
			std::filesystem::create_directories(full.parent_path(), error);

		YAML::Node root = SceneFile::WriteEntityNode(*this, id, false);
		const Record* record = FindRecord(id);
		for (const Plane& plane : m_Planes)
		{
			if (record != nullptr && plane.Id == record->Plane)
			{
				root["plane"] = plane.Name;
				break;
			}
		}

		std::ofstream file(full, std::ios::trunc);
		if (!file)
		{
			Console::Log(std::format("Failed to save prefab: {}", full.string()));
			return false;
		}

		file << root;
		if (!file)
		{
			Console::Log(std::format("Failed to save prefab: {}", full.string()));
			return false;
		}

		Console::Log(std::format("Prefab saved: {}", full.string()));
		return true;
	}

	bool Scene::SaveMaterial(uint32_t id, const std::filesystem::path& path) const
	{
		const ComponentOps* ops = FindComponent(ComponentId::Material);
		const void* data = GetComponent(ComponentId::Material, id);
		if (ops == nullptr || ops->Write == nullptr || data == nullptr || path.empty())
		{
			Console::Log("Failed to save material: the entity has no material");
			return false;
		}

		std::filesystem::path full = path;
		if (!full.is_absolute())
			full = FileSystem::ExecutableDirectory() / full;
		if (full.extension().empty())
			full.replace_extension(".material");
		full = full.lexically_normal();

		std::error_code error;
		if (!full.parent_path().empty())
			std::filesystem::create_directories(full.parent_path(), error);

		YAML::Node root(YAML::NodeType::Map);
		ops->Write(root, data);

		std::ofstream file(full, std::ios::trunc);
		if (!file)
		{
			Console::Log(std::format("Failed to save material: {}", full.string()));
			return false;
		}

		file << root;
		if (!file)
		{
			Console::Log(std::format("Failed to save material: {}", full.string()));
			return false;
		}

		Console::Log(std::format("Material saved: {}", full.string()));
		return true;
	}

	bool Scene::ApplyMaterial(uint32_t id, const std::filesystem::path& path)
	{
		if (FindRecord(id) == nullptr || path.empty())
			return false;

		std::filesystem::path full = path;
		if (!full.is_absolute())
			full = Project::GetAssetFileSystemPath(full);

		std::ifstream file(full);
		if (!file)
		{
			Console::Log(std::format("Failed to apply material: {}", full.string()));
			return false;
		}

		YAML::Node root;
		try
		{
			root = YAML::Load(file);
		}
		catch (const YAML::Exception&)
		{
			Console::Log(std::format("Failed to apply material: {}", full.string()));
			return false;
		}
		if (!root || !root.IsMap())
		{
			Console::Log(std::format("Failed to apply material: {}", full.string()));
			return false;
		}

		const ComponentOps* ops = FindComponent(ComponentId::Material);
		if (ops == nullptr || ops->Read == nullptr)
			return false;

		ops->Read(*this, id, root);
		if (ops->Finish != nullptr)
		{
			if (void* data = GetComponent(ComponentId::Material, id))
				ops->Finish(data);
		}

		Console::Log(std::format("Applied material {}", full.stem().string()));
		return true;
	}

	Entity Scene::PlacePrefab(const std::filesystem::path& path, std::optional<Vec2> position)
	{
		if (path.empty())
			return {};

		std::filesystem::path full = path;
		if (!full.is_absolute())
			full = Project::GetAssetFileSystemPath(full);

		std::ifstream file(full);
		if (!file)
		{
			Console::Log(std::format("Failed to place prefab: {}", full.string()));
			return {};
		}

		YAML::Node root = YAML::Load(file);
		if (!root || !root.IsMap())
		{
			Console::Log(std::format("Failed to place prefab: {}", full.string()));
			return {};
		}

		std::string name = full.stem().string();
		if (root["name"])
			name = root["name"].as<std::string>();
		name = UniqueEntityName(name);

		uint32_t plane = 0;
		if (root["plane"])
			plane = FindPlane(root["plane"].as<std::string>());
		if (plane == 0)
			plane = FindPlane("World");
		if (plane == 0 && !m_Planes.empty())
			plane = m_Planes.front().Id;

		Entity entity = plane != 0 ? CreateEntity(name, plane) : CreateEntity(name);
		SceneFile::ReadEntityNode(*this, entity.GetId(), root);
		if (CameraComponent* camera = entity.Get<CameraComponent>())
			camera->Primary = false;

		std::error_code canonicalError;
		std::filesystem::path canonical = std::filesystem::weakly_canonical(full, canonicalError);
		if (canonicalError)
			canonical = full.lexically_normal();

		std::string link = canonical.generic_string();
		const std::filesystem::path assets = Project::GetAssetDirectory();
		if (!assets.empty() && FileSystem::Contains(assets, canonical))
		{
			std::error_code relativeError;
			std::filesystem::path relative = std::filesystem::relative(canonical, assets, relativeError);
			if (!relativeError)
				link = relative.generic_string();
		}
		SetPrefab(entity.GetId(), link);

		if (position)
		{
			if (TransformComponent* transform = entity.Get<TransformComponent>())
			{
				transform->Local.Position.x = position->x;
				transform->Local.Position.y = position->y;
			}
		}

		RefreshPhysics(entity.GetId());
		Console::Log(std::format("Placed prefab {}", name));
		return entity;
	}

	void Scene::DestroyRecord(uint32_t id)
	{
		if (FindRecord(id) == nullptr)
			return;

		RemovePhysicsBody(id);
		for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
			RemoveComponent(static_cast<ComponentId>(index), id);

		for (auto record = m_Records.begin(); record != m_Records.end(); ++record)
		{
			if (record->Id != id)
				continue;
			m_Records.erase(record);
			break;
		}
	}

	Scene::EntityDestroy Scene::DestroyEntity(uint32_t id, bool detachChildren)
	{
		if (FindRecord(id) == nullptr)
			return EntityDestroy::Missing;

		std::vector<uint32_t> descendants;
		std::vector<uint32_t> pending { id };
		for (size_t index = 0; index < pending.size(); ++index)
		{
			for (const Record& record : m_Records)
			{
				if (record.Parent != pending[index])
					continue;
				descendants.push_back(record.Id);
				pending.push_back(record.Id);
			}
		}

		if (!descendants.empty() && detachChildren)
		{
			std::vector<uint32_t> children;
			for (const Record& record : m_Records)
			{
				if (record.Parent == id)
					children.push_back(record.Id);
			}
			for (uint32_t child : children)
				SetParent(child, 0, true);
			DestroyRecord(id);
			return EntityDestroy::DetachedChildren;
		}

		if (!descendants.empty())
		{
			for (size_t index = descendants.size(); index-- > 0; )
				DestroyRecord(descendants[index]);
			DestroyRecord(id);
			return EntityDestroy::RemovedChildren;
		}

		DestroyRecord(id);
		return EntityDestroy::Removed;
	}

	uint32_t Scene::NextEntity(uint32_t id) const
	{
		uint32_t next = 0;
		uint32_t previous = 0;
		bool found = false;
		for (const Plane& plane : m_Planes)
		{
			for (const Record& record : m_Records)
			{
				if (record.Plane != plane.Id)
					continue;
				if (record.Id == id)
				{
					found = true;
					continue;
				}

				if (!found)
					previous = record.Id;
				else if (next == 0)
					next = record.Id;
			}
		}

		if (!found)
			return 0;
		return next != 0 ? next : previous;
	}

	Scene::TextureAssign Scene::AssignTexture(uint32_t entityId, const std::string& path)
	{
		TextureAssign result;
		Entity entity = GetEntity(entityId);
		MaterialComponent* material = entity ? entity.Get<MaterialComponent>() : nullptr;
		if (material == nullptr)
			return result;

		material->TexturePath = AssetRegistry::Get().Store(path);
		material->Texture = material->TexturePath.empty() ? Ref<Texture>{} : AssetRegistry::Get().Load<Texture>(material->TexturePath);
		result.Path = material->TexturePath;
		if (MeshComponent* mesh = entity.Get<MeshComponent>())
			mesh->Type = MeshType::Sprite;
		result.Applied = true;
		result.Loaded = static_cast<bool>(material->Texture);
		return result;
	}

	Scene::SpritePlacement Scene::PlaceSprite(const std::string& texturePath, Vec2 world, Vec2 viewCenter, float viewSize, float aspect)
	{
		SpritePlacement placed;
		if (texturePath.empty())
			return placed;

		uint32_t backgroundPlane = FindPlane("background");
		uint32_t worldPlane = 0;
		for (const Plane& item : m_Planes)
		{
			if (item.Id != backgroundPlane)
			{
				worldPlane = item.Id;
				break;
			}
		}
		if (worldPlane == 0)
			worldPlane = backgroundPlane != 0 ? backgroundPlane : (m_Planes.empty() ? 0 : m_Planes.front().Id);

		bool backgroundEmpty = backgroundPlane != 0;
		if (backgroundEmpty)
		{
			for (const Record& record : m_Records)
			{
				if (record.Plane != backgroundPlane)
					continue;
				if (GetComponent(ComponentId::Mesh, record.Id) != nullptr)
				{
					backgroundEmpty = false;
					break;
				}
			}
		}

		placed.Background = backgroundEmpty;
		const uint32_t plane = placed.Background ? backgroundPlane : worldPlane;
		std::string name = std::filesystem::path(texturePath).stem().string();
		if (name.empty())
			name = "Sprite";

		Entity entity = plane != 0 ? CreateEntity(name, plane) : CreateEntity(name);
		if (TransformComponent* transform = entity.Get<TransformComponent>())
		{
			if (placed.Background)
			{
				transform->Local.Position = { viewCenter.x, viewCenter.y, 0.0f };
				float height = viewSize > 0.0f ? viewSize * 4.0f : 8.0f;
				transform->Local.Scale = { height * aspect, height, 1.0f };
			}
			else
			{
				transform->Local.Position = { world.x, world.y, 0.0f };
				transform->Local.Scale = { 1.0f, 1.0f, 1.0f };
			}
		}

		int order = 1;
		if (!placed.Background)
		{
			for (const Record& record : m_Records)
			{
				const auto* sorting = static_cast<const SortingComponent*>(GetComponent(ComponentId::Sorting, record.Id));
				if (sorting != nullptr)
					order = std::max(order, sorting->Order + 1);
			}
		}

		entity.Add<MeshComponent>().Type = MeshType::Sprite;
		MaterialComponent& material = entity.Add<MaterialComponent>();
		material.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		material.UseVertexColors = false;
		material.Tiling = { 1.0f, 1.0f };
		placed.Loaded = AssignTexture(entity.GetId(), texturePath).Loaded;
		entity.Add<SortingComponent>().Order = placed.Background ? -10 : order;

		placed.Entity = entity.GetId();
		return placed;
	}

	Entity Scene::Find(std::string_view name)
	{
		for (const Record& record : m_Records)
		{
			if (record.Name == name)
				return Entity(this, record.Id);
		}

		return {};
	}

	Entity Scene::GetEntity(uint32_t id)
	{
		return FindRecord(id) != nullptr ? Entity(this, id) : Entity{};
	}

	const Scene::Record* Scene::FindPrimaryCameraRecord() const
	{
		for (const Record& record : m_Records)
		{
			const auto* camera = static_cast<const CameraComponent*>(GetComponent(ComponentId::Camera, record.Id));
			if (camera != nullptr && camera->Primary)
				return &record;
		}

		return nullptr;
	}

	Entity Scene::GetPrimaryCamera()
	{
		const Record* record = FindPrimaryCameraRecord();
		return record != nullptr ? Entity(this, record->Id) : Entity{};
	}

	std::vector<Entity> Scene::GetEntities()
	{
		std::vector<Entity> entities;
		entities.reserve(m_Records.size());
		for (const Plane& plane : m_Planes)
		{
			for (const Record& record : m_Records)
			{
				if (record.Plane == plane.Id)
					entities.emplace_back(this, record.Id);
			}
		}
		return entities;
	}

	std::vector<Entity> Scene::GetEntities(uint32_t plane)
	{
		std::vector<Entity> entities;
		for (const Record& record : m_Records)
		{
			if (record.Plane == plane)
				entities.emplace_back(this, record.Id);
		}
		return entities;
	}

	void Scene::SetPrimaryCamera(uint32_t id)
	{
		for (Record& record : m_Records)
		{
			auto* camera = static_cast<CameraComponent*>(GetComponent(ComponentId::Camera, record.Id));
			if (camera != nullptr)
				camera->Primary = record.Id == id;
		}
	}

	void Scene::Create()
	{
		if (m_Loaded)
			return;

		if (m_Planes.empty())
			CreatePlane("World");

		m_Loaded = true;
		m_Playback = ScenePlayback::Stopped;
		Console::Log(std::format("Scene created: {}", m_Name));
	}

	void Scene::Load()
	{
		if (m_Loaded)
			return;

		m_Loaded = true;
		if (m_Path.empty())
			Console::Log(std::format("Scene loaded: {}", m_Name));
		else
			Console::Log(std::format("Scene loaded: {} ({})", m_Name, m_Path));
		for (const Plane& plane : m_Planes)
		{
			Console::Log(std::format("  {}", plane.Name));
			for (const Record& record : m_Records)
			{
				if (record.Plane == plane.Id)
					Console::Log(std::format("    {}", record.Name));
			}
		}
	}

	void Scene::Play()
	{
		if (!m_Loaded)
			Load();

		if (m_Playback == ScenePlayback::Playing)
			return;

		bool paused = m_Playback == ScenePlayback::Paused;
		m_Playback = ScenePlayback::Playing;
		if (paused)
			SyncPhysics();
		else
			StartPhysics();
		Console::Log(std::format("Scene playing: {}", m_Name));
	}

	void Scene::Pause()
	{
		if (m_Playback != ScenePlayback::Playing)
			return;

		m_Playback = ScenePlayback::Paused;
		Console::Log(std::format("Scene paused: {}", m_Name));
	}

	void Scene::Stop()
	{
		if (m_Playback == ScenePlayback::Stopped)
			return;

		StopPhysics();
		m_Playback = ScenePlayback::Stopped;
		Console::Log(std::format("Scene stopped: {}", m_Name));
	}

	void Scene::Restart()
	{
		if (!m_Loaded)
			Load();

		StopPhysics();
		m_Playback = ScenePlayback::Stopped;
		StartPhysics();
		m_Playback = ScenePlayback::Playing;
		Console::Log(std::format("Scene restarted: {}", m_Name));
	}

	void Scene::Update(float seconds)
	{
		if (m_Playback != ScenePlayback::Playing)
			return;

		ScriptRuntime::Update(*this, seconds);
		StepPhysics(seconds);
	}

	Mat4 Scene::ViewProjection(float aspect, const Transform& fallbackTransform, const CameraComponent& fallbackCamera) const
	{
		const Record* record = IsPlaying() ? FindPrimaryCameraRecord() : nullptr;
		const auto* transform = record != nullptr ? static_cast<const TransformComponent*>(GetComponent(ComponentId::Transform, record->Id)) : nullptr;
		const auto* camera = record != nullptr ? static_cast<const CameraComponent*>(GetComponent(ComponentId::Camera, record->Id)) : nullptr;
		if (transform == nullptr || camera == nullptr)
			return CameraProjectionMatrix(fallbackCamera, aspect) * fallbackTransform.GetViewMatrix();

		return CameraProjectionMatrix(*camera, aspect) * WorldTransform(record->Id).GetViewMatrix();
	}

	void Scene::Close(Scope<Scene>& scene)
	{
		if (scene == nullptr)
			return;

		scene->Stop();
		if (s_Active == scene.get())
			s_Active = nullptr;
		scene.reset();
	}

	void Scene::Render() const
	{
		struct DrawItem
		{
			Transform Transform;
			const MaterialComponent* Material = nullptr;
			const MeshComponent* Mesh = nullptr;
			int Plane = 0;
			int Order = 0;
		};

		std::vector<int> planeOrders(m_NextPlane, 0);
		for (const Plane& plane : m_Planes)
		{
			if (plane.Id < planeOrders.size())
				planeOrders[plane.Id] = plane.Order;
		}

		std::vector<DrawItem> draw;
		draw.reserve(m_Records.size());
		for (const Record& record : m_Records)
		{
			const auto* mesh = static_cast<const MeshComponent*>(GetComponent(ComponentId::Mesh, record.Id));
			const auto* transform = static_cast<const TransformComponent*>(GetComponent(ComponentId::Transform, record.Id));
			const auto* material = static_cast<const MaterialComponent*>(GetComponent(ComponentId::Material, record.Id));
			if (mesh == nullptr || transform == nullptr || material == nullptr)
				continue;

			const auto* sorting = static_cast<const SortingComponent*>(GetComponent(ComponentId::Sorting, record.Id));
			const int plane = record.Plane < planeOrders.size() ? planeOrders[record.Plane] : 0;
			draw.push_back({ WorldTransform(record.Id), material, mesh, plane, sorting != nullptr ? sorting->Order : 0 });
		}

		std::stable_sort(draw.begin(), draw.end(), [](const DrawItem& left, const DrawItem& right)
		{
			if (left.Plane != right.Plane)
				return left.Plane < right.Plane;
			return left.Order < right.Order;
		});

		for (const DrawItem& item : draw)
			SubmitMesh(item.Transform, *item.Material, item.Mesh->Type);
	}

	bool Scene::Read(std::istream& input)
	{
		YAML::Node root;
		try
		{
			root = YAML::Load(input);
		}
		catch (const YAML::Exception&)
		{
			return false;
		}

		if (!root || !root.IsMap())
			return false;

		if (root["name"])
			m_Name = root["name"].as<std::string>();

		struct ParentLink
		{
			uint32_t Entity = 0;
			int Parent = -1;
		};

		std::vector<uint32_t> created;
		std::vector<ParentLink> parents;

		auto readEntity = [this, &created, &parents](uint32_t plane, const YAML::Node& entityNode)
		{
			if (!entityNode || !entityNode.IsMap())
				return;

			std::string name = "Entity";
			if (entityNode["name"])
				name = entityNode["name"].as<std::string>();
			if (name.empty())
				name = "Entity";

			Entity entity = CreateEntity(name, plane);
			created.push_back(entity.GetId());
			if (entityNode["parent"] && entityNode["parent"].IsScalar())
				parents.push_back({ entity.GetId(), entityNode["parent"].as<int>() });
			SceneFile::ReadEntityNode(*this, entity.GetId(), entityNode);
		};

		auto readEntities = [&readEntity](uint32_t plane, const YAML::Node& entities)
		{
			if (!entities || !entities.IsSequence())
				return;

			for (const YAML::Node& entityNode : entities)
				readEntity(plane, entityNode);
		};

		const YAML::Node planes = root["planes"];
		if (planes && planes.IsSequence())
		{
			for (const YAML::Node& planeNode : planes)
			{
				if (!planeNode || !planeNode.IsMap())
					continue;

				std::string name = "Plane";
				if (planeNode["name"])
					name = planeNode["name"].as<std::string>();
				if (name.empty())
					name = "Plane";

				uint32_t plane = CreatePlane(name);
				if (planeNode["order"])
					SetPlaneOrder(plane, planeNode["order"].as<int>());
				readEntities(plane, planeNode["entities"]);
			}
		}
		else
		{
			readEntities(CreatePlane("World"), root["entities"]);
		}

		if (m_Planes.empty())
			CreatePlane("World");

		for (const ParentLink& link : parents)
		{
			if (link.Parent < 0 || static_cast<size_t>(link.Parent) >= created.size())
				continue;
			SetParent(link.Entity, created[static_cast<size_t>(link.Parent)], false);
		}

		if (m_Name.empty())
			m_Name = "Untitled";

		return true;
	}

	void Scene::Write(std::ostream& output) const
	{
		YAML::Node root;
		root["name"] = m_Name;

		std::vector<uint32_t> order;
		for (const Plane& plane : m_Planes)
		{
			for (const Record& record : m_Records)
			{
				if (record.Plane == plane.Id)
					order.push_back(record.Id);
			}
		}

		auto indexOf = [&order](uint32_t id) -> int
		{
			for (size_t index = 0; index < order.size(); ++index)
			{
				if (order[index] == id)
					return static_cast<int>(index);
			}
			return -1;
		};

		YAML::Node planes(YAML::NodeType::Sequence);
		for (const Plane& plane : m_Planes)
		{
			YAML::Node planeNode;
			planeNode["name"] = plane.Name;
			planeNode["order"] = plane.Order;

			YAML::Node entities(YAML::NodeType::Sequence);
			for (const Record& record : m_Records)
			{
				if (record.Plane != plane.Id)
					continue;

				YAML::Node entity = SceneFile::WriteEntityNode(*this, record.Id, true);
				const int parent = indexOf(record.Parent);
				if (record.Parent != 0 && parent >= 0)
					entity["parent"] = parent;
				entities.push_back(std::move(entity));
			}

			planeNode["entities"] = entities;
			planes.push_back(planeNode);
		}

		root["planes"] = planes;
		output << root;
	}

	std::string Scene::Snapshot() const
	{
		std::ostringstream output;
		Write(output);
		return output.str();
	}

	bool Scene::Restore(std::string_view document)
	{
		const std::string path = m_Path;
		const ScenePlayback playback = m_Playback;
		StopPhysics();
		m_Planes.clear();
		m_Records.clear();
		m_NextPlane = 1;
		m_NextId = 1;
		m_Components = CreateScope<ComponentStorage>();

		std::istringstream input { std::string(document) };
		if (!Read(input))
			return false;

		m_Path = path;
		m_Loaded = true;
		ResolveTextures();
		m_Playback = ScenePlayback::Stopped;
		if (playback != ScenePlayback::Stopped)
		{
			StartPhysics();
			m_Playback = playback;
		}
		return true;
	}

	void Scene::ResolveTextures()
	{
		for (Record& record : m_Records)
		{
			for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
			{
				const ComponentOps* ops = FindComponent(static_cast<ComponentId>(index));
				if (ops == nullptr || ops->Finish == nullptr)
					continue;

				if (void* data = GetComponent(ops->Id, record.Id))
					ops->Finish(data);
			}
		}
	}

	bool Scene::Save()
	{
		if (m_Path.empty())
			return false;

		return SaveAs(m_Path);
	}

	bool Scene::SaveAs(std::string_view path)
	{
		std::filesystem::path full { std::string(path) };
		if (full.empty())
		{
			Console::Log("Failed to save scene: empty path");
			return false;
		}

		if (!full.is_absolute())
			full = FileSystem::ExecutableDirectory() / full;
		if (full.extension().empty())
			full.replace_extension(".scene");
		full = full.lexically_normal();

		std::error_code error;
		if (!full.parent_path().empty())
			std::filesystem::create_directories(full.parent_path(), error);

		std::ofstream file(full, std::ios::trunc);
		if (!file)
		{
			Console::Log(std::format("Failed to save scene: {}", full.string()));
			return false;
		}

		std::string previousName = m_Name;
		m_Name = full.stem().string();
		Write(file);
		if (!file)
		{
			m_Name = std::move(previousName);
			Console::Log(std::format("Failed to save scene: {}", full.string()));
			return false;
		}

		std::error_code canonicalError;
		std::filesystem::path canonical = std::filesystem::weakly_canonical(full, canonicalError);
		m_Path = (canonicalError ? full.lexically_normal() : canonical).string();
		Console::Log(std::format("Scene saved: {}", m_Path));
		return true;
	}

	std::string Scene::Locate(std::string_view relativeToProject)
	{
		std::filesystem::path found = FileSystem::Locate(relativeToProject);
		return found.empty() ? std::string{} : found.string();
	}

	Scope<Scene> Scene::Open(std::string_view path)
	{
		if (path.empty())
		{
			Console::Log("Failed to load scene: empty path");
			return nullptr;
		}

		std::filesystem::path full = ResolvePath(path);
		std::ifstream file(full);
		if (!file)
		{
			Console::Log(std::format("Failed to load scene: {}", path));
			return nullptr;
		}

		auto scene = CreateScope<Scene>(std::string{});
		if (!scene->Read(file))
		{
			Console::Log(std::format("Failed to load scene: {}", path));
			return nullptr;
		}

		std::error_code canonicalError;
		std::filesystem::path canonical = std::filesystem::weakly_canonical(full, canonicalError);
		scene->m_Path = (canonicalError ? full.lexically_normal() : canonical).string();
		scene->ResolveTextures();
		scene->Load();
		return scene;
	}

	std::vector<std::string> Scene::List()
	{
		std::vector<std::string> scenes;
		std::error_code error;
		std::filesystem::path directory = SceneDirectory();
		if (!std::filesystem::exists(directory, error))
			return scenes;

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, error))
		{
			std::error_code fileError;
			if (error || !entry.is_regular_file(fileError))
				continue;
			if (entry.path().extension() != ".scene")
				continue;

			std::filesystem::path relative = std::filesystem::path("assets") / "scenes" / entry.path().filename();
			scenes.push_back(relative.generic_string());
		}

		return scenes;
	}

	void Scene::SetActive(Scene* scene)
	{
		s_Active = scene;
	}

	Scene* Scene::GetActive()
	{
		return s_Active;
	}

}
