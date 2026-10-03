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
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 2.0f));
		if (ImGui::BeginTable("##color", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("swatch", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(label);
			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::ColorEdit4("##swatch", &color.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoLabel);
			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
		ImGui::PopID();
	}

	enum class SectionAction
	{
		Closed,
		Open,
		Remove
	};

	SectionAction ComponentSection(const char* label, const char* id, bool openByDefault)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
		if (openByDefault)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		ImGui::PushID(id);
		const float removeWidth = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		bool open = false;
		bool remove = false;
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 2.0f));
		if (ImGui::BeginTable("##row", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX))
		{
			ImGui::TableSetupColumn("title", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, removeWidth + 8.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			open = ImGui::CollapsingHeader(label, flags);
			ImGui::TableSetColumnIndex(1);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.16f, 0.16f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f, 0.22f, 0.22f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.58f, 0.24f, 0.24f, 1.0f));
			remove = ImGui::Button("Remove", ImVec2(removeWidth, ImGui::GetFrameHeight()));
			ImGui::PopStyleColor(3);
			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
		ImGui::PopID();
		if (remove)
			return SectionAction::Remove;
		return open ? SectionAction::Open : SectionAction::Closed;
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
		ImGui::SameLine(0.0f, 8.0f);
		ImGui::SetNextItemWidth(std::max(48.0f, ImGui::GetContentRegionAvail().x));
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
	ImGui::PushItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x * 0.62f));

	const std::string prefab = scene->GetPrefab(entity.GetId());
	if (!prefab.empty())
		ImGui::TextDisabled("Prefab: %s", prefab.c_str());
	if (Lite::Entity parent = scene->GetEntity(scene->GetParent(entity.GetId())))
		ImGui::TextDisabled("Parent: %s", parent.GetName().c_str());

	if (Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>())
	{
		switch (ComponentSection("Transform", "Transform", true))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::TransformComponent>();
			removed("Transform");
			break;
			case SectionAction::Open:
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
			break;
			}
			default:
				break;
		}
	}

	if (Lite::CameraComponent* camera = entity.Get<Lite::CameraComponent>())
	{
		const char* cameraLabel = EntryLabel(entity, "Orthographic Camera", "Orthographic Camera");
		switch (ComponentSection(cameraLabel, "Camera", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::CameraComponent>();
			removed(cameraLabel);
			break;
			case SectionAction::Open:
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
		bool primary = camera->Primary;
		if (ImGui::Checkbox("Primary", &primary))
		{
			if (primary)
				scene->SetPrimaryCamera(entity.GetId());
			else
				camera->Primary = false;
		}
		finish();
		if (ImGui::TreeNode("Clipping"))
		{
			ImGui::DragFloat("Near", &camera->Near, 0.01f);
			finish();
			ImGui::DragFloat("Far", &camera->Far, 0.01f);
			finish();
			ImGui::TreePop();
		}
			break;
			}
			default:
				break;
		}
	}

	if (Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>())
	{
		const char* meshLabel = EntryLabel(entity, "Quad", "Quad");
		switch (ComponentSection(meshLabel, "Mesh", true))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::MeshComponent>();
			removed(meshLabel);
			break;
			case SectionAction::Open:
			{
			const char* types[] = { "Quad", "Triangle", "Sprite" };
			int current = static_cast<int>(mesh->Type);
			if (ImGui::Combo("Type", &current, types, 3))
				mesh->Type = static_cast<Lite::MeshType>(current);
			finish();
			break;
			}
			default:
				break;
		}
	}

	if (Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>())
	{
		ImGui::BeginGroup();
		switch (ComponentSection("Material", "Material", true))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::MaterialComponent>();
			removed("Material");
			break;
			case SectionAction::Open:
			{
		if (!material->UseVertexColors)
		{
			ColorField("Color", material->Color);
			finish();
		}

		float opacity = material->Color.w;
		if (SurfaceSlider("Opacity", opacity, 0.0f, 1.0f))
			material->Color.w = opacity;
		finish();

		if (ImGui::TreeNode("Surface"))
		{
			if (SurfaceSlider("Roughness", material->Roughness, 0.0f, 1.0f))
				material->Roughness = std::clamp(material->Roughness, 0.0f, 1.0f);
			finish();
			if (SurfaceSlider("Metallic", material->Metallic, 0.0f, 1.0f))
				material->Metallic = std::clamp(material->Metallic, 0.0f, 1.0f);
			finish();
			if (SurfaceSlider("Emission", material->Emission, 0.0f, 4.0f))
				material->Emission = std::max(material->Emission, 0.0f);
			finish();
			ImGui::TextDisabled("0 roughness is smooth. 1 is matte.");
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("More"))
		{
			Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>();
			const bool triangle = mesh != nullptr && mesh->Type == Lite::MeshType::Triangle;
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
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputTextWithHint("##Texture", "Texture path", m_TextureText, sizeof(m_TextureText)))
				scene->AssignTexture(entity.GetId(), m_TextureText);
			if (ImGui::IsItemDeactivatedAfterEdit())
				std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", material->TexturePath.c_str());
			finish();
			if (ImGui::Button("Save Material"))
				SaveMaterial(*scene, entity);
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputTextWithHint("##Shader", "Shader", m_ShaderText, sizeof(m_ShaderText)))
				material->Shader = m_ShaderText;
			finish();
			ImGui::TreePop();
		}
			break;
			}
			default:
				break;
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
		switch (ComponentSection("Spin", "Spin", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::SpinComponent>();
			removed("Spin");
			break;
			case SectionAction::Open:
			if (ImGui::DragFloat("Rate", &spin->Rate, 0.01f))
				refreshPhysics();
			finish();
			break;
			default:
				break;
		}
	}

	if (Lite::Rigidbody2DComponent* body = entity.Get<Lite::Rigidbody2DComponent>())
	{
		const char* bodyLabel = EntryLabel(entity, "Rigidbody 2D", "Dynamic Rigidbody");
		switch (ComponentSection(bodyLabel, "Rigidbody", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::Rigidbody2DComponent>();
			removed(bodyLabel);
			break;
			case SectionAction::Open:
			{
		const char* types[] = { "Static", "Kinematic", "Dynamic" };
		int current = static_cast<int>(body->Type);
		if (ImGui::Combo("Body", &current, types, 3))
		{
			body->Type = static_cast<Lite::BodyType>(current);
			refreshPhysics();
		}
		finish();
		if (ImGui::Checkbox("Freeze rotation", &body->FreezeRotation))
			refreshPhysics();
		finish();
		if (ImGui::TreeNode("Motion"))
		{
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
			ImGui::TreePop();
		}
			break;
			}
			default:
				break;
		}
	}

	if (Lite::BoxCollider2DComponent* box = entity.Get<Lite::BoxCollider2DComponent>())
	{
		switch (ComponentSection("Box Collider 2D", "BoxCollider", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::BoxCollider2DComponent>();
			removed("Box Collider 2D");
			break;
			case SectionAction::Open:
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
			break;
			}
			default:
				break;
		}
	}

	if (Lite::CircleCollider2DComponent* circle = entity.Get<Lite::CircleCollider2DComponent>())
	{
		switch (ComponentSection("Circle Collider 2D", "CircleCollider", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::CircleCollider2DComponent>();
			removed("Circle Collider 2D");
			break;
			case SectionAction::Open:
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
			break;
			}
			default:
				break;
		}
	}

	if (Lite::SortingComponent* sorting = entity.Get<Lite::SortingComponent>())
	{
		switch (ComponentSection("Sorting", "Sorting", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::SortingComponent>();
			removed("Sorting");
			break;
			case SectionAction::Open:
			ImGui::DragInt("Order", &sorting->Order);
			finish();
			break;
			default:
				break;
		}
	}

	if (Lite::ScriptComponent* script = entity.Get<Lite::ScriptComponent>())
	{
		switch (ComponentSection("Script", "Script", false))
		{
			case SectionAction::Remove:
			entity.Remove<Lite::ScriptComponent>();
			removed("Script");
			break;
			case SectionAction::Open:
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputTextWithHint("##ScriptClass", "Class name", m_ScriptText, sizeof(m_ScriptText)))
				script->Class = m_ScriptText;
			finish();
			break;
			default:
				break;
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

	ImGui::PopItemWidth();
	ImGui::End();
}
