#pragma once
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include "imgui.h"
#include <windows.h>
#include "MinHook.h"
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
#include "ProjectMain.cpp"
#pragma comment(lib, "MinHook.x64.lib") // or x86 if 32-bit

// ---------------------------
// Example pattern scanning function
// (you need to implement the actual scan)
// ---------------------------
bool SafeReadMemory(void* src, void* dst, size_t size)
{
    __try
    {
        std::memcpy(dst, src, size); // plain memcpy
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Memory invalid
        return false;
    }
}
bool SafeWriteMemory(void* dst, void* src, size_t size)
{
    __try
    {
        std::memcpy(dst, src, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}
bool SafeWriteMemory(void* dst, const void* src, size_t size)
{
    DWORD oldProtect;
    if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    memcpy(dst, src, size);

    VirtualProtect(dst, size, oldProtect, &oldProtect);
    return true;
}
// -----------------------------
// Globals (runtime-controlled strings)
// -----------------------------
std::string gMapModelString;
std::string gMapMoveSetString;

struct LevelPatch
{
    uintptr_t offset1;           // offset from module base
    uintptr_t offset2;
    std::string* replacement1;   // POINTERS (dynamic)
    std::string* replacement2;
};

// Map level -> patch info (offsets static, strings dynamic)
std::unordered_map<std::string, LevelPatch> gLevelPatches =
{
    {
        "di02",
        {
            0x9F1F00,
            0x9F1F20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "di01",
        {
            0xAD7D80,
            0xAD7DA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    }
};

std::mutex gPatchMutex;

uintptr_t GetModuleBase()
{
    MODULEINFO modInfo = {};
    if (GetModuleInformation(
        GetCurrentProcess(),
        GetModuleHandle(NULL),
        &modInfo,
        sizeof(modInfo)))
    {
        return (uintptr_t)modInfo.lpBaseOfDll;
    }
    return 0;
}

// -----------------------------
// Returns current level
// -----------------------------
std::string GetCurrentLevelArea()
{
    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe");
    if (!moduleBase)
        return "Unknown";

    char buffer[33] = {};
    SafeReadMemory((void*)(moduleBase + 0x232DE90), buffer, 32);

    std::string level(buffer);
    return level.empty() ? "Unknown" : level;
}

// -----------------------------
// Patch monitor thread
// -----------------------------
void MonitorLevelPatches()
{
    std::string lastLevel;

    while (true)
    {
        std::string currentLevel = GetCurrentLevelArea();

        std::lock_guard<std::mutex> lock(gPatchMutex);

        if (currentLevel != lastLevel)
        {
            lastLevel = currentLevel;
        }

        auto it = gLevelPatches.find(currentLevel);
        if (it != gLevelPatches.end())
        {
            uintptr_t base = GetModuleBase();
            if (!base)
                continue;

            LevelPatch& patch = it->second;

            if (patch.replacement1 && !patch.replacement1->empty())
            {
                SafeWriteMemory(
                    (void*)(base + patch.offset1),
                    patch.replacement1->c_str(),
                    patch.replacement1->size() + 1
                );
            }

            if (patch.replacement2 && !patch.replacement2->empty())
            {
                SafeWriteMemory(
                    (void*)(base + patch.offset2),
                    patch.replacement2->c_str(),
                    patch.replacement2->size() + 1
                );
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// -----------------------------
// Initialization
// -----------------------------
void StartPatchMonitor()
{
    static bool started = false;
    if (!started)
    {
        std::thread(MonitorLevelPatches).detach();
        started = true;
    }
}

// -----------------------------
// Runtime setter (THIS is now dynamic)
// -----------------------------
void SetLevelModel(const std::string& model, const std::string& moveset)
{
    std::lock_guard<std::mutex> lock(gPatchMutex);

    gMapModelString = model;
    gMapMoveSetString = moveset;

    StartPatchMonitor();
}
void ParseSignature(const std::string& sigStr, std::vector<uint8_t>& outPattern, std::string& outMask)
{
    outPattern.clear();
    outMask.clear();

    std::istringstream iss(sigStr);
    std::string byteStr;

    while (iss >> byteStr)
    {
        if (byteStr == "??")
        {
            outPattern.push_back(0x00); // dummy
            outMask += '?';
        }
        else
        {
            outPattern.push_back((uint8_t)std::stoul(byteStr, nullptr, 16));
            outMask += 'x';
        }
    }
}

// ---------------------------
// Scan a memory range for pattern
// ---------------------------
uintptr_t ScanPatternInRangeFast(uintptr_t start, uintptr_t end, const uint8_t* pattern, const char* mask)
{
    size_t patternLen = strlen(mask);
    if (patternLen == 0) return 0;

    uint8_t firstByte = pattern[0];

    for (uintptr_t addr = start; addr <= end - patternLen;)
    {
        // skip bytes until first non-wildcard matches
        if (mask[0] != '?' && *(uint8_t*)addr != firstByte)
        {
            addr++;
            continue;
        }

        bool found = true;
        for (size_t i = 0; i < patternLen; ++i)
        {
            if (mask[i] == '?') continue;
            if (*(uint8_t*)(addr + i) != pattern[i])
            {
                found = false;
                break;
            }
        }

        if (found) return addr;

        addr++;
    }

    return 0;
}

// ---------------------------
// Cached .data section scan
// ---------------------------
uintptr_t FindPatternInModuleFast(const uint8_t* pattern, const char* mask)
{
    static uintptr_t dataStart = 0;
    static uintptr_t dataEnd = 0;

    if (!dataStart || !dataEnd)
    {
        HMODULE hMod = GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe");
        if (!hMod) return 0;

        MODULEINFO mi = {};
        if (!GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
            return 0;

        uintptr_t base = (uintptr_t)mi.lpBaseOfDll;
        IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
        IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

        IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        {
            IMAGE_SECTION_HEADER& sec = sections[i];
            std::string name((char*)sec.Name, 8);
            if (name.find(".data") != std::string::npos)
            {
                dataStart = base + sec.VirtualAddress;
                dataEnd = dataStart + sec.Misc.VirtualSize;
                break;
            }
        }
        if (!dataStart || !dataEnd) return 0;
    }

    return ScanPatternInRangeFast(dataStart, dataEnd, pattern, mask);
}

// ---------------------------
// Write entity state at pattern + 0x40
// ---------------------------
bool SetEntityStateFromSignatureString(const std::string& sigStr, uint8_t value)
{
    std::vector<uint8_t> pattern;
    std::string mask;
    ParseSignature(sigStr, pattern, mask);

    uintptr_t addr = FindPatternInModuleFast(pattern.data(), mask.c_str());
    if (!addr)
    {
        iprintln("Pattern not found!");
        return false;
    }

    uintptr_t targetAddr = addr + 0x40;
    if (!SafeWriteMemory((void*)targetAddr, &value, sizeof(value)))
    {
        iprintln("Failed to write entity state at pattern +0x18");
        return false;
    }

    return true;
}

bool SetEntityPartyMemberStateFromSignatureString(const std::string& sigStr, uint8_t value, int PartyMember)
{
    std::vector<uint8_t> pattern;
    std::string mask;
    ParseSignature(sigStr, pattern, mask);

    uintptr_t addr = FindPatternInModuleFast(pattern.data(), mask.c_str());
    if (!addr)
    {
        iprintln("Pattern not found!");
        return false;
    }
    if (PartyMember == 1)
    {
        uintptr_t targetAddr = addr + 0x40 + 0x4B0;//usually donald
        if (!SafeWriteMemory((void*)targetAddr, &value, sizeof(value)))
        {
            iprintln("Failed to write entity state at pattern +0x18");
            return false;
        }
    }
    else if (PartyMember == 2)
    {
        uintptr_t targetAddr = addr + 0x40 + 0x4B0 * 2;//usually goofy
        if (!SafeWriteMemory((void*)targetAddr, &value, sizeof(value)))
        {
            iprintln("Failed to write entity state at pattern +0x18");
            return false;
        }
    }
    return true;
}
std::atomic<bool> gAirHitDistancEnabled = false;
std::atomic<bool> gSuperJumpEnabled = false;
std::thread gSuperJumpThread;
std::thread gAirHitDistancThread;
std::atomic<bool> gSpeedFreezeEnabled = false;
std::atomic<float> gFrozenSpeedValue = 8.0f;
std::thread gSpeedFreezeThread;
// -------------------- GLOBALS --------------------
bool gFlyModeEnabled = false;                     // Sora Fly Mode
std::thread gFlyThread;                           // Sora Fly Mode thread
uint8_t gCurrentState = 0;                        // Sora current entity state

std::unordered_map<int, bool> gPartyFlyEnabled;  // Party members Fly Mode
std::unordered_map<int, std::thread> gPartyFlyThreads;
std::unordered_map<int, uint8_t> currentStates;  // Party members current entity

// -------------------- RESOLVED ADDRESSES --------------------
const char* SIG_PLAYER_MOVEMENT =
"00 00 00 00 00 00 00 00 DB 0F 49 3E 03 03 00 00 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40"; // base of movement struct
constexpr uintptr_t OFFSET_JUMP_HEIGHT = 0x10;
constexpr uintptr_t OFFSET_MOVE_SPEED = 0x08;
uintptr_t gJumpHeightAddr = 0;
uintptr_t gMoveSpeedAddr = 0;
uintptr_t gAirHitDistance = 0;

using tSub_2C3CC0 = __int64(__fastcall*)(__int64 a1, DWORD* a2, __int64 a3);
tSub_2C3CC0 oSub_2C3CC0 = nullptr;

bool IsValidReadPtr(void* ptr, size_t size = sizeof(uintptr_t))
{
    if (!ptr)
        return false;

    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(ptr, &mbi, sizeof(mbi)))
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;

    return (reinterpret_cast<uintptr_t>(ptr) + size) <=
        (reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize);
}

bool ResolveMovementAddresses()
{
    if (gJumpHeightAddr && gMoveSpeedAddr && gAirHitDistance)
        return true;

    std::vector<uint8_t> pattern;
    std::string mask;

    ParseSignature(SIG_PLAYER_MOVEMENT, pattern, mask);

    if (pattern.empty() || mask.empty())
    {
        iprintln("Pattern parse failed");
        return false;
    }

    uintptr_t found = FindPatternInModuleFast(pattern.data(), mask.c_str());
    if (!found)
    {
        iprintln("Movement sig not found");
        return false;
    }

    char buf[64];
    sprintf_s(buf, "Movement sig found @ 0x%llX", (unsigned long long)found);
    iprintln(buf);

    uintptr_t base = found + 0x48;
    if (!IsValidReadPtr((void*)base))
    {
        iprintln("Movement base invalid");
        return false;
    }

    gJumpHeightAddr = base;
    gMoveSpeedAddr = base - 8;
    gAirHitDistance = base + 16;

    return true;
}
void ToggleSuperAirHit()
{
    if (!ResolveMovementAddresses())
    {
        iprintln("Super Air Hit: pattern not found");
        return;
    }

    if (!gAirHitDistancEnabled)
    {
        gAirHitDistancEnabled = true;

        gAirHitDistancThread = std::thread([]()
            {
                DWORD oldProtect;
                VirtualProtect((LPVOID)gAirHitDistance, sizeof(float),
                    PAGE_EXECUTE_READWRITE, &oldProtect);

                while (gAirHitDistancEnabled)
                {
                    *(float*)gAirHitDistance = 0.950000f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }

                *(float*)gAirHitDistance = 1.2000048f;
                VirtualProtect((LPVOID)gAirHitDistance, sizeof(float),
                    oldProtect, &oldProtect);
            });

        gAirHitDistancThread.detach();
        iprintln("Super Air Hit: ON");
    }
    else
    {
        gAirHitDistancEnabled = false;
        iprintln("Super Air Hit: OFF");
    }
}

void ToggleSuperJump()
{
    if (!ResolveMovementAddresses())
    {
        iprintln("Super Jump: pattern not found");
        return;
    }

    if (!gSuperJumpEnabled)
    {
        gSuperJumpEnabled = true;

        gSuperJumpThread = std::thread([]()
            {
                DWORD oldProtect;
                VirtualProtect((LPVOID)gJumpHeightAddr, sizeof(float),
                    PAGE_EXECUTE_READWRITE, &oldProtect);

                while (gSuperJumpEnabled)
                {
                    *(float*)gJumpHeightAddr = 550.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }

                *(float*)gJumpHeightAddr = 190.0f;
                VirtualProtect((LPVOID)gJumpHeightAddr, sizeof(float),
                    oldProtect, &oldProtect);
            });

        gSuperJumpThread.detach();
        iprintln("Super Jump: ON");
    }
    else
    {
        gSuperJumpEnabled = false;
        iprintln("Super Jump: OFF");
    }
}
void StartSpeedFreeze(uintptr_t addr)
{
    if (gSpeedFreezeEnabled)
        return;

    gSpeedFreezeEnabled = true;

    gSpeedFreezeThread = std::thread([addr]()
        {
            DWORD oldProtect;
            VirtualProtect((LPVOID)addr, sizeof(float),
                PAGE_EXECUTE_READWRITE, &oldProtect);

            while (gSpeedFreezeEnabled)
            {
                *(float*)addr = gFrozenSpeedValue.load();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

            // restore default
            *(float*)addr = 8.0f;
            VirtualProtect((LPVOID)addr, sizeof(float), oldProtect, &oldProtect);
        });

    gSpeedFreezeThread.detach();
}
void ToggleSlowSpeed()
{
    if (!ResolveMovementAddresses())
    {
        iprintln("Slow Speed: pattern not found");
        return;
    }

    static const float levels[] = { 8.0f, 1.0f, 3.5f, 7.0f };
    static int index = 0;

    index = (index + 1) % _countof(levels);
    gFrozenSpeedValue = levels[index];

    if (gFrozenSpeedValue == 8.0f)
    {
        gSpeedFreezeEnabled = false;
        iprintln("Slow Speed: OFF (NORMAL)");
        return;
    }

    StartSpeedFreeze(gMoveSpeedAddr);

    char buf[64];
    sprintf_s(buf, "Slow Speed Level: %d", index);
    iprintln(buf);
}

void ToggleSuperSpeed()
{
    if (!ResolveMovementAddresses())
    {
        iprintln("Super Speed: pattern not found");
        return;
    }

    static const float levels[] = { 8.0f, 10.0f, 16.0f, 25.0f };
    static int index = 0;

    index = (index + 1) % _countof(levels);
    gFrozenSpeedValue = levels[index];

    if (gFrozenSpeedValue == 8.0f)
    {
        gSpeedFreezeEnabled = false;
        iprintln("Super Speed: OFF (NORMAL)");
        return;
    }

    StartSpeedFreeze(gMoveSpeedAddr);

    char buf[64];
    sprintf_s(buf, "Super Speed: x%d", index + 1);
    iprintln(buf);
}
std::atomic<bool> gLowGravityEnabled = false;
std::thread gLowGravityThread;
uintptr_t gGravityAddr = (uintptr_t)GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe") + 0x3E9CA4;

void ToggleLowGravity()
{
    if (!gGravityAddr)
    {
        iprintln("Gravity address not resolved yet");
        return;
    }

    if (!gLowGravityEnabled)
    {
        // Enable low gravity
        gLowGravityEnabled = true;

        gLowGravityThread = std::thread([]
            {
                DWORD oldProtect;
                VirtualProtect((LPVOID)gGravityAddr, sizeof(float),
                    PAGE_EXECUTE_READWRITE, &oldProtect);

                while (gLowGravityEnabled)
                {
                    *(float*)gGravityAddr = 0.1232222f; // low gravity
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }

                // Restore default gravity when disabled
                *(float*)gGravityAddr = 0.4846939f;
                VirtualProtect((LPVOID)gGravityAddr, sizeof(float),
                    oldProtect, &oldProtect);
            });

        gLowGravityThread.detach();
        iprintln("Low Gravity: ON");
    }
    else
    {
        // Disable low gravity
        gLowGravityEnabled = false;
        iprintln("Low Gravity: OFF");
    }
}

// -------------------- GENERIC ENTITY STATE --------------------
void SetSoraEntityState(uint8_t state)
{
    uint8_t newState = (gCurrentState == state) ? 0 : state; // toggle if same
    gCurrentState = newState;

    std::thread([newState]() {
        const std::string sig = "?? ?? ?? ?? 00 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 80 3F ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 00 00 80 3F 00 00 00 00  A0 ED A7 81 10 ?? A2 83";

        bool applied = false;
        for (int i = 0; i < 5 && !applied; ++i)
        {
            applied = SetEntityStateFromSignatureString(sig, newState);
            if (!applied)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        char buf[128];
        sprintf_s(buf, "Sora EntityState set: %u %s", newState, applied ? "(applied)" : "(pattern not found)");
        iprintln(buf);
        }).detach();
}

void SetFlyMode()
{
    const uint8_t FLY_STATE = 8;
    const std::string sig = "?? ?? ?? ?? 00 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 80 3F ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 00 00 80 3F 00 00 00 00  A0 ED A7 81 10 ?? A2 83";

    if (!gFlyModeEnabled)
    {
        // Turn ON Fly Mode
        gFlyModeEnabled = true;
        gCurrentState = FLY_STATE;

        gFlyThread = std::thread([sig]() {
            while (gFlyModeEnabled)
            {
                SetEntityStateFromSignatureString(sig, FLY_STATE);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            });
        gFlyThread.detach();

        iprintln("Fly Mode: ON");
    }
    else
    {
        // Turn OFF Fly Mode
        gFlyModeEnabled = false;

        // Reset Sora's entity state to 0
        gCurrentState = 0;
        SetEntityStateFromSignatureString(sig, 0);

        if (gFlyThread.joinable())
            gFlyThread.join();

        iprintln("Fly Mode: OFF");
    }
}

// -------------------- GENERIC PARTY MEMBER ENTITY STATE --------------------
void SetPartyMemEntityState(uint8_t state, int PartyMember)
{
    static std::unordered_map<int, uint8_t> currentStates;

    uint8_t newState = (currentStates[PartyMember] == state) ? 0 : state; // toggle
    currentStates[PartyMember] = newState;

    std::thread([newState, PartyMember]() {
        const std::string sig = "?? ?? ?? ?? 00 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 80 3F ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 00 00 80 3F 00 00 00 00  A0 ED A7 81 10 ?? A2 83";

        bool applied = false;
        for (int i = 0; i < 5 && !applied; ++i)
        {
            applied = SetEntityPartyMemberStateFromSignatureString(sig, newState, PartyMember);
            if (!applied)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        char buf[128];
        sprintf_s(buf, "Party Member %d EntityState set: %u %s",
            PartyMember, newState, applied ? "(applied)" : "(pattern not found)");
        iprintln(buf);
        }).detach();
}

void SetPartyMemFlyMode(int PartyMember)
{
    const uint8_t FLY_STATE = 8;
    const std::string sig = "?? ?? ?? ?? 00 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 80 3F ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 00 00 80 3F 00 00 00 00  A0 ED A7 81 10 ?? A2 83";

    // If not enabled, turn ON
    if (!gPartyFlyEnabled[PartyMember])
    {
        gPartyFlyEnabled[PartyMember] = true;

        gPartyFlyThreads[PartyMember] = std::thread([PartyMember, sig]() {
            while (gPartyFlyEnabled[PartyMember])
            {
                SetEntityPartyMemberStateFromSignatureString(sig, FLY_STATE, PartyMember);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            });
        gPartyFlyThreads[PartyMember].detach();

        currentStates[PartyMember] = FLY_STATE;

        char buf[64];
        sprintf_s(buf, "Party Member %d Fly Mode: ON", PartyMember);
        iprintln(buf);
    }
    else
    {
        // Turn OFF Fly Mode
        gPartyFlyEnabled[PartyMember] = false;

        // Can't join detached thread — optional: leave detach
        // If you want joinable, you need to store threads as pointers
        currentStates[PartyMember] = 0;

        // Apply state 0 once
        SetEntityPartyMemberStateFromSignatureString(sig, 0, PartyMember);

        char buf[64];
        sprintf_s(buf, "Party Member %d Fly Mode: OFF", PartyMember);
        iprintln(buf);
    }
}

void PrintCurrentLevel()
{
    std::string currentLevel = GetCurrentLevelArea();
    iprintln(("Current Level: " + currentLevel).c_str(), true, 30);
}
