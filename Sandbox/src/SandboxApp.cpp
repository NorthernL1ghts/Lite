#include <Lite.h>

namespace {

	class Sandbox final : public Lite::Application
	{
	public:
		Sandbox()
		{
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
