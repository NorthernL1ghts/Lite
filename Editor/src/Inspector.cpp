#include <Inspector.h>

#include <SceneHistory.h>

#include <Lite/Project/Project.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>

namespace {

	void ColorField(const char* label, Lite::Vec4& color)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::ColorEdit4(label, &color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	}

	bool ComponentHeader(const char* label, const char* id)
	{
		ImGui::Separator();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		const float width = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::SameLine();
		const float spare = ImGui::GetContentRegionAvail().x - width;
		if (spare > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spare);
		ImGui::PushID(id);
		const bool pressed = ImGui::SmallButton("Remove");
		ImGui::PopID();
		return pressed;
	}

	void SaveMaterial(Lite::Scene& scene, Lite::Entity entity)
	{
		if (Lite::Project::GetActive() == nullptr)
		{
			Lite::Console::Log("Failed to save material: open a project first");
			return;
		}
		if (!entity || !entity.Has<Lite::MaterialComponent>())
		{
			Lite::Console::Log("Failed to save material: the entity has no material");
			return;
		}

		std::string name = entity.GetName();
		if (name.empty())
			name = "Material";
		for (char& character : name)
		{
			if (std::string("\\/:*?\"<>|").find(character) != std::string::npos)
				character = '_';
		}

		Lite::Project::EnsureContentFolders();
		std::filesystem::path file = Lite::Project::GetAssetDirectory() / "materials" / (name + ".material");
		scene.SaveMaterial(entity.GetId(), file);
	}

	bool SurfaceSlider(const char* label, float& value, float min, float max)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine(110.0f);
		ImGui::SetNextItemWidth(-1.0f);
		return ImGui::SliderFloat(std::format("##{}", label).c_str(), &value, min, max, "%.2f");
	}

	const char* EntryLabel(Lite::Entity entity, const char* name, const char* fallback)
	{
		size_t count = 0;
		const Lite::ComponentEntry* catalog = Lite::ComponentCatalog(count);
		for (size_t index = 0; index < count; ++index)
		{
			if (std::strcmp(catalog[index].Name, name) != 0)
				continue;
			if (const char* label = catalog[index].Label(entity))
				return label;
		}

		return fallback;
	}

}

void Inspector::Reset()
{
	m_SyncedId = 0;
}

void Inspector::ApplyTexture(std::uint32_t selected, const std::string& path)
{
	Lite::Scene* scene = Lite::Scene::GetActive();
	const Lite::Scene::TextureAssign assigned = scene != nullptr ? scene->AssignTexture(selected, path) : Lite::Scene::TextureAssign {};
	if (!assigned.Applied)
	{
		Lite::Console::Log("Drop a texture onto an entity with a material");
		return;
	}

	if (assigned.Applied)
		std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", assigned.Path.c_str());
	if (!path.empty() && !assigned.Loaded)
		Lite::Console::Log(std::format("Failed to load texture {}", path));
	else
		Lite::Console::Log(std::format("Texture set: {}", path));
}

