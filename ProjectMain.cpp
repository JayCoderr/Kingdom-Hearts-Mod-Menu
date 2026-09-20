#pragma once

#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H

#include "imgui.h"
#include <windows.h>
#include <reshade.hpp>

#include <string>
#include <algorithm>
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
#include <filesystem>
#include <array>
#include <cstdint>
#include <iomanip>
#include <atomic>

#include "ProjectMenuStructure.h"


// ============================================================
// ReShade Events
// ============================================================

static void onReshadeOverlay(
    reshade::api::effect_runtime*)
{
    ProjectX::MenuUtils();
}


// ============================================================
// Debug Console
// ============================================================

static void AllocateDebugConsole()
{
    if (GetConsoleWindow() != nullptr)
        return;

    if (!AllocConsole())
        return;

    FILE* dummyFile = nullptr;

    freopen_s(
        &dummyFile,
        "CONOUT$",
        "w",
        stdout);

    freopen_s(
        &dummyFile,
        "CONOUT$",
        "w",
        stderr);

    freopen_s(
        &dummyFile,
        "CONIN$",
        "r",
        stdin);

    std::ios::sync_with_stdio(true);

    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();

    SetConsoleTitleA(
        "Kingdom Hearts - ProjectX");

    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << " ProjectX Debug Console"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl;
}


// ============================================================
// DllMain
// ============================================================

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);


        // ====================================================
        // Allocate debug console
        // ====================================================

        //AllocateDebugConsole(); only for debugging


        std::cout
            << "[KH] DLL attached."
            << std::endl;


        // ====================================================
        // Register ReShade addon
        // ====================================================

        if (!reshade::register_addon(hModule))
        {
            std::cout
                << "[KH] Failed to register ReShade addon."
                << std::endl;

            return FALSE;
        }


        std::cout
            << "[KH] ReShade addon registered."
            << std::endl;


        // ====================================================
        // Register overlay callback
        // ====================================================

        reshade::register_event<
            reshade::addon_event::reshade_overlay
        >(
            onReshadeOverlay
        );


        std::cout
            << "[KH] Overlay callback registered."
            << std::endl;


        // ====================================================
        // DO NOT automatically start asset discovery.
        //
        // The user must press F2.
        // ====================================================

        std::cout
            << "[KH] Asset dumper is idle."
            << std::endl;

        std::cout
            << "[KH] Press F2 to begin asset dumping."
            << std::endl;
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        // ====================================================
        // Unregister ReShade events
        // ====================================================

        reshade::unregister_event<
            reshade::addon_event::reshade_overlay
        >(
            onReshadeOverlay
        );


        // ====================================================
        // Unregister addon
        // ====================================================

        reshade::unregister_addon(hModule);


        // ====================================================
        // Close console
        // ====================================================

        if (GetConsoleWindow() != nullptr)
        {
            FreeConsole();
        }
    }

    return TRUE;
}