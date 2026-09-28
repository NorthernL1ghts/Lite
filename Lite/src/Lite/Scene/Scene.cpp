#include <Lite/Scene/Scene.h>

#include <Lite/Scene/Console.h>

#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Renderer/Renderer2D.h>

#include <algorithm>
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

	void* Scene::AddComponent(ComponentId id, uint32_t entity)
	{
		Record* record = FindRecord(entity);
		if (record == nullptr)
			return nullptr;

		switch (id)
		{
			case ComponentId::Transform:
				if (!record->Transform)
					record->Transform = TransformComponent{};
				return &record->Transform.value();
			case ComponentId::Camera:
				if (!record->Camera)
					record->Camera = CameraComponent{};
				return &record->Camera.value();
			case ComponentId::Mesh:
				if (!record->Mesh)
					record->Mesh = MeshComponent{};
				return &record->Mesh.value();
			case ComponentId::Material:
				if (!record->Material)
					record->Material = MaterialComponent{};
				return &record->Material.value();
			case ComponentId::Spin:
				if (!record->Spin)
					record->Spin = SpinComponent{};
				return &record->Spin.value();
			case ComponentId::Rigidbody2D:
				if (!record->Rigidbody2D)
					record->Rigidbody2D = Rigidbody2DComponent{};
				return &record->Rigidbody2D.value();
			case ComponentId::BoxCollider2D:
				if (!record->BoxCollider2D)
					record->BoxCollider2D = BoxCollider2DComponent{};
				return &record->BoxCollider2D.value();
			case ComponentId::CircleCollider2D:
				if (!record->CircleCollider2D)
					record->CircleCollider2D = CircleCollider2DComponent{};
				return &record->CircleCollider2D.value();
			case ComponentId::Sorting:
				if (!record->Sorting)
					record->Sorting = SortingComponent{};
				return &record->Sorting.value();
		}

		return nullptr;
	}

	void* Scene::GetComponent(ComponentId id, uint32_t entity)
	{
		Record* record = FindRecord(entity);
		if (record == nullptr)
			return nullptr;

		switch (id)
		{
			case ComponentId::Transform: return record->Transform ? &record->Transform.value() : nullptr;
			case ComponentId::Camera: return record->Camera ? &record->Camera.value() : nullptr;
			case ComponentId::Mesh: return record->Mesh ? &record->Mesh.value() : nullptr;
			case ComponentId::Material: return record->Material ? &record->Material.value() : nullptr;
			case ComponentId::Spin: return record->Spin ? &record->Spin.value() : nullptr;
			case ComponentId::Rigidbody2D: return record->Rigidbody2D ? &record->Rigidbody2D.value() : nullptr;
			case ComponentId::BoxCollider2D: return record->BoxCollider2D ? &record->BoxCollider2D.value() : nullptr;
			case ComponentId::CircleCollider2D: return record->CircleCollider2D ? &record->CircleCollider2D.value() : nullptr;
			case ComponentId::Sorting: return record->Sorting ? &record->Sorting.value() : nullptr;
		}

		return nullptr;
	}

	void Scene::RemoveComponent(ComponentId id, uint32_t entity)
	{
		Record* record = FindRecord(entity);
		if (record == nullptr)
			return;

		switch (id)
		{
			case ComponentId::Transform: record->Transform.reset(); break;
			case ComponentId::Camera: record->Camera.reset(); break;
			case ComponentId::Mesh: record->Mesh.reset(); break;
			case ComponentId::Material: record->Material.reset(); break;
			case ComponentId::Spin: record->Spin.reset(); break;
			case ComponentId::Rigidbody2D: record->Rigidbody2D.reset(); break;
			case ComponentId::BoxCollider2D: record->BoxCollider2D.reset(); break;
			case ComponentId::CircleCollider2D: record->CircleCollider2D.reset(); break;
			case ComponentId::Sorting: record->Sorting.reset(); break;
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
			if (!record.Camera)
				continue;
			if (record.Camera->Primary)
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
		if (record == nullptr || !record->Transform || !record->Camera)
			return CameraProjectionMatrix(fallbackCamera, aspect) * fallbackTransform.GetViewMatrix();

		return CameraProjectionMatrix(*record->Camera, aspect) * record->Transform->Local.GetViewMatrix();
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
		std::vector<const Record*> draw;
		for (const Record& record : m_Records)
		{
			if (record.Mesh && record.Transform && record.Material)
				draw.push_back(&record);
		}

		std::stable_sort(draw.begin(), draw.end(), [](const Record* left, const Record* right)
		{
			int leftOrder = left->Sorting ? left->Sorting->Order : 0;
			int rightOrder = right->Sorting ? right->Sorting->Order : 0;
			return leftOrder < rightOrder;
		});

		for (const Record* record : draw)
		{
			const Transform& transform = record->Transform->Local;
			const MaterialComponent& material = *record->Material;
			switch (record->Mesh->Type)
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
				else if (component == "rigidbody2d")
				{
					section = Section::Rigidbody2D;
					if (!current->Rigidbody2D)
						current->Rigidbody2D = Rigidbody2DComponent{};
				}
				else if (component == "box-collider2d")
				{
					section = Section::BoxCollider2D;
					if (!current->BoxCollider2D)
						current->BoxCollider2D = BoxCollider2DComponent{};
				}
				else if (component == "circle-collider2d")
				{
					section = Section::CircleCollider2D;
					if (!current->CircleCollider2D)
						current->CircleCollider2D = CircleCollider2DComponent{};
				}
				else if (component == "sorting")
				{
					section = Section::Sorting;
					if (!current->Sorting)
						current->Sorting = SortingComponent{};
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
				else if (key == "projection")
				{
					std::string projection;
					stream >> projection;
					current->Camera->Projection = projection == "perspective"
						? CameraProjection::Perspective
						: CameraProjection::Orthographic;
				}
				else if (key == "fov")
					stream >> current->Camera->FieldOfView;
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
				continue;
			}

			if (section == Section::Rigidbody2D && current->Rigidbody2D)
			{
				Rigidbody2DComponent& body = *current->Rigidbody2D;
				if (key == "type")
				{
					std::string type;
					stream >> type;
					if (type == "static")
						body.Type = BodyType::Static;
					else if (type == "kinematic")
						body.Type = BodyType::Kinematic;
					else
						body.Type = BodyType::Dynamic;
				}
				else if (key == "mass")
					stream >> body.Mass;
				else if (key == "gravity")
					stream >> body.GravityScale;
				else if (key == "velocity")
					stream >> body.LinearVelocity.x >> body.LinearVelocity.y;
				else if (key == "angular")
					stream >> body.AngularVelocity;
				else if (key == "freeze")
				{
					int freeze = 0;
					stream >> freeze;
					body.FreezeRotation = freeze != 0;
				}
				continue;
			}

			if (section == Section::BoxCollider2D && current->BoxCollider2D)
			{
				BoxCollider2DComponent& box = *current->BoxCollider2D;
				if (key == "size")
					stream >> box.Size.x >> box.Size.y;
				else if (key == "offset")
					stream >> box.Offset.x >> box.Offset.y;
				else if (key == "trigger")
				{
					int trigger = 0;
					stream >> trigger;
					box.IsTrigger = trigger != 0;
				}
				continue;
			}

			if (section == Section::CircleCollider2D && current->CircleCollider2D)
			{
				CircleCollider2DComponent& circle = *current->CircleCollider2D;
				if (key == "radius")
					stream >> circle.Radius;
				else if (key == "offset")
					stream >> circle.Offset.x >> circle.Offset.y;
				else if (key == "trigger")
				{
					int trigger = 0;
					stream >> trigger;
					circle.IsTrigger = trigger != 0;
				}
				continue;
			}

			if (section == Section::Sorting && current->Sorting && key == "order")
				stream >> current->Sorting->Order;
		}

		if (m_Name.empty())
			m_Name = "Untitled";

		for (Record& record : m_Records)
		{
			if (record.Name.empty())
				record.Name = "Entity";
			if (record.Material && record.Material->Shader.empty())
				record.Material->Shader = kDefaultShader;
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
				output << std::format("projection {}\n", record.Camera->Projection == CameraProjection::Perspective ? "perspective" : "orthographic");
				output << std::format("size {:.4f}\n", record.Camera->Size);
				output << std::format("fov {:.4f}\n", record.Camera->FieldOfView);
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
				output << std::format("shader {}\n", material.Shader.empty() ? kDefaultShader : material.Shader);
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
			if (record.Rigidbody2D)
			{
				const char* type = "dynamic";
				if (record.Rigidbody2D->Type == BodyType::Static)
					type = "static";
				else if (record.Rigidbody2D->Type == BodyType::Kinematic)
					type = "kinematic";
				const Rigidbody2DComponent& body = *record.Rigidbody2D;
				output << "component rigidbody2d\n";
				output << std::format("type {}\n", type);
				output << std::format("mass {:.4f}\n", body.Mass);
				output << std::format("gravity {:.4f}\n", body.GravityScale);
				output << std::format("velocity {:.4f} {:.4f}\n", body.LinearVelocity.x, body.LinearVelocity.y);
				output << std::format("angular {:.4f}\n", body.AngularVelocity);
				output << std::format("freeze {}\n", body.FreezeRotation ? 1 : 0);
			}
			if (record.BoxCollider2D)
			{
				const BoxCollider2DComponent& box = *record.BoxCollider2D;
				output << "component box-collider2d\n";
				output << std::format("size {:.4f} {:.4f}\n", box.Size.x, box.Size.y);
				output << std::format("offset {:.4f} {:.4f}\n", box.Offset.x, box.Offset.y);
				output << std::format("trigger {}\n", box.IsTrigger ? 1 : 0);
			}
			if (record.CircleCollider2D)
			{
				const CircleCollider2DComponent& circle = *record.CircleCollider2D;
				output << "component circle-collider2d\n";
				output << std::format("radius {:.4f}\n", circle.Radius);
				output << std::format("offset {:.4f} {:.4f}\n", circle.Offset.x, circle.Offset.y);
				output << std::format("trigger {}\n", circle.IsTrigger ? 1 : 0);
			}
			if (record.Sorting)
			{
				output << "component sorting\n";
				output << std::format("order {}\n", record.Sorting->Order);
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
