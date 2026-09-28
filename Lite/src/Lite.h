#pragma once

// Public engine header. Include from exactly one client translation unit
// so the entry point is defined once.
#include "Lite/Core/Logger.h"
#include "Lite/Core/Assert.h"
#include "Lite/Core/Layer.h"
#include "Lite/Core/LayerStack.h"
#include "Lite/Input/Input.h"
#include "Lite/Math/Math.h"
#include "Lite/Assets/Asset.h"
#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Assets/Shader.h"
#include "Lite/Assets/Texture.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Renderer2D.h"
#include "Lite/Core/Events/Event.h"
#include "Lite/Core/Events/KeyEvent.h"
#include "Lite/Core/Events/MouseEvent.h"
#include "Lite/Core/Events/WindowEvent.h"
#include "Lite/Core/Application.h"
#include "EntryPoint.h"
