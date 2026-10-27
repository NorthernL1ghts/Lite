#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/RevoluteJoint2DComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<RevoluteJoint2DComponent>() ? "Revolute Joint" : nullptr;
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* joint = static_cast<RevoluteJoint2DComponent*>(scene.AddComponent(ComponentId::RevoluteJoint2D, entity));
			if (joint == nullptr)
				return;

			if (node["other"])
				joint->Other = node["other"].as<std::string>();
			if (node["motor"])
				joint->Motor = node["motor"].as<bool>();
			if (node["speed"])
				joint->MotorSpeed = node["speed"].as<float>();
			if (node["torque"])
				joint->MaxTorque = node["torque"].as<float>();
			if (node["limit"])
				joint->Limit = node["limit"].as<bool>();
			if (node["lower"])
				joint->LowerAngle = node["lower"].as<float>();
			if (node["upper"])
				joint->UpperAngle = node["upper"].as<float>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& joint = *static_cast<const RevoluteJoint2DComponent*>(component);
			node["other"] = joint.Other;
			if (joint.Motor)
			{
				node["motor"] = true;
				node["speed"] = joint.MotorSpeed;
				node["torque"] = joint.MaxTorque;
			}
			if (joint.Limit)
			{
				node["limit"] = true;
				node["lower"] = joint.LowerAngle;
				node["upper"] = joint.UpperAngle;
			}
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<RevoluteJoint2DComponent>(ComponentId::RevoluteJoint2D, "revolute-joint2d", "Revolute Joint", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
