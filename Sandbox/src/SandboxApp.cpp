#include <Lite.h>

#include "SandboxLayer.h"

namespace {

	class Sandbox final : public Lite::Application
	{
	public:
		Sandbox()
			: Lite::Application(Lite::WindowProps("Sandbox"))
		{
			PushLayer(Lite::CreateScope<SandboxLayer>());
			LITE_CLIENT_INFO("Created");
		}

		~Sandbox() override
		{
			LITE_CLIENT_INFO("Destroyed");
		}
	};

}

Lite::Scope<Lite::Application> Lite::CreateApplication()
{
	return Lite::CreateScope<Sandbox>();
}
