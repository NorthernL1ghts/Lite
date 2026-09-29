#include <Lite/Scene/Components/CameraComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>
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

		void Add(Entity entity)
		{
			entity.Add<CameraComponent>();
		}

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			auto* camera = static_cast<CameraComponent*>(scene.GetComponent(ComponentId::Camera, entity));
			if (camera == nullptr)
				return false;

			if (key == "size")
				stream >> camera->Size;
			else if (key == "near")
				stream >> camera->Near;
			else if (key == "far")
				stream >> camera->Far;
			else if (key == "projection")
			{
				std::string projection;
				stream >> projection;
				camera->Projection = projection == "perspective" ? CameraProjection::Perspective : CameraProjection::Orthographic;
			}
			else if (key == "fov")
				stream >> camera->FieldOfView;
			else if (key == "primary")
			{
				int primary = 0;
				stream >> primary;
				camera->Primary = primary != 0;
			}
			else
				return false;

			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& camera = *static_cast<const CameraComponent*>(component);
			output << "component camera\n";
			output << std::format("projection {}\n", camera.Projection == CameraProjection::Perspective ? "perspective" : "orthographic");
			output << std::format("size {:.4f}\n", camera.Size);
			output << std::format("fov {:.4f}\n", camera.FieldOfView);
			output << std::format("near {:.4f}\n", camera.Near);
			output << std::format("far {:.4f}\n", camera.Far);
			output << std::format("primary {}\n", camera.Primary ? 1 : 0);
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Camera;
				ops.Section = "camera";
				ops.CatalogName = "Orthographic Camera";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Camera, []() -> ComponentPool* { return new TypedPool<CameraComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
