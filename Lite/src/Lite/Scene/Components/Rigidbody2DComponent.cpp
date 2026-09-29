#include <Lite/Scene/Components/Rigidbody2DComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>
#include <string>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			Rigidbody2DComponent* body = entity.Get<Rigidbody2DComponent>();
			if (body == nullptr)
				return nullptr;
			switch (body->Type)
			{
				case BodyType::Static: return "Static Rigidbody";
				case BodyType::Kinematic: return "Kinematic Rigidbody";
				default: return "Dynamic Rigidbody";
			}
		}

		void Add(Entity entity)
		{
			entity.Add<Rigidbody2DComponent>();
		}

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			auto* body = static_cast<Rigidbody2DComponent*>(scene.GetComponent(ComponentId::Rigidbody2D, entity));
			if (body == nullptr)
				return false;

			if (key == "type")
			{
				std::string type;
				stream >> type;
				if (type == "static")
					body->Type = BodyType::Static;
				else if (type == "kinematic")
					body->Type = BodyType::Kinematic;
				else
					body->Type = BodyType::Dynamic;
			}
			else if (key == "mass")
				stream >> body->Mass;
			else if (key == "gravity")
				stream >> body->GravityScale;
			else if (key == "velocity")
				stream >> body->LinearVelocity.x >> body->LinearVelocity.y;
			else if (key == "angular")
				stream >> body->AngularVelocity;
			else if (key == "freeze")
			{
				int freeze = 0;
				stream >> freeze;
				body->FreezeRotation = freeze != 0;
			}
			else
				return false;

			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& body = *static_cast<const Rigidbody2DComponent*>(component);
			const char* type = "dynamic";
			if (body.Type == BodyType::Static)
				type = "static";
			else if (body.Type == BodyType::Kinematic)
				type = "kinematic";

			output << "component rigidbody2d\n";
			output << std::format("type {}\n", type);
			output << std::format("mass {:.4f}\n", body.Mass);
			output << std::format("gravity {:.4f}\n", body.GravityScale);
			output << std::format("velocity {:.4f} {:.4f}\n", body.LinearVelocity.x, body.LinearVelocity.y);
			output << std::format("angular {:.4f}\n", body.AngularVelocity);
			output << std::format("freeze {}\n", body.FreezeRotation ? 1 : 0);
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Rigidbody2D;
				ops.Section = "rigidbody2d";
				ops.CatalogName = "Rigidbody 2D";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Rigidbody2D, []() -> ComponentPool* { return new TypedPool<Rigidbody2DComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
