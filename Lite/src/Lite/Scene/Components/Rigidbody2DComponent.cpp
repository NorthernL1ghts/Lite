#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/Rigidbody2DComponent.h>
#include <Lite/Scene/Components/SceneYaml.h>

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

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* body = static_cast<Rigidbody2DComponent*>(scene.AddComponent(ComponentId::Rigidbody2D, entity));
			if (body == nullptr)
				return;

			if (node["type"])
			{
				std::string type = node["type"].as<std::string>();
				if (type == "static")
					body->Type = BodyType::Static;
				else if (type == "kinematic")
					body->Type = BodyType::Kinematic;
				else
					body->Type = BodyType::Dynamic;
			}
			if (node["mass"])
				body->Mass = node["mass"].as<float>();
			if (node["gravity"])
				body->GravityScale = node["gravity"].as<float>();
			if (node["velocity"])
				body->LinearVelocity = ReadVec2(node["velocity"], body->LinearVelocity);
			if (node["angular"])
				body->AngularVelocity = node["angular"].as<float>();
			if (node["freeze"])
				body->FreezeRotation = node["freeze"].as<bool>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& body = *static_cast<const Rigidbody2DComponent*>(component);
			const char* type = "dynamic";
			if (body.Type == BodyType::Static)
				type = "static";
			else if (body.Type == BodyType::Kinematic)
				type = "kinematic";

			node["type"] = type;
			node["mass"] = body.Mass;
			node["gravity"] = body.GravityScale;
			node["velocity"] = WriteVec2(body.LinearVelocity);
			node["angular"] = body.AngularVelocity;
			node["freeze"] = body.FreezeRotation;
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<Rigidbody2DComponent>(ComponentId::Rigidbody2D, "rigidbody2d", "Rigidbody 2D", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
