#include <Lite.h>

#include <print>

namespace {

	class Sandbox final : public Lite::Application
	{
	public:
		Sandbox()
		{
			std::println("Sandbox created");
		}

		~Sandbox() override
		{
			std::println("Sandbox destroyed");
		}
	};

}

Lite::Application* Lite::CreateApplication()
{
    return new Sandbox();
}