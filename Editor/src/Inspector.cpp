#include <Inspector.h>

#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Assets/Texture.h>
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
	}

	bool editing = scene->GetPlayback() != Lite::ScenePlayback::Playing;
	if (!editing)
		ImGui::BeginDisabled();

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##ObjectName", m_ObjectName, sizeof(m_ObjectName)))
		entity.SetName(m_ObjectName);

	if (Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>())
	{
		ImGui::SeparatorText("Transform");
		ImGui::DragFloat3("Position", &transform->Local.Position.x, 0.01f);
		float rotation = transform->Local.GetRotationZ();
		if (ImGui::DragFloat("Rotation", &rotation, 0.01f))
			transform->Local.SetRotationZ(rotation);
		ImGui::DragFloat3("Scale", &transform->Local.Scale.x, 0.01f);
	}

	if (Lite::CameraComponent* camera = entity.Get<Lite::CameraComponent>())
	{
		ImGui::SeparatorText(EntryLabel(entity, "Orthographic Camera", "Orthographic Camera"));
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
			camera->Primary = primary;
			if (primary)
				scene->SetPrimaryCamera(entity.GetId());
		}
	}

	if (Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>())
	{
		ImGui::SeparatorText(EntryLabel(entity, "Quad", "Quad"));
		const char* types[] = { "Quad", "Triangle", "Sprite" };
		int current = static_cast<int>(mesh->Type);
		if (ImGui::Combo("Type", &current, types, 3))
			mesh->Type = static_cast<Lite::MeshType>(current);
	}

	if (Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>())
	{
		ImGui::SeparatorText("Material");
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Shader");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##Shader", m_ShaderText, sizeof(m_ShaderText)))
			material->Shader = m_ShaderText;

		Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>();
		bool vertexColors = material->UseVertexColors || (mesh != nullptr && mesh->Type == Lite::MeshType::Triangle);
		if (ImGui::Checkbox("Vertex colors", &material->UseVertexColors))
			vertexColors = material->UseVertexColors || (mesh != nullptr && mesh->Type == Lite::MeshType::Triangle);

		if (vertexColors)
		{
			int colors = mesh != nullptr && mesh->Type == Lite::MeshType::Triangle ? 3 : 4;
			for (int index = 0; index < colors; ++index)
				ColorField(std::format("Color {}", index + 1).c_str(), material->Colors[index]);
		}
		else
		{
			ColorField("Color", material->Color);
		}

		if (mesh != nullptr && mesh->Type == Lite::MeshType::Sprite)
		{
			ImGui::DragFloat2("Tiling", &material->Tiling.x, 0.01f);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Texture");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##Texture", m_TextureText, sizeof(m_TextureText)))
			{
				material->TexturePath = m_TextureText;
				material->Texture = material->TexturePath.empty()
					? Lite::Ref<Lite::Texture>{}
					: Lite::AssetRegistry::Get().Load<Lite::Texture>(material->TexturePath);
			}
		}
	}

	if (Lite::SpinComponent* spin = entity.Get<Lite::SpinComponent>())
	{
		ImGui::SeparatorText("Spin");
		ImGui::DragFloat("Rate", &spin->Rate, 0.01f);
	}

	if (Lite::Rigidbody2DComponent* body = entity.Get<Lite::Rigidbody2DComponent>())
	{
		ImGui::SeparatorText(EntryLabel(entity, "Rigidbody 2D", "Dynamic Rigidbody"));
		const char* types[] = { "Static", "Kinematic", "Dynamic" };
		int current = static_cast<int>(body->Type);
		if (ImGui::Combo("Body", &current, types, 3))
			body->Type = static_cast<Lite::BodyType>(current);
		ImGui::DragFloat("Mass", &body->Mass, 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat("Gravity", &body->GravityScale, 0.01f);
		ImGui::DragFloat2("Velocity", &body->LinearVelocity.x, 0.01f);
		ImGui::DragFloat("Angular", &body->AngularVelocity, 0.01f);
		ImGui::Checkbox("Freeze rotation", &body->FreezeRotation);
	}

	if (Lite::BoxCollider2DComponent* box = entity.Get<Lite::BoxCollider2DComponent>())
	{
		ImGui::SeparatorText("Box Collider 2D");
		ImGui::DragFloat2("Box size", &box->Size.x, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat2("Box offset", &box->Offset.x, 0.01f);
		ImGui::Checkbox("Box trigger", &box->IsTrigger);
	}

	if (Lite::CircleCollider2DComponent* circle = entity.Get<Lite::CircleCollider2DComponent>())
	{
		ImGui::SeparatorText("Circle Collider 2D");
		ImGui::DragFloat("Radius", &circle->Radius, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat2("Circle offset", &circle->Offset.x, 0.01f);
		ImGui::Checkbox("Circle trigger", &circle->IsTrigger);
	}

	if (Lite::SortingComponent* sorting = entity.Get<Lite::SortingComponent>())
	{
		ImGui::SeparatorText("Sorting");
		ImGui::DragInt("Order", &sorting->Order);
	}

	if (editing)
	{
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
			catalog[addIndex].Add(entity);
	}

	if (!editing)
	{
		ImGui::EndDisabled();
		ImGui::TextDisabled("Pause to edit");
	}

	ImGui::End();
}
