#pragma once

#include "Lite/Core/Logger.h"
#include "Lite/Core/Application.h"

int main(int, char**)
{
    Lite::Logger::Init();

    auto app = Lite::CreateApplication();
    app->Run();
    return 0;
}
