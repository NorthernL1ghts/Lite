#pragma once

#include <Lite/Scene/Scene.h>

#include <cstdint>

namespace Lite {

	struct ScriptClass
	{
		const char* Name = nullptr;
		void (*Start)(Entity entity) = nullptr;
		void (*Update)(Entity entity, float seconds) = nullptr;
		void (*OnCollision)(Entity entity, uint32_t other, bool begin) = nullptr;
	};

	using ScriptAddFn = void (*)(const ScriptClass* script);

	inline constexpr const char* ScriptRegisterExport = "LiteRegisterScripts";

	class LITE_API ScriptRuntime
	{
	public:
		static void Load(Scene& scene);
		static void Unload();
		static void Update(Scene& scene, float seconds);
		static void OnCollision(Scene& scene, uint32_t first, uint32_t second, bool begin);
	};

}
