#include <Lite.h>

#include "SandboxLayer.h"

namespace {

	class Sandbox final : public Lite::Application
	{
	public:
		Sandbox()
			: Lite::Application(Lite::WindowProps("Sandbox"))
		{
			PushLayer(std::make_unique<SandboxLayer>());
			LITE_CLIENT_INFO("Created");
		}

		~Sandbox() override
		{
			LITE_CLIENT_INFO("Destroyed");
		}
	};

}

std::unique_ptr<Lite::Application> Lite::CreateApplication()
{
	return std::make_unique<Sandbox>();
}
