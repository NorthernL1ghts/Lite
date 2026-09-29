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
		for (auto& layer : m_Layers)
			layer->OnDetach();
	}

	void LayerStack::PushLayer(Scope<Layer> layer)
	{
		LITE_TRACE("Attached layer {}", layer->GetName());
		layer->OnAttach();
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
		++m_LayerInsertIndex;
	}

	void LayerStack::PushOverlay(Scope<Layer> layer)
	{
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
		for (auto& layer : m_Layers)
		{
			LITE_TRACE("Detached layer {}", layer->GetName());
			layer->OnDetach();
		}

		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}

	void LayerStack::PopOverlay(Layer* layer)
	{
		Detach(m_Layers, m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsertIndex), m_Layers.end(), layer, "overlay");
	}

}
