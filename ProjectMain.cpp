#pragma once
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imgui.h"
#include <windows.h>
#include <reshade.hpp>
#include <string>
#include <algorithm>
#include <string>
#include <functional>
#include <deque>
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <map>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <psapi.h>
#include <mutex>
#include "ProjectMenuStructure.h"


// ----------------------------
// ReShade Events
// ----------------------------
static void onReshadeOverlay(reshade::api::effect_runtime*)
{
    ProjectX::MenuUtils();
}
// ----------------------------
// DllMain
// ----------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!reshade::register_addon(hModule))
            return FALSE;

        reshade::register_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        reshade::unregister_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
        reshade::unregister_addon(hModule);
    }
    return TRUE;
}
