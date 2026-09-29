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

		void Consider(const std::filesystem::path& candidate, std::vector<std::filesystem::path>& matches)
		{
			std::error_code error;
			if (!std::filesystem::is_regular_file(candidate, error) && !std::filesystem::is_directory(candidate, error))
				return;

			std::filesystem::path normal = candidate.lexically_normal();
			for (const std::filesystem::path& match : matches)
			{
				if (match == normal)
					return;
			}

			matches.push_back(std::move(normal));
		}

		void Collect(const std::filesystem::path& start, const std::filesystem::path& relative, std::vector<std::filesystem::path>& matches)
		{
			std::filesystem::path cursor = start;
			for (int step = 0; step < 8 && !cursor.empty(); ++step)
			{
				Consider(cursor / relative, matches);

				std::error_code error;
				if (std::filesystem::is_directory(cursor, error))
				{
					for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(cursor, error))
					{
						std::error_code childError;
						if (error || !entry.is_directory(childError))
							continue;
						Consider(entry.path() / relative, matches);
					}
				}

				std::filesystem::path parent = cursor.parent_path();
				if (parent.empty() || parent == cursor)
					break;
				cursor = parent;
			}
		}

		std::filesystem::path PreferSource(const std::vector<std::filesystem::path>& matches)
		{
			std::filesystem::path executable = FileSystem::ExecutableDirectory();
			std::filesystem::path inside;
			for (const std::filesystem::path& match : matches)
			{
				std::error_code error;
				std::filesystem::path relative = std::filesystem::relative(match, executable, error);
				bool contained = !error && (relative.empty() || relative.begin()->string() != "..");
				if (!contained)
					return match;
				if (inside.empty())
					inside = match;
			}

			return inside;
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

	Entity Scene::CreateEntity(std::string name)
	{
		Record record;
		record.Id = m_NextId++;
		record.Name = std::move(name);
		m_Records.push_back(std::move(record));
		AddComponent(ComponentId::Transform, m_Records.back().Id);
		return Entity(this, m_Records.back().Id);
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
		const Record* fallback = nullptr;
		for (const Record& record : m_Records)
		{
			const auto* camera = static_cast<const CameraComponent*>(GetComponent(ComponentId::Camera, record.Id));
			if (camera == nullptr)
				continue;
			if (camera->Primary)
				return &record;
			if (fallback == nullptr)
				fallback = &record;
		}

		return fallback;
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
		for (const Record& record : m_Records)
			entities.emplace_back(this, record.Id);
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
		for (const Record& record : m_Records)
			Console::Log(std::format("  {}", record.Name));
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
					Renderer2D::DrawTriangle(transform->Local, material->Colors[0], material->Colors[1], material->Colors[2]);
					break;
				case MeshType::Quad:
					Renderer2D::DrawQuad(transform->Local, material->Color);
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

		const YAML::Node entities = root["entities"];
		if (entities && entities.IsSequence())
		{
			for (const YAML::Node& entityNode : entities)
			{
				if (!entityNode || !entityNode.IsMap())
					continue;

				std::string name = "Entity";
				if (entityNode["name"])
					name = entityNode["name"].as<std::string>();
				if (name.empty())
					name = "Entity";

				Entity entity = CreateEntity(name);
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
			}
		}

		if (m_Name.empty())
			m_Name = "Untitled";

		return true;
	}

	void Scene::Write(std::ostream& output) const
	{
		YAML::Node root;
		root["name"] = m_Name;

		YAML::Node entities(YAML::NodeType::Sequence);
		for (const Record& record : m_Records)
		{
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

		root["entities"] = entities;
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
		std::filesystem::path relative(relativeToProject);
		std::vector<std::filesystem::path> matches;
		std::error_code error;
		Collect(std::filesystem::current_path(error), relative, matches);
		Collect(FileSystem::ExecutableDirectory(), relative, matches);

		std::filesystem::path chosen = PreferSource(matches);
		if (chosen.empty())
			return {};

		std::filesystem::path canonical = std::filesystem::weakly_canonical(chosen, error);
		return (error ? chosen : canonical).string();
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
