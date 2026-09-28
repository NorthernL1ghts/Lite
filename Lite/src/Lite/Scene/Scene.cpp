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

		std::string SafeName(std::string_view name)
		{
			std::string safe;
			safe.reserve(name.size());
			for (char character : name)
			{
				switch (character)
				{
					case '<':
					case '>':
					case ':':
					case '"':
					case '/':
					case '\\':
					case '|':
					case '?':
					case '*':
						continue;
					default:
						safe.push_back(character);
						break;
				}
			}

			safe = Trim(safe);
			if (safe.empty())
				safe = "Untitled";
			return safe;
		}

		const char* KindName(SceneObjectKind kind)
		{
			switch (kind)
			{
				case SceneObjectKind::Sprite: return "sprite";
				case SceneObjectKind::Triangle: return "triangle";
				case SceneObjectKind::Quad: return "quad";
			}

			return "quad";
		}

		SceneObjectKind ParseKind(std::string_view kind)
		{
			if (kind == "sprite")
				return SceneObjectKind::Sprite;
			if (kind == "triangle")
				return SceneObjectKind::Triangle;
			return SceneObjectKind::Quad;
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

	SceneObject* Scene::Find(std::string_view name)
	{
		for (SceneObject& object : m_Objects)
		{
			if (object.Name == name)
				return &object;
		}

		return nullptr;
	}

	void Scene::AddObject(SceneObject object)
	{
		m_Objects.push_back(std::move(object));
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
		for (const SceneObject& object : m_Objects)
			Console::Log(std::format("  {}", object.Name));
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

		for (SceneObject& object : m_Objects)
		{
			if (object.Spin == 0.0f)
				continue;

			object.Transform.SetRotationZ(object.Transform.GetRotationZ() + object.Spin * seconds);
		}
	}

	void Scene::Render() const
	{
		for (const SceneObject& object : m_Objects)
		{
			switch (object.Kind)
			{
				case SceneObjectKind::Sprite:
					if (object.UseCornerColors)
					{
						Renderer2D::DrawQuad(
							object.Transform,
							object.Texture,
							object.Tiling,
							object.CornerColors[0],
							object.CornerColors[1],
							object.CornerColors[2],
							object.CornerColors[3]);
					}
					else
					{
						Renderer2D::DrawQuad(object.Transform, object.Texture, object.Tiling, object.Color);
					}
					break;
				case SceneObjectKind::Triangle:
					Renderer2D::DrawTriangle(
						object.Transform,
						object.CornerColors[0],
						object.CornerColors[1],
						object.CornerColors[2]);
					break;
				case SceneObjectKind::Quad:
					Renderer2D::DrawQuad(object.Transform, object.Color);
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

		SceneObject* current = nullptr;
		std::string line;
		while (std::getline(input, line))
		{
			line = Trim(line);
			if (line.empty() || line[0] == '#')
				continue;

			std::istringstream stream(line);
			std::string key;
			stream >> key;
			if (key == "object")
			{
				m_Objects.emplace_back();
				current = &m_Objects.back();
				continue;
			}

			if (key == "name" && current == nullptr)
			{
				std::string name;
				std::getline(stream >> std::ws, name);
				m_Name = Trim(name);
				continue;
			}

			if (current == nullptr)
				continue;

			if (key == "name")
			{
				std::string name;
				std::getline(stream >> std::ws, name);
				current->Name = Trim(name);
			}
			else if (key == "kind")
			{
				std::string kind;
				stream >> kind;
				current->Kind = ParseKind(kind);
			}
			else if (key == "position")
			{
				stream >> current->Transform.Position.x >> current->Transform.Position.y >> current->Transform.Position.z;
			}
			else if (key == "rotation")
			{
				float radians = 0.0f;
				stream >> radians;
				current->Transform.SetRotationZ(radians);
			}
			else if (key == "scale")
			{
				stream >> current->Transform.Scale.x >> current->Transform.Scale.y >> current->Transform.Scale.z;
			}
			else if (key == "color")
			{
				stream >> current->Color.x >> current->Color.y >> current->Color.z >> current->Color.w;
			}
			else if (key == "tiling")
			{
				stream >> current->Tiling.x >> current->Tiling.y;
			}
			else if (key == "texture")
			{
				std::string texture;
				std::getline(stream >> std::ws, texture);
				current->TexturePath = Trim(texture);
			}
			else if (key == "spin")
			{
				stream >> current->Spin;
			}
			else if (key == "corners")
			{
				current->UseCornerColors = true;
				for (Vec4& corner : current->CornerColors)
					stream >> corner.x >> corner.y >> corner.z >> corner.w;
			}
		}

		if (m_Name.empty())
			m_Name = "Untitled";

		for (SceneObject& object : m_Objects)
		{
			if (object.Name.empty())
				object.Name = "Object";
		}

		return true;
	}

	void Scene::Write(std::ostream& output) const
	{
		output << "lite-scene 1\n";
		output << std::format("name {}\n", m_Name);
		for (const SceneObject& object : m_Objects)
		{
			const Vec3& position = object.Transform.Position;
			const Vec3& scale = object.Transform.Scale;
			output << "object\n";
			output << std::format("name {}\n", object.Name);
			output << std::format("kind {}\n", KindName(object.Kind));
			output << std::format("position {:.4f} {:.4f} {:.4f}\n", position.x, position.y, position.z);
			output << std::format("rotation {:.4f}\n", object.Transform.GetRotationZ());
			output << std::format("scale {:.4f} {:.4f} {:.4f}\n", scale.x, scale.y, scale.z);
			output << std::format("color {:.4f} {:.4f} {:.4f} {:.4f}\n", object.Color.x, object.Color.y, object.Color.z, object.Color.w);
			output << std::format("tiling {:.4f} {:.4f}\n", object.Tiling.x, object.Tiling.y);
			if (!object.TexturePath.empty())
				output << std::format("texture {}\n", object.TexturePath);
			output << std::format("spin {:.4f}\n", object.Spin);
			if (object.UseCornerColors)
			{
				output << "corners";
				for (const Vec4& corner : object.CornerColors)
					output << std::format(" {:.4f} {:.4f} {:.4f} {:.4f}", corner.x, corner.y, corner.z, corner.w);
				output << '\n';
			}
		}
	}

	void Scene::ResolveTextures()
	{
		for (SceneObject& object : m_Objects)
		{
			if (object.TexturePath.empty())
				continue;

			object.Texture = AssetRegistry::Get().Load<Texture>(object.TexturePath);
			if (!object.Texture)
				Console::Log(std::format("Failed to load texture {}", object.TexturePath));
		}
	}

	bool Scene::Save()
	{
		std::string relative = std::format("assets/scenes/{}.scene", SafeName(m_Name));
		std::filesystem::path full = ResolvePath(relative);
		std::error_code error;
		std::filesystem::create_directories(full.parent_path(), error);
		std::ofstream file(full, std::ios::trunc);
		if (!file)
		{
			Console::Log(std::format("Failed to save scene: {}", relative));
			return false;
		}

		Write(file);
		if (!file)
		{
			Console::Log(std::format("Failed to save scene: {}", relative));
			return false;
		}

		m_Path = relative;
		Console::Log(std::format("Scene saved: {}", relative));
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