void Inspector::Draw(std::uint32_t selected, SceneHistory& history)
{
	ImGui::Begin("Inspector");
	Lite::Scene* scene = Lite::Scene::GetActive();
	Lite::Entity entity = scene != nullptr ? scene->GetEntity(selected) : Lite::Entity{};
	if (!entity)
	{
		ImGui::TextDisabled("No selection");
		ImGui::End();
		return;
	}

	if (m_SyncedId != entity.GetId())
	{
		m_SyncedId = entity.GetId();
		std::snprintf(m_ObjectName, sizeof(m_ObjectName), "%s", entity.GetName().c_str());
		Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>();
		std::snprintf(m_ShaderText, sizeof(m_ShaderText), "%s", material != nullptr ? material->Shader.c_str() : "");
		std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", material != nullptr ? material->TexturePath.c_str() : "");
		Lite::ScriptComponent* script = entity.Get<Lite::ScriptComponent>();
		std::snprintf(m_ScriptText, sizeof(m_ScriptText), "%s", script != nullptr ? script->Class.c_str() : "");
	}

	auto refreshPhysics = [&]()
	{
		scene->RefreshPhysics(entity.GetId());
	};

	auto removed = [&](const char* label)
	{
		refreshPhysics();
		m_SyncedId = 0;
		history.Commit(*scene, entity.GetId());
		Lite::Console::Log(std::format("Removed {}", label));
	};

	auto finish = [&]()
	{
		if (ImGui::IsItemDeactivatedAfterEdit())
			history.Commit(*scene, entity.GetId());
	};

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##ObjectName", m_ObjectName, sizeof(m_ObjectName)))
		entity.SetName(m_ObjectName);
	finish();

	const std::string prefab = scene->GetPrefab(entity.GetId());
	if (!prefab.empty())
		ImGui::TextDisabled("Prefab: %s", prefab.c_str());
	if (Lite::Entity parent = scene->GetEntity(scene->GetParent(entity.GetId())))
		ImGui::TextDisabled("Parent: %s", parent.GetName().c_str());

	if (Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>())
	{
		if (ComponentHeader("Transform", "Transform"))
		{
			entity.Remove<Lite::TransformComponent>();
			removed("Transform");
		}
		else
		{
			if (ImGui::DragFloat3("Position", &transform->Local.Position.x, 0.01f))
				refreshPhysics();
			finish();
			float rotation = transform->Local.GetRotationZ();
			if (ImGui::DragFloat("Rotation", &rotation, 0.01f))
			{
				transform->Local.SetRotationZ(rotation);
				refreshPhysics();
			}
			finish();
			if (ImGui::DragFloat3("Scale", &transform->Local.Scale.x, 0.01f))
				refreshPhysics();
			finish();
		}
	}

	if (Lite::CameraComponent* camera = entity.Get<Lite::CameraComponent>())
	{
		const char* cameraLabel = EntryLabel(entity, "Orthographic Camera", "Orthographic Camera");
		if (ComponentHeader(cameraLabel, "Camera"))
		{
			entity.Remove<Lite::CameraComponent>();
			removed(cameraLabel);
		}
		else
		{
		const char* projections[] = { "Orthographic", "Perspective" };
		int projection = static_cast<int>(camera->Projection);
		if (ImGui::Combo("Projection", &projection, projections, 2))
		{
			camera->Projection = static_cast<Lite::CameraProjection>(projection);
			if (camera->Projection == Lite::CameraProjection::Perspective && camera->Near <= 0.0f)
			{
				camera->Near = 0.1f;
				if (camera->Far <= camera->Near)
					camera->Far = 100.0f;
			}
		}
		finish();
		if (camera->Projection == Lite::CameraProjection::Orthographic)
			ImGui::DragFloat("Size", &camera->Size, 0.01f, 0.25f, 12.0f);
		else
		{
			float degrees = camera->FieldOfView * (180.0f / 3.14159265f);
			if (ImGui::DragFloat("Field of view", &degrees, 0.1f, 1.0f, 179.0f))
				camera->FieldOfView = degrees * (3.14159265f / 180.0f);
			finish();
		}
		finish();
		ImGui::DragFloat("Near", &camera->Near, 0.01f);
		finish();
		ImGui::DragFloat("Far", &camera->Far, 0.01f);
		finish();
		bool primary = camera->Primary;
		if (ImGui::Checkbox("Primary", &primary))
		{
			if (primary)
				scene->SetPrimaryCamera(entity.GetId());
			else
				camera->Primary = false;
		}
		finish();
		}
	}

	if (Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>())
	{
		const char* meshLabel = EntryLabel(entity, "Quad", "Quad");
		if (ComponentHeader(meshLabel, "Mesh"))
		{
			entity.Remove<Lite::MeshComponent>();
			removed(meshLabel);
		}
		else
		{
			const char* types[] = { "Quad", "Triangle", "Sprite" };
			int current = static_cast<int>(mesh->Type);
			if (ImGui::Combo("Type", &current, types, 3))
				mesh->Type = static_cast<Lite::MeshType>(current);
			finish();
		}
	}

	if (Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>())
	{
		ImGui::BeginGroup();
		if (ComponentHeader("Material", "Material"))
		{
			entity.Remove<Lite::MaterialComponent>();
			removed("Material");
		}
		else
		{
		Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>();
		const bool triangle = mesh != nullptr && mesh->Type == Lite::MeshType::Triangle;
		if (!material->UseVertexColors)
		{
			ColorField("Color", material->Color);
			finish();
		}

		float opacity = material->Color.w;
		if (SurfaceSlider("Opacity", opacity, 0.0f, 1.0f))
			material->Color.w = opacity;
		finish();
		if (SurfaceSlider("Roughness", material->Roughness, 0.0f, 1.0f))
			material->Roughness = std::clamp(material->Roughness, 0.0f, 1.0f);
		finish();
		if (SurfaceSlider("Metallic", material->Metallic, 0.0f, 1.0f))
			material->Metallic = std::clamp(material->Metallic, 0.0f, 1.0f);
		finish();
		if (SurfaceSlider("Emission", material->Emission, 0.0f, 4.0f))
			material->Emission = std::max(material->Emission, 0.0f);
		finish();
		ImGui::TextDisabled("0 roughness is smooth. 1 is matte. Emission adds glow.");

		ImGui::Checkbox("Vertex colors", &material->UseVertexColors);
		finish();
		if (material->UseVertexColors)
		{
			const char* quadNames[] = { "Bottom left", "Bottom right", "Top right", "Top left" };
			const char* triangleNames[] = { "Bottom", "Upper left", "Upper right" };
			int colors = triangle ? 3 : 4;
			for (int index = 0; index < colors; ++index)
			{
				const char* name = triangle ? triangleNames[index] : quadNames[index];
				ColorField(name, material->Colors[index]);
				finish();
			}
		}

		ImGui::DragFloat2("Tiling", &material->Tiling.x, 0.01f, 0.0f, 64.0f);
		finish();
		ImGui::DragFloat2("Offset", &material->Offset.x, 0.01f);
		finish();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Texture");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##Texture", m_TextureText, sizeof(m_TextureText)))
			scene->AssignTexture(entity.GetId(), m_TextureText);
		if (ImGui::IsItemDeactivatedAfterEdit())
			std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", material->TexturePath.c_str());
		finish();
		ImGui::TextDisabled("Drop a texture or a .material file on this section.");

		if (ImGui::Button("Save Material"))
			SaveMaterial(*scene, entity);
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Shader");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##Shader", m_ShaderText, sizeof(m_ShaderText)))
			material->Shader = m_ShaderText;
		finish();
		}
		ImGui::EndGroup();
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_TEXTURE"))
			{
				ApplyTexture(entity.GetId(), static_cast<const char*>(payload->Data));
				history.Commit(*scene, entity.GetId());
			}
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_MATERIAL"))
			{
				if (scene->ApplyMaterial(entity.GetId(), static_cast<const char*>(payload->Data)))
				{
					std::snprintf(m_ShaderText, sizeof(m_ShaderText), "%s", material->Shader.c_str());
					std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", material->TexturePath.c_str());
					history.Commit(*scene, entity.GetId());
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	if (Lite::SpinComponent* spin = entity.Get<Lite::SpinComponent>())
	{
		if (ComponentHeader("Spin", "Spin"))
		{
			entity.Remove<Lite::SpinComponent>();
			removed("Spin");
		}
		else if (ImGui::DragFloat("Rate", &spin->Rate, 0.01f))
			refreshPhysics();
		finish();
	}

	if (Lite::Rigidbody2DComponent* body = entity.Get<Lite::Rigidbody2DComponent>())
	{
		const char* bodyLabel = EntryLabel(entity, "Rigidbody 2D", "Dynamic Rigidbody");
		if (ComponentHeader(bodyLabel, "Rigidbody"))
		{
			entity.Remove<Lite::Rigidbody2DComponent>();
			removed(bodyLabel);
		}
		else
		{
		const char* types[] = { "Static", "Kinematic", "Dynamic" };
		int current = static_cast<int>(body->Type);
		if (ImGui::Combo("Body", &current, types, 3))
		{
			body->Type = static_cast<Lite::BodyType>(current);
			refreshPhysics();
		}
		finish();
		if (ImGui::DragFloat("Mass", &body->Mass, 0.01f, 0.0f, 1000.0f))
			refreshPhysics();
		finish();
		if (ImGui::DragFloat("Gravity", &body->GravityScale, 0.01f))
			refreshPhysics();
		finish();
		if (ImGui::DragFloat2("Velocity", &body->LinearVelocity.x, 0.01f))
			refreshPhysics();
		finish();
		if (ImGui::DragFloat("Angular", &body->AngularVelocity, 0.01f))
			refreshPhysics();
		finish();
		if (ImGui::Checkbox("Freeze rotation", &body->FreezeRotation))
			refreshPhysics();
		finish();
		}
	}

	if (Lite::BoxCollider2DComponent* box = entity.Get<Lite::BoxCollider2DComponent>())
	{
		if (ComponentHeader("Box Collider 2D", "BoxCollider"))
		{
			entity.Remove<Lite::BoxCollider2DComponent>();
			removed("Box Collider 2D");
		}
		else
		{
			if (ImGui::DragFloat2("Box size", &box->Size.x, 0.01f, 0.0f, 100.0f))
				refreshPhysics();
			finish();
			if (ImGui::DragFloat2("Box offset", &box->Offset.x, 0.01f))
				refreshPhysics();
			finish();
			if (ImGui::Checkbox("Box trigger", &box->IsTrigger))
				refreshPhysics();
			finish();
		}
	}

	if (Lite::CircleCollider2DComponent* circle = entity.Get<Lite::CircleCollider2DComponent>())
	{
		if (ComponentHeader("Circle Collider 2D", "CircleCollider"))
		{
			entity.Remove<Lite::CircleCollider2DComponent>();
			removed("Circle Collider 2D");
		}
		else
		{
			if (ImGui::DragFloat("Radius", &circle->Radius, 0.01f, 0.0f, 100.0f))
				refreshPhysics();
			finish();
			if (ImGui::DragFloat2("Circle offset", &circle->Offset.x, 0.01f))
				refreshPhysics();
			finish();
			if (ImGui::Checkbox("Circle trigger", &circle->IsTrigger))
				refreshPhysics();
			finish();
		}
	}

	if (Lite::SortingComponent* sorting = entity.Get<Lite::SortingComponent>())
	{
		if (ComponentHeader("Sorting", "Sorting"))
		{
			entity.Remove<Lite::SortingComponent>();
			removed("Sorting");
		}
		else
			ImGui::DragInt("Order", &sorting->Order);
		finish();
	}

	if (Lite::ScriptComponent* script = entity.Get<Lite::ScriptComponent>())
	{
		if (ComponentHeader("Script", "Script"))
		{
			entity.Remove<Lite::ScriptComponent>();
			removed("Script");
		}
		else
		{
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##ScriptClass", m_ScriptText, sizeof(m_ScriptText)))
				script->Class = m_ScriptText;
			finish();
			ImGui::TextDisabled("Class name in the project script module");
		}
	}

	ImGui::Separator();
	static int addIndex = 0;
	size_t count = 0;
	const Lite::ComponentEntry* catalog = Lite::ComponentCatalog(count);
	ImGui::SetNextItemWidth(-90.0f);
	ImGui::Combo("##AddComponent", &addIndex, [](void* data, int index) -> const char*
	{
		const auto* entries = static_cast<const Lite::ComponentEntry*>(data);
		return entries[index].Name;
	}, const_cast<Lite::ComponentEntry*>(catalog), static_cast<int>(count));
	ImGui::SameLine();
	if (ImGui::Button("Add") && addIndex >= 0 && static_cast<size_t>(addIndex) < count)
	{
		catalog[addIndex].Add(entity);
		refreshPhysics();
		history.Commit(*scene, entity.GetId());
	}

	ImGui::End();
}
