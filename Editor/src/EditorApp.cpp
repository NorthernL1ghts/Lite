#include <Lite.h>

#include "EditorLayer.h"

namespace {

	class Editor final : public Lite::Application
	{
	public:
		Editor()
			: Lite::Application(Lite::WindowProps("Editor"))
		{
			PushOverlay(Lite::CreateScope<EditorLayer>());
			LITE_CLIENT_INFO("Created");
		}

		~Editor() override
		{
			LITE_CLIENT_INFO("Destroyed");
		}
	};

}

Lite::Scope<Lite::Application> Lite::CreateApplication()
{
	return Lite::CreateScope<Editor>();
}
