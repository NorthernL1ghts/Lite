#pragma once

#include "Base.h"

#include <memory>

namespace Lite {

	class LITE_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		bool m_Running = true;
	};

	std::unique_ptr<Application> CreateApplication();

}
