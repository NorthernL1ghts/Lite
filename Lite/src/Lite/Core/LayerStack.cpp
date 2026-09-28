#include <Lite/Core/LayerStack.h>

#include <Lite/Core/Log/Logger.h>

#include <algorithm>

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
		auto it = std::find_if(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, [layer](const auto& item)
		{
			return item.get() == layer;
		});

		if (it == m_Layers.begin() + m_LayerInsertIndex)
			return;

		LITE_TRACE("Detached layer {}", (*it)->GetName());
		(*it)->OnDetach();
		m_Layers.erase(it);
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
		auto it = std::find_if(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), [layer](const auto& item)
		{
			return item.get() == layer;
		});

		if (it == m_Layers.end())
			return;

		LITE_TRACE("Detached overlay {}", (*it)->GetName());
		(*it)->OnDetach();
		m_Layers.erase(it);
	}

}
