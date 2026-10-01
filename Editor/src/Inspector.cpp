#include <Inspector.h>

#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>

#include <imgui.h>

#include <cstdio>
#include <cstring>
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

	std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", path.c_str());
	if (!path.empty() && !assigned.Loaded)
		Lite::Console::Log(std::format("Failed to load texture {}", path));
	else
		Lite::Console::Log(std::format("Texture set: {}", path));
}

void Inspector::Draw(std::uint32_t selected)
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
		Lite::Console::Log(std::format("Removed {}", label));
	};

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##ObjectName", m_ObjectName, sizeof(m_ObjectName)))
		entity.SetName(m_ObjectName);

	const std::string prefab = scene->GetPrefab(entity.GetId());
	if (!prefab.empty())
		ImGui::TextDisabled("Prefab: %s", prefab.c_str());

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
			float rotation = transform->Local.GetRotationZ();
			if (ImGui::DragFloat("Rotation", &rotation, 0.01f))
			{
				transform->Local.SetRotationZ(rotation);
				refreshPhysics();
			}
			if (ImGui::DragFloat3("Scale", &transform->Local.Scale.x, 0.01f))
				refreshPhysics();
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
		if (camera->Projection == Lite::CameraProjection::Orthographic)
			ImGui::DragFloat("Size", &camera->Size, 0.01f, 0.25f, 12.0f);
		else
		{
			float degrees = camera->FieldOfView * (180.0f / 3.14159265f);
			if (ImGui::DragFloat("Field of view", &degrees, 0.1f, 1.0f, 179.0f))
				camera->FieldOfView = degrees * (3.14159265f / 180.0f);
		}
		ImGui::DragFloat("Near", &camera->Near, 0.01f);
		ImGui::DragFloat("Far", &camera->Far, 0.01f);
		bool primary = camera->Primary;
		if (ImGui::Checkbox("Primary", &primary))
		{
			if (primary)
				scene->SetPrimaryCamera(entity.GetId());
			else
				camera->Primary = false;
		}
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
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Shader");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##Shader", m_ShaderText, sizeof(m_ShaderText)))
			material->Shader = m_ShaderText;

		Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>();
		ImGui::Checkbox("Vertex colors", &material->UseVertexColors);

		if (material->UseVertexColors)
		{
			int colors = mesh != nullptr && mesh->Type == Lite::MeshType::Triangle ? 3 : 4;
			for (int index = 0; index < colors; ++index)
				ColorField(std::format("Color {}", index + 1).c_str(), material->Colors[index]);
		}
		else
		{
			ColorField("Color", material->Color);
		}

		float opacity = material->Color.w;
		if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f))
			material->Color.w = opacity;

		if (mesh != nullptr && mesh->Type == Lite::MeshType::Sprite)
		{
			ImGui::DragFloat2("Tiling", &material->Tiling.x, 0.01f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Texture");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##Texture", m_TextureText, sizeof(m_TextureText)))
				scene->AssignTexture(entity.GetId(), m_TextureText);
		}
		}
		ImGui::EndGroup();
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_TEXTURE"))
				ApplyTexture(entity.GetId(), static_cast<const char*>(payload->Data));
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
		if (ImGui::DragFloat("Mass", &body->Mass, 0.01f, 0.0f, 1000.0f))
			refreshPhysics();
		if (ImGui::DragFloat("Gravity", &body->GravityScale, 0.01f))
			refreshPhysics();
		if (ImGui::DragFloat2("Velocity", &body->LinearVelocity.x, 0.01f))
			refreshPhysics();
		if (ImGui::DragFloat("Angular", &body->AngularVelocity, 0.01f))
			refreshPhysics();
		if (ImGui::Checkbox("Freeze rotation", &body->FreezeRotation))
			refreshPhysics();
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
			if (ImGui::DragFloat2("Box offset", &box->Offset.x, 0.01f))
				refreshPhysics();
			if (ImGui::Checkbox("Box trigger", &box->IsTrigger))
				refreshPhysics();
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
			if (ImGui::DragFloat2("Circle offset", &circle->Offset.x, 0.01f))
				refreshPhysics();
			if (ImGui::Checkbox("Circle trigger", &circle->IsTrigger))
				refreshPhysics();
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
	}

	ImGui::End();
}
