#include <Lite/Core/LayerStack.h>

#include <Lite/Core/Log/Logger.h>

#include <algorithm>

namespace {

	bool Detach(std::vector<Lite::Scope<Lite::Layer>>& layers, std::vector<Lite::Scope<Lite::Layer>>::iterator begin, std::vector<Lite::Scope<Lite::Layer>>::iterator end, Lite::Layer* layer, const char* kind)
	{
		auto it = std::find_if(begin, end, [layer](const Lite::Scope<Lite::Layer>& item)
		{
			return item.get() == layer;
		});

		if (it == end)
			return false;

		LITE_TRACE("Detached {} {}", kind, (*it)->GetName());
		(*it)->OnDetach();
		layers.erase(it);
		return true;
	}

}

namespace Lite {

	LayerStack::~LayerStack()
	{
		for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
			(*it)->OnDetach();
	}

	void LayerStack::PushLayer(Scope<Layer> layer)
	{
		if (!layer)
			return;

		LITE_TRACE("Attached layer {}", layer->GetName());
		layer->OnAttach();
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
		++m_LayerInsertIndex;
	}

	void LayerStack::PushOverlay(Scope<Layer> layer)
	{
		if (!layer)
			return;

		LITE_TRACE("Attached overlay {}", layer->GetName());
		layer->OnAttach();
		m_Layers.push_back(std::move(layer));
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		if (Detach(m_Layers, m_Layers.begin(), m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsertIndex), layer, "layer"))
			--m_LayerInsertIndex;
	}

	void LayerStack::Clear()
	{
		for (size_t index = m_Layers.size(); index > 0; --index)
		{
			const size_t slot = index - 1;
			const char* kind = slot < m_LayerInsertIndex ? "layer" : "overlay";
			LITE_TRACE("Detached {} {}", kind, m_Layers[slot]->GetName());
			m_Layers[slot]->OnDetach();
		}

		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}

	void LayerStack::PopOverlay(Layer* layer)
	{
		Detach(m_Layers, m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsertIndex), m_Layers.end(), layer, "overlay");
	}

}
