#include "Scene.h"

#include "Console.h"

#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Core/FileSystem.h"
#include "Lite/Renderer/Renderer2D.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace Lite {

	namespace {

		Scene* s_Active = nullptr;

		std::string Trim(std::string_view value)
		{
			size_t begin = 0;
			while (begin < value.size() && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r'))
				++begin;

			size_t end = value.size();
			while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r'))
				--end;

			return std::string(value.substr(begin, end - begin));
		}

		const char* MeshName(MeshType type)
		{
			switch (type)
			{
				case MeshType::Sprite: return "sprite";
				case MeshType::Triangle: return "triangle";
				case MeshType::Quad: return "quad";
			}

			return "quad";
		}

		MeshType ParseMesh(std::string_view type)
		{
			if (type == "sprite")
				return MeshType::Sprite;
			if (type == "triangle")
				return MeshType::Triangle;
			return MeshType::Quad;
		}

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

		void ReadVec3(std::istream& stream, Vec3& value)
		{
			stream >> value.x >> value.y >> value.z;
		}

		void ReadVec4(std::istream& stream, Vec4& value)
		{
			stream >> value.x >> value.y >> value.z >> value.w;
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

	bool Entity::HasTransform() { return GetTransform() != nullptr; }
	bool Entity::HasCamera() { return GetCamera() != nullptr; }
	bool Entity::HasMesh() { return GetMesh() != nullptr; }
	bool Entity::HasMaterial() { return GetMaterial() != nullptr; }
	bool Entity::HasSpin() { return GetSpin() != nullptr; }

	TransformComponent* Entity::GetTransform()
	{
		Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr;
		return record != nullptr && record->Transform ? &record->Transform.value() : nullptr;
	}

	CameraComponent* Entity::GetCamera()
	{
		Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr;
		return record != nullptr && record->Camera ? &record->Camera.value() : nullptr;
	}

	MeshComponent* Entity::GetMesh()
	{
		Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr;
		return record != nullptr && record->Mesh ? &record->Mesh.value() : nullptr;
	}

	MaterialComponent* Entity::GetMaterial()
	{
		Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr;
		return record != nullptr && record->Material ? &record->Material.value() : nullptr;
	}

	SpinComponent* Entity::GetSpin()
	{
		Scene::Record* record = m_Scene != nullptr ? m_Scene->FindRecord(m_Id) : nullptr;
		return record != nullptr && record->Spin ? &record->Spin.value() : nullptr;
	}

	TransformComponent& Entity::AddTransform()
	{
		Scene::Record* record = m_Scene->FindRecord(m_Id);
		if (!record->Transform)
			record->Transform = TransformComponent{};
		return *record->Transform;
	}

	CameraComponent& Entity::AddCamera()
	{
		Scene::Record* record = m_Scene->FindRecord(m_Id);
		if (!record->Camera)
			record->Camera = CameraComponent{};
		return *record->Camera;
	}

	MeshComponent& Entity::AddMesh()
	{
		Scene::Record* record = m_Scene->FindRecord(m_Id);
		if (!record->Mesh)
			record->Mesh = MeshComponent{};
		return *record->Mesh;
	}

	MaterialComponent& Entity::AddMaterial()
	{
		Scene::Record* record = m_Scene->FindRecord(m_Id);
		if (!record->Material)
			record->Material = MaterialComponent{};
		return *record->Material;
	}

	SpinComponent& Entity::AddSpin()
	{
		Scene::Record* record = m_Scene->FindRecord(m_Id);
		if (!record->Spin)
			record->Spin = SpinComponent{};
		return *record->Spin;
	}

	Scene::Scene(std::string name)
		: m_Name(std::move(name))
	{
	}

	Scene::~Scene()
	{
		Stop();
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
		record.Transform = TransformComponent{};
		m_Records.push_back(std::move(record));
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

	Entity Scene::GetPrimaryCamera()
	{
		Entity fallback;
		for (const Record& record : m_Records)
		{
			if (!record.Camera)
				continue;

			Entity entity(this, record.Id);
			if (record.Camera->Primary)
				return entity;
			if (!fallback)
				fallback = entity;
		}

		return fallback;
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
			if (record.Camera)
				record.Camera->Primary = record.Id == id;
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
		Console::Log(std::format("Scene loaded: {}", m_Name));
		for (const Record& record : m_Records)
			Console::Log(std::format("  {}", record.Name));
	}

	void Scene::Play()
	{
		if (!m_Loaded)
			Load();

		if (m_Playback == ScenePlayback::Playing)
			return;

		m_Playback = ScenePlayback::Playing;
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

		m_Playback = ScenePlayback::Stopped;
		Console::Log(std::format("Scene stopped: {}", m_Name));
	}

	void Scene::Update(float seconds)
	{
		if (m_Playback != ScenePlayback::Playing)
			return;

		for (Record& record : m_Records)
		{
			if (!record.Spin || !record.Transform || record.Spin->Rate == 0.0f)
				continue;

			Transform& transform = record.Transform->Local;
			transform.SetRotationZ(transform.GetRotationZ() + record.Spin->Rate * seconds);
		}
	}

	void Scene::Render() const
	{
		for (const Record& record : m_Records)
		{
			if (!record.Mesh || !record.Transform || !record.Material)
				continue;

			const Transform& transform = record.Transform->Local;
			const MaterialComponent& material = *record.Material;
			switch (record.Mesh->Type)
			{
				case MeshType::Sprite:
					if (material.UseVertexColors)
					{
						Renderer2D::DrawQuad(
							transform,
							material.Texture,
							material.Tiling,
							material.Colors[0],
							material.Colors[1],
							material.Colors[2],
							material.Colors[3]);
					}
					else
					{
						Renderer2D::DrawQuad(transform, material.Texture, material.Tiling, material.Color);
					}
					break;
				case MeshType::Triangle:
					Renderer2D::DrawTriangle(transform, material.Colors[0], material.Colors[1], material.Colors[2]);
					break;
				case MeshType::Quad:
					Renderer2D::DrawQuad(transform, material.Color);
					break;
			}
		}
	}

	bool Scene::Read(std::istream& input)
	{
		std::string magic;
		int version = 0;
		input >> magic >> version;
		if (!input || magic != "lite-scene" || version != 1)
			return false;

		std::string rest;
		std::getline(input, rest);

		Record* current = nullptr;
		Section section = Section::None;
		std::string line;
		while (std::getline(input, line))
		{
			line = Trim(line);
			if (line.empty() || line[0] == '#')
				continue;

			std::istringstream stream(line);
			std::string key;
			stream >> key;

			if (key == "name" && current == nullptr)
			{
				std::string name;
				std::getline(stream >> std::ws, name);
				m_Name = Trim(name);
				continue;
			}

			if (key == "entity" || key == "object")
			{
				Entity entity = CreateEntity("Entity");
				current = FindRecord(entity.GetId());
				section = key == "object" ? Section::Legacy : Section::None;
				continue;
			}

			if (current == nullptr)
				continue;

			if (key == "component")
			{
				std::string component;
				stream >> component;
				if (component == "transform")
					section = Section::Transform;
				else if (component == "camera")
				{
					section = Section::Camera;
					if (!current->Camera)
						current->Camera = CameraComponent{};
				}
				else if (component == "mesh")
				{
					section = Section::Mesh;
					if (!current->Mesh)
						current->Mesh = MeshComponent{};
				}
				else if (component == "material")
				{
					section = Section::Material;
					if (!current->Material)
						current->Material = MaterialComponent{};
				}
				else if (component == "spin")
				{
					section = Section::Spin;
					if (!current->Spin)
						current->Spin = SpinComponent{};
				}
				continue;
			}

			if (key == "name" && (section == Section::None || section == Section::Legacy))
			{
				std::string name;
				std::getline(stream >> std::ws, name);
				current->Name = Trim(name);
				continue;
			}

			const bool transformSection = section == Section::Transform || section == Section::Legacy || section == Section::None;
			if (transformSection && current->Transform)
			{
				if (key == "position")
				{
					ReadVec3(stream, current->Transform->Local.Position);
					continue;
				}
				if (key == "rotation")
				{
					float radians = 0.0f;
					stream >> radians;
					current->Transform->Local.SetRotationZ(radians);
					continue;
				}
				if (key == "scale")
				{
					ReadVec3(stream, current->Transform->Local.Scale);
					continue;
				}
			}

			if (section == Section::Camera && current->Camera)
			{
				if (key == "size")
					stream >> current->Camera->Size;
				else if (key == "near")
					stream >> current->Camera->Near;
				else if (key == "far")
					stream >> current->Camera->Far;
				else if (key == "primary")
				{
					int primary = 0;
					stream >> primary;
					current->Camera->Primary = primary != 0;
				}
				continue;
			}

			if ((section == Section::Mesh || section == Section::Legacy) && (key == "type" || key == "kind"))
			{
				if (!current->Mesh)
					current->Mesh = MeshComponent{};
				std::string type;
				stream >> type;
				current->Mesh->Type = ParseMesh(type);
				continue;
			}

			if (section == Section::Material || section == Section::Legacy || section == Section::Spin)
			{
				if (key == "shader" || key == "color" || key == "tiling" || key == "texture" || key == "vertex-colors" || key == "colors" || key == "corners")
				{
					if (!current->Material)
						current->Material = MaterialComponent{};
				}

				if (current->Material)
				{
					if (key == "shader")
					{
						std::string shader;
						std::getline(stream >> std::ws, shader);
						current->Material->Shader = Trim(shader);
						continue;
					}
					if (key == "color")
					{
						ReadVec4(stream, current->Material->Color);
						continue;
					}
					if (key == "tiling")
					{
						stream >> current->Material->Tiling.x >> current->Material->Tiling.y;
						continue;
					}
					if (key == "texture")
					{
						std::string texture;
						std::getline(stream >> std::ws, texture);
						current->Material->TexturePath = Trim(texture);
						continue;
					}
					if (key == "vertex-colors")
					{
						int enabled = 0;
						stream >> enabled;
						current->Material->UseVertexColors = enabled != 0;
						continue;
					}
					if (key == "colors" || key == "corners")
					{
						current->Material->UseVertexColors = true;
						for (Vec4& color : current->Material->Colors)
							ReadVec4(stream, color);
						continue;
					}
				}
			}

			if ((section == Section::Spin || section == Section::Legacy) && (key == "rate" || key == "spin"))
			{
				float rate = 0.0f;
				stream >> rate;
				if (section == Section::Spin || rate != 0.0f)
				{
					if (!current->Spin)
						current->Spin = SpinComponent{};
					current->Spin->Rate = rate;
				}
			}
		}

		if (m_Name.empty())
			m_Name = "Untitled";

		for (Record& record : m_Records)
		{
			if (record.Name.empty())
				record.Name = "Entity";
			if (record.Material && record.Material->Shader.empty())
				record.Material->Shader = "Batch";
		}

		return true;
	}

	void Scene::Write(std::ostream& output) const
	{
		output << "lite-scene 1\n";
		output << std::format("name {}\n", m_Name);
		for (const Record& record : m_Records)
		{
			output << "entity\n";
			output << std::format("name {}\n", record.Name);
			if (record.Transform)
			{
				const Vec3& position = record.Transform->Local.Position;
				const Vec3& scale = record.Transform->Local.Scale;
				output << "component transform\n";
				output << std::format("position {:.4f} {:.4f} {:.4f}\n", position.x, position.y, position.z);
				output << std::format("rotation {:.4f}\n", record.Transform->Local.GetRotationZ());
				output << std::format("scale {:.4f} {:.4f} {:.4f}\n", scale.x, scale.y, scale.z);
			}
			if (record.Camera)
			{
				output << "component camera\n";
				output << std::format("size {:.4f}\n", record.Camera->Size);
				output << std::format("near {:.4f}\n", record.Camera->Near);
				output << std::format("far {:.4f}\n", record.Camera->Far);
				output << std::format("primary {}\n", record.Camera->Primary ? 1 : 0);
			}
			if (record.Mesh)
			{
				output << "component mesh\n";
				output << std::format("type {}\n", MeshName(record.Mesh->Type));
			}
			if (record.Material)
			{
				const MaterialComponent& material = *record.Material;
				output << "component material\n";
				output << std::format("shader {}\n", material.Shader.empty() ? "Batch" : material.Shader);
				output << std::format("color {:.4f} {:.4f} {:.4f} {:.4f}\n", material.Color.x, material.Color.y, material.Color.z, material.Color.w);
				output << std::format("tiling {:.4f} {:.4f}\n", material.Tiling.x, material.Tiling.y);
				if (!material.TexturePath.empty())
					output << std::format("texture {}\n", material.TexturePath);
				output << std::format("vertex-colors {}\n", material.UseVertexColors ? 1 : 0);
				if (material.UseVertexColors)
				{
					output << "colors";
					for (const Vec4& color : material.Colors)
						output << std::format(" {:.4f} {:.4f} {:.4f} {:.4f}", color.x, color.y, color.z, color.w);
					output << '\n';
				}
			}
			if (record.Spin)
			{
				output << "component spin\n";
				output << std::format("rate {:.4f}\n", record.Spin->Rate);
			}
		}
	}

	void Scene::ResolveTextures()
	{
		for (Record& record : m_Records)
		{
			if (!record.Material || record.Material->TexturePath.empty())
				continue;

			record.Material->Texture = AssetRegistry::Get().Load<Texture>(record.Material->TexturePath);
			if (!record.Material->Texture)
				Console::Log(std::format("Failed to load texture {}", record.Material->TexturePath));
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

		m_Path = full.string();
		Console::Log(std::format("Scene saved: {}", m_Path));
		return true;
	}

	Scope<Scene> Scene::Open(std::string_view path)
	{
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

		scene->m_Path = std::string(path);
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
