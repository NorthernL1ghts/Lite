#pragma once

#include "Lite/Core/Application.h"

int main(int, char**)
{
    auto app = Lite::CreateApplication();
    app->Run();
    return 0;
}
