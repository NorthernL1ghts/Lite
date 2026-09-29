#include <Lite/Scene/Scene.h>

#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Console.h>

#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Renderer/Renderer2D.h>

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>

namespace Lite {

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
		, m_Components(new ComponentStorage())
	{
	}

	Scene::~Scene()
	{
		Stop();
		delete m_Components;
		m_Components = nullptr;
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

	uint32_t Scene::CreatePlane(std::string name)
	{
		if (name.empty())
			name = "Plane";

		Plane plane;
		plane.Id = m_NextPlane++;
		plane.Name = std::move(name);
		m_Planes.push_back(std::move(plane));
		return m_Planes.back().Id;
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

	Entity Scene::DuplicateEntity(uint32_t id)
	{
		Record* source = FindRecord(id);
		if (source == nullptr)
			return {};

		const std::string name = source->Name.empty() ? std::string("Entity Copy") : source->Name + " Copy";
		const uint32_t plane = source->Plane;

		struct Piece
		{
			const ComponentOps* Ops = nullptr;
			YAML::Node Node;
		};

		std::vector<Piece> pieces;
		for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
		{
			const ComponentOps* ops = FindComponent(static_cast<ComponentId>(index));
			const void* data = GetComponent(static_cast<ComponentId>(index), id);
			if (ops == nullptr || ops->Write == nullptr || data == nullptr)
				continue;

			Piece piece;
			piece.Ops = ops;
			piece.Node = YAML::Node(YAML::NodeType::Map);
			ops->Write(piece.Node, data);
			pieces.push_back(std::move(piece));
		}

		Entity copy = CreateEntity(name, plane);
		for (const Piece& piece : pieces)
		{
			if (piece.Ops->Read != nullptr)
				piece.Ops->Read(*this, copy.GetId(), piece.Node);
			if (piece.Ops->Finish == nullptr)
				continue;
			if (void* data = GetComponent(piece.Ops->Id, copy.GetId()))
				piece.Ops->Finish(data);
		}

		if (CameraComponent* camera = copy.Get<CameraComponent>())
			camera->Primary = false;

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

	void Scene::DestroyEntity(uint32_t id)
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

		StepPhysics(seconds);
	}

	Mat4 Scene::ViewProjection(float aspect, const Transform& fallbackTransform, const CameraComponent& fallbackCamera) const
	{
		const Record* record = IsPlaying() ? FindPrimaryCameraRecord() : nullptr;
		const auto* transform = record != nullptr ? static_cast<const TransformComponent*>(GetComponent(ComponentId::Transform, record->Id)) : nullptr;
		const auto* camera = record != nullptr ? static_cast<const CameraComponent*>(GetComponent(ComponentId::Camera, record->Id)) : nullptr;
		if (transform == nullptr || camera == nullptr)
			return CameraProjectionMatrix(fallbackCamera, aspect) * fallbackTransform.GetViewMatrix();

		return CameraProjectionMatrix(*camera, aspect) * transform->Local.GetViewMatrix();
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
			uint32_t Id = 0;
			int Order = 0;
		};

		std::vector<DrawItem> draw;
		for (const Record& record : m_Records)
		{
			if (GetComponent(ComponentId::Mesh, record.Id) == nullptr || GetComponent(ComponentId::Transform, record.Id) == nullptr || GetComponent(ComponentId::Material, record.Id) == nullptr)
				continue;

			const auto* sorting = static_cast<const SortingComponent*>(GetComponent(ComponentId::Sorting, record.Id));
			draw.push_back({ record.Id, sorting != nullptr ? sorting->Order : 0 });
		}

		std::stable_sort(draw.begin(), draw.end(), [](const DrawItem& left, const DrawItem& right)
		{
			return left.Order < right.Order;
		});

		for (const DrawItem& item : draw)
		{
			const auto* transform = static_cast<const TransformComponent*>(GetComponent(ComponentId::Transform, item.Id));
			const auto* material = static_cast<const MaterialComponent*>(GetComponent(ComponentId::Material, item.Id));
			const auto* mesh = static_cast<const MeshComponent*>(GetComponent(ComponentId::Mesh, item.Id));
			if (transform == nullptr || material == nullptr || mesh == nullptr)
				continue;

			switch (mesh->Type)
			{
				case MeshType::Sprite:
					if (material->UseVertexColors)
					{
						Renderer2D::DrawQuad(
							transform->Local,
							material->Texture,
							material->Tiling,
							material->Colors[0],
							material->Colors[1],
							material->Colors[2],
							material->Colors[3]);
					}
					else
					{
						Renderer2D::DrawQuad(transform->Local, material->Texture, material->Tiling, material->Color);
					}
					break;
				case MeshType::Triangle:
					if (material->UseVertexColors)
						Renderer2D::DrawTriangle(transform->Local, material->Colors[0], material->Colors[1], material->Colors[2]);
					else
						Renderer2D::DrawTriangle(transform->Local, material->Color, material->Color, material->Color);
					break;
				case MeshType::Quad:
					if (material->UseVertexColors)
					{
						Renderer2D::DrawQuad(
							transform->Local,
							material->Texture,
							material->Tiling,
							material->Colors[0],
							material->Colors[1],
							material->Colors[2],
							material->Colors[3]);
					}
					else
					{
						Renderer2D::DrawQuad(transform->Local, material->Color);
					}
					break;
			}
		}
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

		auto readEntity = [this](uint32_t plane, const YAML::Node& entityNode)
		{
			if (!entityNode || !entityNode.IsMap())
				return;

			std::string name = "Entity";
			if (entityNode["name"])
				name = entityNode["name"].as<std::string>();
			if (name.empty())
				name = "Entity";

			Entity entity = CreateEntity(name, plane);
			for (YAML::const_iterator it = entityNode.begin(); it != entityNode.end(); ++it)
			{
				const std::string key = it->first.as<std::string>();
				if (key == "name")
					continue;

				const ComponentOps* ops = FindComponentSection(key);
				if (ops == nullptr || ops->Read == nullptr)
					continue;

				ops->Read(*this, entity.GetId(), it->second);
			}
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

				readEntities(CreatePlane(name), planeNode["entities"]);
			}
		}
		else
		{
			readEntities(CreatePlane("World"), root["entities"]);
		}

		if (m_Planes.empty())
			CreatePlane("World");

		if (m_Name.empty())
			m_Name = "Untitled";

		return true;
	}

	void Scene::Write(std::ostream& output) const
	{
		YAML::Node root;
		root["name"] = m_Name;

		YAML::Node planes(YAML::NodeType::Sequence);
		for (const Plane& plane : m_Planes)
		{
			YAML::Node planeNode;
			planeNode["name"] = plane.Name;

			YAML::Node entities(YAML::NodeType::Sequence);
			for (const Record& record : m_Records)
			{
				if (record.Plane != plane.Id)
					continue;

				YAML::Node entity;
				entity["name"] = record.Name;
				for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
				{
					const ComponentOps* ops = FindComponent(static_cast<ComponentId>(index));
					const void* data = GetComponent(static_cast<ComponentId>(index), record.Id);
					if (ops == nullptr || ops->Write == nullptr || ops->Section == nullptr || data == nullptr)
						continue;

					YAML::Node component(YAML::NodeType::Map);
					ops->Write(component, data);
					entity[ops->Section] = component;
				}
				entities.push_back(entity);
			}

			planeNode["entities"] = entities;
			planes.push_back(planeNode);
		}

		root["planes"] = planes;
		output << root;
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
