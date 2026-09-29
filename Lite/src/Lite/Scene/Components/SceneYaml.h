#pragma once

#include <Lite/Math/Math.h>

#include <yaml-cpp/yaml.h>

namespace Lite {

	inline Vec2 ReadVec2(const YAML::Node& node, Vec2 fallback = {})
	{
		if (!node || !node.IsSequence() || node.size() < 2)
			return fallback;

		return { node[0].as<float>(), node[1].as<float>() };
	}

	inline Vec3 ReadVec3(const YAML::Node& node, Vec3 fallback = {})
	{
		if (!node || !node.IsSequence() || node.size() < 3)
			return fallback;

		return { node[0].as<float>(), node[1].as<float>(), node[2].as<float>() };
	}

	inline Vec4 ReadVec4(const YAML::Node& node, Vec4 fallback = {})
	{
		if (!node || !node.IsSequence() || node.size() < 4)
			return fallback;

		return { node[0].as<float>(), node[1].as<float>(), node[2].as<float>(), node[3].as<float>() };
	}

	inline YAML::Node WriteVec2(const Vec2& value)
	{
		YAML::Node node(YAML::NodeType::Sequence);
		node.SetStyle(YAML::EmitterStyle::Flow);
		node.push_back(value.x);
		node.push_back(value.y);
		return node;
	}

	inline YAML::Node WriteVec3(const Vec3& value)
	{
		YAML::Node node(YAML::NodeType::Sequence);
		node.SetStyle(YAML::EmitterStyle::Flow);
		node.push_back(value.x);
		node.push_back(value.y);
		node.push_back(value.z);
		return node;
	}

	inline YAML::Node WriteVec4(const Vec4& value)
	{
		YAML::Node node(YAML::NodeType::Sequence);
		node.SetStyle(YAML::EmitterStyle::Flow);
		node.push_back(value.x);
		node.push_back(value.y);
		node.push_back(value.z);
		node.push_back(value.w);
		return node;
	}

}
