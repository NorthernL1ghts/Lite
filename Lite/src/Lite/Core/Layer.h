#pragma once

#include "Events/Event.h"
#include "Time.h"

#include <string>

namespace Lite {

	class LITE_API Layer
	{
	public:
		explicit Layer(std::string name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep timestep) { (void)timestep; }
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) { (void)event; }

		const std::string& GetName() const { return m_Name; }

	protected:
		std::string m_Name;
	};

}
