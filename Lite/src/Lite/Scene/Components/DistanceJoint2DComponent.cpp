#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/DistanceJoint2DComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<DistanceJoint2DComponent>() ? "Distance Joint" : nullptr;
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* joint = static_cast<DistanceJoint2DComponent*>(scene.AddComponent(ComponentId::DistanceJoint2D, entity));
			if (joint == nullptr)
				return;

			if (node["other"])
				joint->Other = node["other"].as<std::string>();
			if (node["length"])
				joint->Length = node["length"].as<float>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& joint = *static_cast<const DistanceJoint2DComponent*>(component);
			node["other"] = joint.Other;
			if (joint.Length > 0.0f)
				node["length"] = joint.Length;
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<DistanceJoint2DComponent>(ComponentId::DistanceJoint2D, "distance-joint2d", "Distance Joint", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
