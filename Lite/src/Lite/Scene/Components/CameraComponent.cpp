#include <Lite/Scene/Components/CameraComponent.h>
#include <Lite/Scene/Components/ComponentInstall.h>

#include <string>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			CameraComponent* camera = entity.Get<CameraComponent>();
			if (camera == nullptr)
				return nullptr;
			return camera->Projection == CameraProjection::Perspective ? "Perspective Camera" : "Orthographic Camera";
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* camera = static_cast<CameraComponent*>(scene.AddComponent(ComponentId::Camera, entity));
			if (camera == nullptr)
				return;

			if (node["projection"])
			{
				std::string projection = node["projection"].as<std::string>();
				camera->Projection = projection == "perspective" ? CameraProjection::Perspective : CameraProjection::Orthographic;
			}
			if (node["size"])
				camera->Size = node["size"].as<float>();
			if (node["fov"])
				camera->FieldOfView = node["fov"].as<float>();
			if (node["near"])
				camera->Near = node["near"].as<float>();
			if (node["far"])
				camera->Far = node["far"].as<float>();
			if (node["primary"])
				camera->Primary = node["primary"].as<bool>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& camera = *static_cast<const CameraComponent*>(component);
			node["projection"] = camera.Projection == CameraProjection::Perspective ? "perspective" : "orthographic";
			node["size"] = camera.Size;
			node["fov"] = camera.FieldOfView;
			node["near"] = camera.Near;
			node["far"] = camera.Far;
			node["primary"] = camera.Primary;
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<CameraComponent>(ComponentId::Camera, "camera", "Orthographic Camera", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
