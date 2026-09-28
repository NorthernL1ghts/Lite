#include "Layer.h"

namespace Lite {

	Layer::Layer(std::string name)
		: m_Name(std::move(name))
	{
	}

	Layer::~Layer() = default;

}
