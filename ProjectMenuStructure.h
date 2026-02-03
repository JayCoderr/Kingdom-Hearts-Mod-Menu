#pragma region KingDomHeartsMenuIncludes
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
#include <cctype>
#pragma endregion
#pragma region globals
using namespace std;

// ----------------------------
// Globals
// ----------------------------
static float g_overlayOpacity = 0.8f;
static ImVec2 g_windowPos = ImVec2(100, 100);
static ImVec2 g_windowSize = ImVec2(600, 800);

static char g_debugMessage[1024] = "";
static bool g_showDebugMessage = false;

static int menuInitalWidth = 158;

// At the top of your CPP (outside any struct/class)
static ImVec2 g_infoBoxPosCurrent(-500, -500);
static ImVec2 g_menuBoxPosCurrent(-500, -500);

static ImVec2 g_infoBoxPosTarget;
static ImVec2 g_menuBoxPosTarget;

static float g_slideSpeed = 50.0f;

static float g_menuWidthTarget = 158.0f;
static float g_menuWidthCurrent = 158.0f;
static float g_menuWidthSpeed = 600.0f; // pixels per second

static float nextOffsetY = 120.0f;
static float menuX = 15.0f;

static bool g_menuVisible = false;      // menu hidden by default
static int g_selectedIndex = 0;
static int g_menuItemCount = 0;
static bool g_triggerOption = false;
static bool g_menuChanged = false;

static const char* g_currentMenu = "^1Limit^7less ^1v1";
static std::vector<const char*> g_menuStack;

static uintptr_t playerEntityStateOffset = 0xE0;
static uintptr_t partyMemberOffset = 0x4B0;
const std::string EntityPlayerPtr = "?? ?? ?? ?? 20 AE ?? 81 60 AE ?? 81 D0 ?? ?? 82 02 00 00 00 ?? 00 00 00 ?? 00 00 00 00 00 ?? ?? 00 00 00 00 00 00 C0 40";
static std::atomic<bool> gLowGravityEnabled = false;
static std::thread gLowGravityThread;
static uintptr_t gGravityAddr = (uintptr_t)GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe") + 0x3E9CA4;
/*
63 B3 CA 3E 20 AE 54 81 60 AE 54 81 D0 15 C1 82 02 00 00 00 0A 00 00 00 0D 00 00 00 00 00 20 41
?? ?? ?? ?? 20 AE ?? 81 60 AE ?? 81 D0 ?? ?? 82 02 00 00 00 ?? 00 00 00 ?? 00 00 00 00 00 ?? ??
?? ?? ?? ?? 20 AE ?? 81 60 AE ?? 81 D0 ?? ?? 82 02 00 00 00 ?? 00 00 00 ?? 00 00 00 00 00 ?? ?? 00 00 00 00 00 00 C0 40//newest pattern
*/
static std::atomic<bool> gAirHitDistancEnabled = false;
static std::atomic<bool> gSuperJumpEnabled = false;
static std::thread gSuperJumpThread;
static std::thread gAirHitDistancThread;
static std::atomic<bool> gSpeedFreezeEnabled = false;
static std::atomic<float> gFrozenSpeedValue = 8.0f;
static std::thread gSpeedFreezeThread;
// -------------------- GLOBALS --------------------
static bool gFlyModeEnabled = false;                     // Sora Fly Mode
static std::thread gFlyThread;                           // Sora Fly Mode thread
static std::thread gNoclipModeThread;
static std::thread gCollisionThread;
static std::atomic<bool> gCollisionModeEnabled;
static uint8_t gCurrentState = 0;                        // Sora current entity state
static std::atomic_bool gNoclipModeEnabled{ false };

static std::unordered_map<int, bool> gPartyFlyEnabled;  // Party members Fly Mode
static std::unordered_map<int, std::thread> gPartyFlyThreads;
static std::unordered_map<int, uint8_t> currentStates;  // Party members current entity

// -------------------- RESOLVED ADDRESSES --------------------
static const char* SIG_PLAYER_MOVEMENT =
"00 00 00 00 00 00 00 00 DB 0F 49 3E 03 03 00 00 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40"; // base of movement struct
static constexpr uintptr_t OFFSET_JUMP_HEIGHT = 0x10;
static constexpr uintptr_t OFFSET_MOVE_SPEED = 0x08;
static uintptr_t gJumpHeightAddr = 0;
static uintptr_t gMoveSpeedAddr = 0;
static uintptr_t gAirHitDistance = 0;
static int g_scrollOffset = 0;     // which item is at visual index 0
static int g_visibleCount = 15;

static constexpr int START_INDEX = 0;
static constexpr int MID_INDEX = 8;
static constexpr int END_INDEX = 14;

static float g_enterCooldown = 0.0f;

using tSub_2C3CC0 = __int64(__fastcall*)(__int64 a1, DWORD* a2, __int64 a3);
static tSub_2C3CC0 oSub_2C3CC0 = nullptr;

static void mainMenu();
static void MainMods();
static void SkillsMenu();
static void SoraMenu();
static void WorldMenu();
static void FunMods();
static void MunnyMenu();
static void EditMenu();
static void EntityMenu();
static void ModelsMenu();
static void LevelsMenu();
static void TeleportMenu();
//static void CloneMenu();
static void SelfMsg();
static void ItemsMenu();
static void WeaponsMenu();
static void AblitysMenu();
static void SoundsMenu();
static void ForgeMenu();
static void AliceWorld();
static void AtlanticaWorld();
static void beautybeastWorld();
static void DonaldMenu();
static void GoofyMenu();
static void HalloweenWorld();
static void HeartLessMenu();
static void PeterpanWorld();
static void NoclipModes();
#pragma endregion
#pragma region otherfunctions

//To Upper and To Lower
static std::string ToUpper(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
}

// Convert string to lowercase
static std::string ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}
// ----------------------------
// Debug Message Box
// ----------------------------
static void ShowDebugMessage(const char* msg)
{
    strncpy_s(g_debugMessage, msg, sizeof(g_debugMessage) - 1);
    g_showDebugMessage = true;
}

// ----------------------------
// Drag Logic
// ----------------------------
static void HandleWindowDrag()
{
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        g_windowPos.x += io.MouseDelta.x;
        g_windowPos.y += io.MouseDelta.y;
    }
}
using MenuFunc = void(*)();
static std::vector<MenuFunc> g_menuStackFunc;
static MenuFunc g_currentMenuFunc = mainMenu;

static void subMenu(const char* menuName, MenuFunc menuStructFunc)
{
    g_triggerOption = false;
    g_menuStack.push_back(g_currentMenu);
    g_menuStackFunc.push_back(g_currentMenuFunc);

    g_currentMenu = menuName;
    g_currentMenuFunc = menuStructFunc;

    g_selectedIndex = 0;   // reset selection
    g_menuChanged = true;  // trigger UI update
}

static void backMenu()
{
    if (!g_menuStack.empty())
    {
        g_triggerOption = false;
        g_currentMenu = g_menuStack.back();
        g_menuStack.pop_back();

        g_currentMenuFunc = g_menuStackFunc.back();
        g_menuStackFunc.pop_back();

        g_selectedIndex = 0;
        g_menuChanged = true;
    }
}
// ----------------------------
// HUD Utils
// ----------------------------
struct HUDRect {
    ImVec2 pos;
    ImVec2 size;
    ImU32 color;
    float rounding;
};

// Create a filled rectangle
static void createRectangle(HUDRect r)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(
        r.pos,
        ImVec2(r.pos.x + r.size.x, r.pos.y + r.size.y), // ? use dynamic size
        r.color
    );
}
static void menuStart(float x, float y)
{
    menuX = x + 15.0f;
    nextOffsetY = y + 120.0f;
    g_menuItemCount = 0; // we can recalc it while drawing
}
// Draw a menu option (absolute coordinates)
static ImU32 GetColorForCode(char code)
{
    switch (code)
    {
    case '0': return IM_COL32(0, 0, 0, 255); // white
    case '1': return IM_COL32(255, 0, 0, 255); // red
    case '2': return IM_COL32(0, 255, 0, 255); // green
    case '3': return IM_COL32(0, 128, 255, 255); // blue
    case '4': return IM_COL32(255, 255, 0, 255); // yellow
    case '5': return IM_COL32(255, 0, 255, 255); // magenta
    case '6': return IM_COL32(0, 255, 255, 255); // cyan
    case '7': return IM_COL32(255, 255, 255, 255); // white
    case '8': return IM_COL32(160, 160, 160, 255); // gray
    case '9': return IM_COL32(128, 0, 255, 255); // purple
    case '10': return IM_COL32(255, 128, 0, 255); // orange
    default:  return IM_COL32(255, 255, 255, 255);
    }
}
enum class MenuAnimState {
    Idle,
    Shrinking,
    Growing
};

static MenuAnimState g_menuAnimState = MenuAnimState::Idle;
// -------------------------
// Menu structures
// -------------------------
static struct MenuEntry
{
    std::string label;
    std::function<void()> callback;
};

static struct MenuBuilder
{
    // Helper: check key press
    static inline bool ButtonPressed(int vk) {
        return GetAsyncKeyState(vk) & 1;
    }

    // Call this each frame to handle menu toggles and navigation
    static void MenuControls()
    {
        if (g_enterCooldown > 0.0f)
            g_enterCooldown -= ImGui::GetIO().DeltaTime;

        if (ButtonPressed(VK_F1))
            g_menuVisible = !g_menuVisible;

        ImGui::GetIO().MouseDrawCursor = g_menuVisible;

        if (g_menuItemCount <= 0)
            return;

        constexpr int visibleCount = END_INDEX - START_INDEX + 1; // 14 visible options
        int maxScroll = g_menuItemCount - visibleCount; // max offset

        // ----- UP -----
        if (ButtonPressed(VK_UP) && g_menuVisible)
        {
            g_selectedIndex--;

            if (g_selectedIndex < 0)
            {
                // wrap to bottom
                g_selectedIndex = g_menuItemCount - 1;
                g_scrollOffset = (maxScroll > 0) ? maxScroll : 0; // scroll to bottom
            }
            else if (g_selectedIndex < g_scrollOffset)
            {
                // scroll window up if selection moves above
                g_scrollOffset = g_selectedIndex;
            }
        }

        // ----- DOWN -----
        if (ButtonPressed(VK_DOWN) && g_menuVisible)
        {
            g_selectedIndex++;

            if (g_selectedIndex >= g_menuItemCount)
            {
                // wrap to top
                g_selectedIndex = 0;
                g_scrollOffset = 0; // scroll to top
            }
            else if (g_selectedIndex >= g_scrollOffset + visibleCount)
            {
                // scroll window down if selection moves below visible
                g_scrollOffset = g_selectedIndex - visibleCount + 1;
            }
        }

        // Back / Return
        if (ButtonPressed(VK_BACK) && g_menuVisible)
            backMenu();

        if (ButtonPressed(VK_RETURN) && g_menuVisible)
        {
            g_menuAnimState = MenuAnimState::Shrinking;
            g_triggerOption = true;
        }
    }

    static void menuTitle(
        const char* title,
        const ImVec2& absPos,
        float width,
        bool drawLine = true,
        ImU32 lineColor = IM_COL32(255, 0, 0, 255))
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImVec2 cursor = absPos;
        ImU32 currentColor = IM_COL32(255, 255, 255, 255);

        float lineHeight = ImGui::GetTextLineHeight();

        for (const char* p = title; *p; ++p)
        {
            // Color code ^0 - ^9
            if (*p == '^' && *(p + 1) >= '0' && *(p + 1) <= '9')
            {
                currentColor = GetColorForCode(*(p + 1));
                ++p;
                continue;
            }

            // NEWLINE SUPPORT
            if (*p == '\n')
            {
                cursor.x = absPos.x;
                cursor.y += lineHeight;
                continue;
            }

            char ch[2] = { *p, '\0' };

            // Draw character
            draw->AddText(cursor, currentColor, ch);

            // Advance cursor
            ImVec2 sz = ImGui::CalcTextSize(ch);
            cursor.x += sz.x;
        }

        if (drawLine)
        {
            float lineY = cursor.y + lineHeight + 2.0f;
            draw->AddLine(
                ImVec2(absPos.x, lineY),
                ImVec2(absPos.x + width, lineY),
                lineColor,
                1.0f
            );
        }
        ImVec2 winPos = ImGui::GetWindowPos();
        menuStart(winPos.x, winPos.y - 15);
    }

    static void addMenuTitle(const char* titleText)
    {
        if (strcmp(titleText, g_currentMenu) != 0)
            return;

        ImVec2 winPos = ImGui::GetWindowPos();
        //Position Math Shit
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 basePos = ImVec2(winPos.x + 15, winPos.y + 85);

        menuTitle(titleText, basePos, 100);
    }
    static void UpdateMenuWidth(float deltaTime)
    {
        float targetWidth = 158.0f; // default expanded width

        switch (g_menuAnimState)
        {
        case MenuAnimState::Idle:
            g_menuWidthCurrent = targetWidth;
            break;

        case MenuAnimState::Shrinking:
            g_menuWidthCurrent -= g_menuWidthSpeed * deltaTime;
            if (g_menuWidthCurrent <= 0.0f) {
                g_menuWidthCurrent = 0.0f;
                g_menuAnimState = MenuAnimState::Growing; // start growing back next frame
            }
            break;

        case MenuAnimState::Growing:
            g_menuWidthCurrent += g_menuWidthSpeed * deltaTime;
            if (g_menuWidthCurrent >= targetWidth) {
                g_menuWidthCurrent = targetWidth;
                g_menuAnimState = MenuAnimState::Idle; // done animating
            }
            break;
        }
    }
    template<typename Func, typename... Args>
    static void addMenuOpt(const char* menuName, const char* label, Func func, Args&&... args)
    {
        if (strcmp(menuName, g_currentMenu) != 0)
            return;

        ImDrawList* draw = ImGui::GetWindowDrawList();
        float lineHeight = ImGui::GetTextLineHeight();
        int thisIndex = g_menuItemCount++;

        // ---- NEW: calculate visual index and skip if outside visible window ----
        int visualIndex = thisIndex - g_scrollOffset;
        if (visualIndex < START_INDEX || visualIndex > END_INDEX)
            return;
        // ----------------------------------------------------------------------

        // Highlight selected option
        if (thisIndex == g_selectedIndex)
        {
            draw->AddRectFilled(ImVec2(menuX - 2, nextOffsetY - 2),
                ImVec2(menuX + g_menuWidthCurrent, nextOffsetY + lineHeight + 2),
                IM_COL32(255, 0, 0, 255));

            // Trigger callback immediately if Enter pressed
            if (g_triggerOption && thisIndex == g_selectedIndex)
            {
                g_triggerOption = false; // consume FIRST
                auto f = [=]() { func(args...); };
                f();
            }
        }

        // Draw text (your existing code)
        ImVec2 cursor(menuX, nextOffsetY);
        ImU32 currentColor = IM_COL32(255, 255, 255, 255);
        for (const char* p = label; *p; ++p)
        {
            if (*p == '^' && *(p + 1) >= '0' && *(p + 1) <= '9') {
                currentColor = GetColorForCode(*(p + 1));
                ++p;
                continue;
            }
            if (*p == '\n') { cursor.x = menuX; cursor.y += lineHeight; continue; }

            char ch[2] = { *p, '\0' };
            ImVec2 sz = ImGui::CalcTextSize(ch);
            draw->AddText(cursor, currentColor, ch);
            cursor.x += sz.x;
        }

        // Mouse click still works
        if (ImGui::IsMouseHoveringRect(ImVec2(menuX, nextOffsetY),
            ImVec2(menuX + 128, nextOffsetY + lineHeight)) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            auto f = [=]() { func(args...); };
            f();
        }

        nextOffsetY += lineHeight + 5.0f;
    }
    static void DrawColoredTextAbs(
        const char* text,
        const ImVec2& absPos)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImVec2 cursor = absPos;
        ImU32 currentColor = IM_COL32(255, 255, 255, 255);
        float lineHeight = ImGui::GetTextLineHeight();

        for (const char* p = text; *p; ++p)
        {
            // ^0 - ^9 color codes
            if (*p == '^' && *(p + 1) >= '0' && *(p + 1) <= '9')
            {
                currentColor = GetColorForCode(*(p + 1));
                ++p;
                continue;
            }

            // New line
            if (*p == '\n')
            {
                cursor.x = absPos.x;
                cursor.y += lineHeight;
                continue;
            }

            char ch[2] = { *p, '\0' };
            draw->AddText(cursor, currentColor, ch);

            cursor.x += ImGui::CalcTextSize(ch).x;
        }
    }
    // ----------------------------
    // HUD Rectangles (menuHuds)
    // ----------------------------
    static float CalcColoredTextWidth(const char* text)
    {
        float maxWidth = 0.0f;
        std::string line;

        for (const char* p = text; *p; ++p)
        {
            // Skip ^0–^9 color codes
            if (*p == '^' && *(p + 1) >= '0' && *(p + 1) <= '9')
            {
                ++p;
                continue;
            }

            // Newline ? measure current line
            if (*p == '\n')
            {
                maxWidth = std::max(maxWidth, ImGui::CalcTextSize(line.c_str()).x);
                line.clear();
                continue;
            }

            line.push_back(*p);
        }

        // Measure last line
        if (!line.empty())
            maxWidth = std::max(maxWidth, ImGui::CalcTextSize(line.c_str()).x);

        return maxWidth;
    }

    static float CalcTextHeight(const char* text)
    {
        int lines = 1;
        for (const char* p = text; *p; ++p)
            if (*p == '\n')
                lines++;

        return lines * ImGui::GetTextLineHeight();
    }

    static ImVec2 Lerp(const ImVec2& start, const ImVec2& end, float speed, float deltaTime)
    {
        ImVec2 result;
        result.x = start.x + (end.x - start.x) * speed * deltaTime;
        result.y = start.y + (end.y - start.y) * speed * deltaTime;
        return result;
    }

    static void menuHuds(const ImVec2& winPos)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        float deltaTime = ImGui::GetIO().DeltaTime;

        // Reset positions if menu is not visible
        if (!g_menuVisible)
        {
            g_infoBoxPosCurrent = ImVec2(-500, -500);
            g_menuBoxPosCurrent = ImVec2(-500, -500);
            return; // don't draw anything
        }

        // Set target positions based on desired final positions
        g_infoBoxPosTarget = ImVec2(winPos.x + 200, winPos.y + 80);
        g_menuBoxPosTarget = ImVec2(winPos.x + 10, winPos.y + 80);

        // Smoothly slide current positions toward target
        g_infoBoxPosCurrent = Lerp(g_infoBoxPosCurrent, g_infoBoxPosTarget, g_slideSpeed, deltaTime);
        g_menuBoxPosCurrent = Lerp(g_menuBoxPosCurrent, g_menuBoxPosTarget, g_slideSpeed, deltaTime);

        // ======================
        // INFO BOX
        // ======================
        const char* infoText =
            "^7Version: [^21.0^7]\n"
            "^7[^1Up^7]&[^1Down^7] Keys to scroll\n"
            "^7[^1Backspace^7] to go back\n"
            "^7[^1Enter^7] to select\n"
            "^7[^1F1^7] to open/close menu\n"
            "Created By: ^1J^7a^1y^7c^1o^7d^1e^7r ";

        const char* infoTitle = "^1Information ^7box";

        float paddingX = 10.0f;
        float paddingY = 10.0f;
        float extraWidth = 20.0f;

        float textWidth = CalcColoredTextWidth(infoText);
        float titleWidth = CalcColoredTextWidth(infoTitle);
        float finalWidth = std::max(textWidth, titleWidth);

        float textHeight = CalcTextHeight(infoText);
        float titleHeight = ImGui::GetTextLineHeight();
        float finalHeight = textHeight + titleHeight + paddingY * 2 + 10;

        HUDRect infoBox =
        {
            g_infoBoxPosCurrent,
            ImVec2(finalWidth + extraWidth, finalHeight),
            IM_COL32(0, 0, 0, 220),
            0.0f
        };

        createRectangle(infoBox);
        draw->AddRect(infoBox.pos, ImVec2(infoBox.pos.x + infoBox.size.x, infoBox.pos.y + infoBox.size.y),
            IM_COL32(255, 0, 0, 200), 0.0f, 0, 1.0f);

        menuTitle(infoTitle, ImVec2(infoBox.pos.x + 5, infoBox.pos.y + 5), infoBox.size.x - 10);

        DrawColoredTextAbs(infoText, ImVec2(infoBox.pos.x + paddingX, infoBox.pos.y + paddingY + titleHeight + 5));

        // ======================
        // MENU BOX
        // ======================
        int baseHeight = 315;
        float extraPerOption = 0;//future update
        int threshold = 11;

        int extraHeight = 0;

        if (g_menuItemCount > threshold)
        {
            // Above threshold ? grow menu
            extraHeight = (g_menuItemCount - threshold) * extraPerOption;
        }
        else if (g_menuItemCount < threshold)
        {
            // Below threshold ? shrink menu
            extraHeight = (g_menuItemCount + threshold) * extraPerOption; // negative
        }

        int menuHeight = baseHeight + extraHeight;

        HUDRect menuBox =
        {
            g_menuBoxPosCurrent,
            ImVec2(180, (float)menuHeight),
            IM_COL32(0, 0, 0, 220),
            0.0f
        };

        createRectangle(menuBox);
        draw->AddRect(menuBox.pos, ImVec2(menuBox.pos.x + menuBox.size.x, menuBox.pos.y + menuBox.size.y),
            IM_COL32(255, 0, 0, 200), 0.0f, 0, 1.0f);

        menuTitle("", ImVec2(menuBox.pos.x + 5, menuBox.pos.y + 5), menuBox.size.x - 10);
    }

    static ImU32 RainbowColor(float t, float speed = 1.0f)
    {
        // t = time in seconds, speed controls how fast it cycles
        float r = (sinf(t * speed + 0.0f) * 0.5f + 0.5f) * 255.0f;
        float g = (sinf(t * speed + 2.0f) * 0.5f + 0.5f) * 255.0f;
        float b = (sinf(t * speed + 4.0f) * 0.5f + 0.5f) * 255.0f;
        return IM_COL32((int)r, (int)g, (int)b, 255);
    }

    static void addVertText(const char* text, double verticalLineHeight, float glowStrength = 3.5f, int glowAlpha = 10)
    {
        ImVec2 winPos = ImGui::GetWindowPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        float fontSize = ImGui::GetFontSize();

        ImVec2 projPos = ImVec2(winPos.x + 180, winPos.y + 100);
        float textHeight = ImGui::GetTextLineHeight() * verticalLineHeight;
        float time = ImGui::GetTime();

        // ===== Glow layers based on glowStrength =====
        int layers = static_cast<int>(8 * glowStrength); // number of glow layers
        for (int i = 0; i < layers; ++i)
        {
            // Circular offsets
            float angle = (float)i / layers * 6.2831853f; // 2 * PI
            float radius = 0.8f * glowStrength;           // scale by glowStrength
            ImVec2 offsetPos = ImVec2(
                projPos.x + cosf(angle) * radius,
                projPos.y + sinf(angle) * radius
            );

            ImU32 rainbow = IM_COL32(
                (int)((sinf(time * 2.0f + 0.0f) * 0.5f + 0.5f) * 255),
                (int)((sinf(time * 2.0f + 2.0f) * 0.5f + 0.5f) * 255),
                (int)((sinf(time * 2.0f + 4.0f) * 0.5f + 0.5f) * 255),
                glowAlpha // now controlled by argument
            );

            draw->AddText(font, fontSize, offsetPos, rainbow, text);
        }

        // Draw main white text last (on top)
        draw->AddText(font, fontSize, projPos, IM_COL32(255, 255, 255, 255), text);

        // Draw vertical line
        ImU32 lineColor = IM_COL32(255, 0, 0, 200);
        draw->AddLine(
            ImVec2(projPos.x - 5, projPos.y),
            ImVec2(projPos.x - 5, projPos.y + textHeight),
            lineColor, 1.0f
        );
    }
    static void BeginMenu() {
        g_menuItemCount = 0;
        nextOffsetY = 120.0f;
    }
};
// -------------------------
// Global menu builder instance
// -------------------------
static inline MenuBuilder MenuBuilderInstance;
// Struct to track message box state
// ----------------------------
// Message box state
// ----------------------------
static struct MessageBoxX
{
    std::string text;
    float timer = 0.0f;
    float duration = 0.0f;
    bool typeOut = false;         // Should the text type out
    float typingSpeed = 30.0f;    // Characters per second
    float typedProgress = 0.0f;   // How much of text has been typed
};

static MessageBoxX g_messageBox;

// ----------------------------
// Helper: start a new message
// ----------------------------
static void iprintln(const char* msg, bool typeOut = false, float typingSpeed = 30.0f)
{
    if (!msg || !*msg) return;

    g_messageBox.text = msg;
    g_messageBox.typeOut = typeOut;
    g_messageBox.typingSpeed = typingSpeed;
    g_messageBox.typedProgress = 0.0f;

    // Random duration 3-8 seconds
    static std::mt19937 rng((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> dist(3.0f, 8.0f);

    g_messageBox.duration = dist(rng);
    g_messageBox.timer = g_messageBox.duration;
}

// ----------------------------
// Draw message box each frame
// Supports typeOut & carrot color codes
// ----------------------------
static void DrawMessageBox()
{
    if (g_messageBox.text.empty() || g_messageBox.timer <= 0.0f)
        return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    // Update timer
    g_messageBox.timer -= io.DeltaTime;

    // ---- Typewriter logic ----
    if (g_messageBox.typeOut)
    {
        g_messageBox.typedProgress += g_messageBox.typingSpeed * io.DeltaTime;
        if (g_messageBox.typedProgress > g_messageBox.text.size())
            g_messageBox.typedProgress = (float)g_messageBox.text.size();
    }
    else
    {
        g_messageBox.typedProgress = (float)g_messageBox.text.size();
    }

    // Extract visible text
    size_t visibleChars = (size_t)g_messageBox.typedProgress;
    std::string visibleText = g_messageBox.text.substr(0, visibleChars);

    // ---- Measure text size ----
    ImVec2 textSize = ImGui::CalcTextSize(visibleText.c_str());
    float padding = 10.0f;
    ImVec2 boxPos(20.0f, 20.0f);
    ImVec2 boxSize = ImVec2(textSize.x + padding * 2, textSize.y + padding * 2);

    // ---- Draw box ----
    draw->AddRectFilled(boxPos, ImVec2(boxPos.x + boxSize.x, boxPos.y + boxSize.y),
        IM_COL32(0, 0, 0, 220));
    draw->AddRect(boxPos, ImVec2(boxPos.x + boxSize.x, boxPos.y + boxSize.y),
        IM_COL32(255, 0, 0, 255), 0.0f, 0, 2.0f);

    // ---- Draw text with carrot support ----
    ImVec2 cursor = ImVec2(boxPos.x + padding, boxPos.y + padding);
    ImU32 currentColor = IM_COL32(255, 255, 255, 255);
    for (size_t i = 0; i < visibleText.size(); ++i)
    {
        if (visibleText[i] == '^' && i + 1 < visibleText.size() &&
            visibleText[i + 1] >= '0' && visibleText[i + 1] <= '9')
        {
            currentColor = GetColorForCode(visibleText[i + 1]);
            ++i;
            continue;
        }

        if (visibleText[i] == '\n')
        {
            cursor.x = boxPos.x + padding;
            cursor.y += ImGui::GetTextLineHeight();
            continue;
        }

        char ch[2] = { visibleText[i], '\0' };
        ImVec2 sz = ImGui::CalcTextSize(ch);
        draw->AddText(cursor, currentColor, ch);
        cursor.x += sz.x;
    }

    // ---- Clear when done ----
    if (g_messageBox.timer <= 0.0f)
        g_messageBox.text.clear();
}
// -------------------------
// Macro magic for `self menuOption(...)`
// -------------------------
#define CAT(a,b) a##b
#define self CAT(MenuBuilderInstance, .)
// ---------------------------
// Example pattern scanning function
// (you need to implement the actual scan)
// ---------------------------
static bool SafeReadMemory(void* src, void* dst, size_t size)
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
static bool SafeWriteMemory(void* dst, void* src, size_t size)
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
static bool SafeWriteMemory(void* dst, const void* src, size_t size)
{
    DWORD oldProtect;
    if (!VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    memcpy(dst, src, size);

    VirtualProtect(dst, size, oldProtect, &oldProtect);
    return true;
}
#pragma endregion
#pragma region levelmodelsMap
// -----------------------------
// Globals (runtime-controlled strings)
// -----------------------------
static std::string gMapModelString;
static std::string gMapMoveSetString;

struct LevelPatch
{
    uintptr_t offset1;           // offset from module base
    uintptr_t offset2;
    std::string* replacement1;   // POINTERS (dynamic)
    std::string* replacement2;
};

// Map level -> patch info (offsets static, strings dynamic)
static std::unordered_map<std::string, LevelPatch> gLevelPatches =
{
    {
        "dh01",
        {
            0x8E5980,
            0x8E59A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "dh02",
        {
            0x8A7A80,
            0x8A7AA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "dh03",
        {
            0x9CBC00,
            0x9CBC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "dh05",
        {
            0x7FA400,
            0x7FA420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "dh06",
        {
            0x9AC880,
            0x9AC8A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
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
    },
    {
        "di03",
        {
            0xA57580,
            0xA575A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "di11",
        {
            0x82EF80,
            0x82EFA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw01",
        {
            0xB19E00,
            0xB19E20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw02",
        {
            0xAC3A00,
            0xAC3A20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw05",
        {
            0xA9A980,
            0xA9A9A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw06",
        {
            0xAC6B00,
            0xAC6B20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw07",
        {
            0xA29300,
            0xA29320,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw08",
        {
            0xA09700,
            0xA09720,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw09",
        {
            0x910C00,
            0x910C20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw10",
        {
            0x7F4080,
            0x7F40A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw11",
        {
            0x8F7900,
            0x8F7920,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw14",
        {
            0x8A6400,
            0x8A6420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw15",
        {
            0x898380,
            0x8983A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw16",
        {
            0x898780,
            0x8987A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw17",
        {
            0x8A5D80,
            0x8A5DA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw18",
        {
            0x9CDA80,
            0x9CDAA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw20",
        {
            0x947180,
            0x9471A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw21",
        {
            0x944E00,
            0x944E20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw23",
        {
            0x96D400,
            0x96D420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw23",
        {
            0x96D400,
            0x96D420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw25",
        {
            0xA9A480,
            0xA9A4A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tw26",
        {
            0xA24980,
            0xA249A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm12",
        {
            0x9E2780,
            0x9E27A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw01",
        {
            0x855000,
            0x855020,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw02",
        {
            0x9FB380,
            0x9FB3A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw04",
        {
            0x9FFF80,
            0x9FFFA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw05",
        {
            0x8E4200,
            0x8E4220,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw06",
        {
            0x88E580,
            0x88E5A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw07",
        {
            0x88E580,
            0x8A78A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw08",
        {
            0x8A7880,
            0x8A78A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw09",
        {
            0x848900,
            0x848920,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "aw10",
        {
            0x91E580,
            0x91E5A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "he01",
        {
            0x974780,
            0x9747A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "he02",
        {
            0x8CBE80,
            0x8CBEA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "he03",
        {
            0xAD3700,
            0xAD3720,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz01",
        {
            0xA23400,
            0xA23420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz03",
        {
            0xA90480,
            0xA904A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz05",
        {
            0x942700,
            0x942720,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz06",
        {
            0xA1B500,
            0xA1B520,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz07",
        {
            0x9E6B80,
            0x9E6BA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz08",
        {
            0x9A8400,
            0x9A8420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz09",
        {
            0x9E4100,
            0x9E4120,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz10",
        {
            0x95BB00,
            0x95BB20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz11",
        {
            0x969300,
            0x969320,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz12",
        {
            0x996100,
            0x996120,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz13",
        {
            0x9F7480,
            0x9F74A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz15",
        {
            0x951600,
            0x951620,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "tz16",
        {
            0xAFA100,
            0xAFA120,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al01",
        {
            0xB27AC0,
            0xB27AF0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al02",
        {
            0x9E3840,
            0x9E3860,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al03",
        {
            0xA858C0,
            0xA858E0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al04",
        {
            0x95F740,
            0x95F760,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al06",
        {
            0x956D40,
            0x956D60,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al07",
        {
            0x95B840,
            0x95B860,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al08",
        {
            0x806240,
            0x806260,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al09",
        {
            0x9B6740,
            0x9B6760,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al10",
        {
            0x9AFF40,
            0x9AFF60,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al11",
        {
            0x9761C0,
            0x9761E0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al12",
        {
            0x8A3B80,
            0x8A3BA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al13",
        {
            0x9380C0,
            0x9380E0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al14",
        {
            0x960DC0,
            0x960DE0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al15",
        {
            0x93B240,
            0x93B260,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al16",
        {
            0x98C040,
            0x98C060,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al17",
        {
            0x97EE40,
            0x97EE60,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "al19",
        {
            0x83C080,
            0x83C0A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm01",
        {
            0xA43C80,
            0xA43CA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm02",
        {
            0xA4A980,
            0xA4A9A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm03",
        {
            0xA10D80,
            0xA10DA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm04",
        {
            0xA10480,
            0xA104A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm05",
        {
            0x962400,
            0x962420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm06",
        {
            0x972600,
            0x972620,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm07",
        {
            0x92DF80,
            0x92DFA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm08",
        {
            0x93CC00,
            0x93CC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm10",
        {
            0x974300,
            0x974320,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm11",
        {
            0x99BF80,
            0x99BFA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm12",
        {
            0x94D580,
            0x94D5A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm13",
        {
            0x9BE200,
            0x9BE220,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm14",
        {
            0x8FA700,
            0x8FA720,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm15",
        {
            0x99D080,
            0x99D0A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "lm16",
        {
            0x8A1700,
            0x8A1720,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi01",
        {
            0x951A00,
            0x951A20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi02",
        {
            0x9B3B80,
            0x9B3BA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi05",
        {
            0x9D2900,
            0x9D2920,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi06",
        {
            0x983580,
            0x9835A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi07",
        {
            0x9D6180,
            0x9D61A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi08",
        {
            0x9C5600,
            0x9C5620,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi09",
        {
            0x98A500,
            0x98A520,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi10",
        {
            0x9DAC80,
            0x9DACA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pi11",
        {
            0x9CA080,
            0x9CA0A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm01",
        {
            0xA65A80,
            0xA65AA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm02",
        {
            0x91E780,
            0x91E7A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm03",
        {
            0xA05100,
            0xA05120,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm04",
        {
            0xA25680,
            0xA05120,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm05",
        {
            0x9AB840,
            0x9AB860,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm07",
        {
            0xB1C300,
            0xB1C320,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm10",
        {
            0x8A8080,
            0x8A80A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "nm11",
        {
            0x8CD400,
            0x8CD420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pp01",
        {
            0x996F80,
            0x996FA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pp02",
        {
            0x95D000,
            0x95D020,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pp04",
        {
            0x95D100,
            0x95D120,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pp05",
        {
            0x9C3A00,
            0x9C3A20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pp08",
        {
            0x978F00,
            0x978F20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc01",
        {
            0x9CFE00,
            0x9CFE20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc02",
        {
            0x953980,
            0x9539A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc03",
        {
            0x91EF00,
            0x91EF20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc04",
        {
            0x9E1A00,
            0x9E1A20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc05",
        {
            0x9F6380,
            0x9F63A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc06",
        {
            0xAE3380,
            0xAE33A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc07",
        {
            0x9BBC80,
            0x9BBCA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc08",
        {
            0x950F80,
            0x950FA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc09",
        {
            0xB5D500,
            0xB5D520,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc10",
        {
            0xB5DD00,
            0xB5DD20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc11",
        {
            0x9D3680,
            0x9D36A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc12",
        {
            0xA2DF80,
            0xA2DFA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc13",
        {
            0xA0A780,
            0xA0A7A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc15",
        {
            0xBB3B80,
            0xBB3BA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "pc16",
        {
            0x9CDE80,
            0x9CDEA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew01",
        {
            0xAB3980,
            0xAB39A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew02",
        {
            0xA5AC80,
            0xA5ACA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew03",
        {
            0xA57A00,
            0xA57A20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew04",
        {
            0xA4BC00,
            0xA4BC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew06",
        {
            0xA57A00,
            0xA57A20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew07",
        {
            0xA4BC00,
            0xA4BC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew09",
        {
            0xA4BC00,
            0xA4BC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew10",
        {
            0xA4BC00,
            0xA4BC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew12",
        {
            0xA4BC00,
            0xA4BC20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew13",
        {
            0xA22400,
            0xA22420,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew15",
        {
            0x9F3C80,
            0x9F3CA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew16",
        {
            0x9A0F00,
            0x9A0F20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew17",
        {
            0xAE9080,
            0xAE90A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew17",
        {
            0x8CA680,
            0x8CA6A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew25",
        {
            0x7BE300,
            0x7BE320,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew26",
        {
            0x838F00,
            0x838F20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew27",
        {
            0x970580,
            0x9705A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew28",
        {
            0x96BF00,
            0x96BF20,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew29",
        {
            0x98D700,
            0x98D720,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew30",
        {
            0x9F1600,
            0x9F1620,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew31",
        {
            0xB5F980,
            0xB5F9A0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew34",
        {
            0x98C000,
            0x98C020,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew36",
        {
            0x919A80,
            0x919AA0,
            &gMapModelString,
            &gMapMoveSetString
        }
    },
    {
        "ew37",
        {
            0x87F100,
            0x87F120,
            &gMapModelString,
            &gMapMoveSetString
        }
    }
};
static std::mutex gPatchMutex;
#pragma endregion
#pragma region gameworldRelatedFunctions
static uintptr_t GetModuleBase()
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
static std::string GetCurrentLevelArea()
{
    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe");
    if (!moduleBase)
        return "Unknown";

    char buffer[33] = {};
    SafeReadMemory((void*)(moduleBase + 0x232DE90), buffer, 32);

    std::string level(buffer);
    return level.empty() ? "Unknown" : level;
}

static void setWeaponIndex(int Index)
{
    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe");
    if (!moduleBase) return;

    uintptr_t address = moduleBase + 0x232DE90;

    SafeWriteMemory((void*)address, &Index, sizeof(Index));

    iprintln("Weapon Index: " + Index);
}

static void giveMunny(int Amount)
{
    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"KINGDOM HEARTS FINAL MIX.exe");
    if (!moduleBase) return;

    uintptr_t address = moduleBase + 0x2DFF77C;

    // Read current munny safely
    uint32_t current = 0;
    if (!SafeReadMemory((void*)address, &current, sizeof(current)))
    {
        iprintln("Failed to read current munny");
        return;
    }

    // Calculate new amount safely
    int64_t newAmount = static_cast<int64_t>(current) + static_cast<int64_t>(Amount);
    if (newAmount < 0) newAmount = 0; // don't allow negative munny
    if (newAmount > 99999999) newAmount = 99999999; // optional max cap

    uint32_t safeAmount = static_cast<uint32_t>(newAmount);

    // Write new amount safely
    SafeWriteMemory((void*)address, &safeAmount, sizeof(safeAmount));

    iprintln("^2Sora's Munny: " + safeAmount);
}

static void ParseSignature(const std::string& sigStr, std::vector<uint8_t>& outPattern, std::string& outMask)
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
static uintptr_t ScanPatternInRangeFast(uintptr_t start, uintptr_t end, const uint8_t* pattern, const char* mask)
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
static uintptr_t FindPatternInModuleFast(const uint8_t* pattern, const char* mask)
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
//collision flags: 32771 || value == 41849
// ---------------------------
// Write entity state at pattern - E0
// ---------------------------
inline void DebugPrintTarget(const std::string& level,
    uintptr_t addr,
    uintptr_t playerEntityStateOffset,
    int32_t infoOffset,
    uintptr_t targetAddrSigned,
    uintptr_t targetAddr,
    bool enableConsole = false)
{
    if (!enableConsole) return; // console is off by default

    static bool consoleCreated = false;
    if (!consoleCreated)
    {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        consoleCreated = true;
    }

    printf("[DEBUG] Level: %s\n", level.c_str());
    printf("[DEBUG] addr: 0x%p\n", (void*)addr);
    printf("[DEBUG] playerEntityStateOffset: 0x%p\n", (void*)playerEntityStateOffset);
    printf("[DEBUG] info.offset: 0x%X\n", infoOffset);
    printf("[DEBUG] targetAddrSigned: 0x%p\n", (void*)targetAddrSigned);
    printf("[DEBUG] targetAddr: 0x%p\n", (void*)targetAddr);
}
// static cache variables (persistent between calls)
static std::unordered_map<std::string, uintptr_t> sigCache;
static std::string cachedLevel;

static bool EntityPoolFunction(
    const std::string& sigStr,
    uint32_t value,
    const char* entityType
)
{
    using namespace std::literals;

    // -----------------------------
    // ENTITY FIELD TABLE (local)
    // -----------------------------
    struct FieldInfo
    {
        int32_t offset;
        size_t  size;
    };

    static const std::unordered_map<std::string_view, FieldInfo> fields =
    {
        { "collisionState"sv,{-0x70,sizeof(uint8_t)}},
        { "animationState"sv,{0x214,sizeof(uint8_t)}},
        { "entityState"sv,{0x00,sizeof(uint8_t)}},//sora
        { "entityStatePrtyMem1"sv,{0x4B0,sizeof(uint8_t)}},
        { "entityStatePrtyMem2"sv,{0x4B0*2,sizeof(uint8_t)}},
        { "entityX"sv,{-0x60,sizeof(float)}},
        { "entityY"sv,{-0x58,sizeof(float)}},
        { "entityZ"sv,{-0x5C,sizeof(float)}},
        { "entityRotLeft"sv,{0x50,sizeof(float)}},//1 > -1
        { "entityRotRight"sv,{0x48,sizeof(float)}},//1 > -1
        { "entityWidth"sv,{-0x30,sizeof(float)}},//0-1 by default game stops at 1 but you can go further
        { "entityHeight"sv,{-0x2C,sizeof(float)}},
        { "entityDepth"sv,{-0x28,sizeof(float)}},
        { "entityRed"sv,{0x30,sizeof(float)}},//0-1 by default game stops at 1 but you can go further
        { "entityGreen"sv,{0x34,sizeof(float)}},
        { "entityBlue"sv,{0x38,sizeof(float)}},
        { "entityAlpha"sv,{0x3C,sizeof(float)}},
        { "entityAnimationSpeed"sv,{0x214,sizeof(float)}},
    };

    // -----------------------------
    // LOOKUP FIELD
    // -----------------------------
    auto fieldIt = fields.find(entityType);
    if (fieldIt == fields.end())
    {
        iprintln("Unknown entity field");
        return false;
    }

    // -----------------------------
    // CHECK LEVEL & CACHE
    // -----------------------------
    std::string currentLevel = GetCurrentLevelArea();
    if (currentLevel != cachedLevel)
    {
        sigCache.clear(); // refresh all signatures if level changed
        cachedLevel = currentLevel;
    }

    // -----------------------------
    // SIGNATURE LOOKUP (cached per level)
    // -----------------------------
    uintptr_t addr = 0;
    if (auto it = sigCache.find(sigStr); it != sigCache.end())
    {
        addr = it->second; // use cached address
    }
    else
    {
        std::vector<uint8_t> pattern;
        std::string mask;
        ParseSignature(sigStr, pattern, mask);

        addr = FindPatternInModuleFast(pattern.data(), mask.c_str());
        if (!addr)
        {
            iprintln("Pattern not found!");
            return false;
        }

        sigCache[sigStr] = addr; // store for this level
    }

    // -----------------------------
    // CALCULATE TARGET
    // -----------------------------
    const FieldInfo& info = fieldIt->second;
    intptr_t targetAddrSigned = static_cast<intptr_t>(addr) - static_cast<intptr_t>(playerEntityStateOffset);
    targetAddrSigned += static_cast<intptr_t>(info.offset);
    uintptr_t targetAddr = static_cast<uintptr_t>(targetAddrSigned);

    // -----------------------------
    // DEBUG PRINT (off by default)
    // -----------------------------
    DebugPrintTarget(currentLevel, addr, playerEntityStateOffset, info.offset,
        targetAddrSigned, targetAddr, /*enableConsole=*/false);

    // -----------------------------
    // WRITE MEMORY
    // -----------------------------
    if (info.size == sizeof(uint8_t))
    {
        uint8_t v = static_cast<uint8_t>(value);
        return SafeWriteMemory((void*)targetAddr, &v, sizeof(v));
    }
    else if (info.size == sizeof(uint16_t))
    {
        uint16_t v = static_cast<uint16_t>(value);
        return SafeWriteMemory((void*)targetAddr, &v, sizeof(v));
    }
    else if (info.size == sizeof(float))
    {
        float v;
        std::memcpy(&v, &value, sizeof(float));
        return SafeWriteMemory((void*)targetAddr, &v, sizeof(v));
    }
    else if (info.size == sizeof(uint32_t))
    {
        uint32_t v = static_cast<uint32_t>(value);
        return SafeWriteMemory((void*)targetAddr, &v, sizeof(v));
    }
    else
    {
        return SafeWriteMemory((void*)targetAddr, &value, info.size);
    }
}
// Float helper (pack float bits)
static uint32_t FloatToU32(float f)
{
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

// Read current float from entity (using same FindPatternInModuleFast cache)
static bool ReadEntityFloat(
    const std::string& sigStr,
    const char* entityType,
    float& outValue
)
{
    struct FieldInfo { int32_t offset; };

    static const std::unordered_map<std::string_view, FieldInfo> fields =
    {
        { "entityX", { -0x60 } },
        { "entityY", { -0x58 } },
        { "entityZ", { -0x5C } },
    };

    auto it = fields.find(entityType);
    if (it == fields.end())
        return false;

    // -----------------------------
    // reuse same signature cache as SetEntityFunction
    // -----------------------------
    static std::unordered_map<std::string, uintptr_t> sigCache;

    uintptr_t addr = 0;
    if (auto sit = sigCache.find(sigStr); sit != sigCache.end())
        addr = sit->second;
    else
    {
        std::vector<uint8_t> pattern;
        std::string mask;
        ParseSignature(sigStr, pattern, mask);

        addr = FindPatternInModuleFast(pattern.data(), mask.c_str());
        if (!addr)
            return false;

        sigCache[sigStr] = addr;
    }

    uintptr_t targetAddr = addr - playerEntityStateOffset + it->second.offset;

    return SafeReadMemory((void*)targetAddr, &outValue, sizeof(float));
}

static bool IsValidReadPtr(void* ptr, size_t size = sizeof(uintptr_t))
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

static bool ResolveMovementAddresses()
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
static void ToggleSuperAirHit()
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

static void ToggleSuperJump()
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
static void StartSpeedFreeze(uintptr_t addr)
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
static void ToggleSlowSpeed()
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

static void ToggleSuperSpeed()
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

static void ToggleLowGravity()
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
static void SetSoraEntityState(uint8_t state)
{
    uint8_t newState = (gCurrentState == state) ? 0 : state; // toggle if same
    gCurrentState = newState;

    std::thread([newState]() {
        bool applied = false;
        for (int i = 0; i < 5 && !applied; ++i)
        {
            applied = EntityPoolFunction(EntityPlayerPtr, newState, "entityState");
            if (!applied)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        char buf[128];
        sprintf_s(buf, "Sora EntityState set: %u %s", newState, applied ? "(applied)" : "(pattern not found)");
        iprintln(buf);
        }).detach();
}
static void ForceUp()
{
    iprintln("does nothing unless\nyou jump");
    SetSoraEntityState(3);
}
static void Teabag()
{
    static bool teabagOn = false;
    constexpr uint64_t TEABAG_DELAY_MS = 150; // delay between state changes

    // Toggle teabag on/off each call
    teabagOn = !teabagOn;

    if (!teabagOn)
    {
        iprintln("Teabag: OFF");
        SetSoraEntityState(0); // reset to normal
        return;
    }

    iprintln("Teabag: ON");

    // Start a thread to alternate states while teabag is active
    std::thread([]()
        {
            while (teabagOn)
            {
                SetSoraEntityState(1); // crouch down
                std::this_thread::sleep_for(std::chrono::milliseconds(TEABAG_DELAY_MS));

                SetSoraEntityState(3); // stand up / emote
                std::this_thread::sleep_for(std::chrono::milliseconds(TEABAG_DELAY_MS));
            }

            // Reset state when thread exits
            SetSoraEntityState(0);
        }).detach();
}


static void SetFlyMode()
{
    const uint8_t FLY_STATE = 8;

    if (!gFlyModeEnabled)
    {
        // Turn ON Fly Mode
        gFlyModeEnabled = true;
        gCurrentState = FLY_STATE;

        gFlyThread = std::thread([]() {
            while (gFlyModeEnabled)
            {
                EntityPoolFunction(EntityPlayerPtr, FLY_STATE, "entityState");
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
        EntityPoolFunction(EntityPlayerPtr, 0, "entityState");

        if (gFlyThread.joinable())
            gFlyThread.join();

        iprintln("Fly Mode: OFF");
    }
}

static void SetCollision(bool enable)
{
    if (!enable)
    {
        gCollisionModeEnabled.store(true);

        EntityPoolFunction(EntityPlayerPtr, 41849, "collisionState");//58233

        gCollisionThread = std::thread([]()
            {
                while (gCollisionModeEnabled.load())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

        gCollisionThread.detach();
        //iprintln("Collision: ON"); remove // for debugging
    }
    else
    {
        gCollisionModeEnabled.store(false);

        EntityPoolFunction(EntityPlayerPtr, 32771, "collisionState");
        //iprintln("Collision: OFF");
    }
}

struct TeleportTask
{
    float x, y, z;
    float timer;
    int step; // 0=pre-state, 1=apply position, 2=reset state
    bool active;
};

static std::vector<TeleportTask> g_teleportTasks;

// Call this from the menu to schedule a teleport
static void ScheduleTeleport(float x, float y, float z)
{
    g_teleportTasks.push_back({ x, y, z, 0.05f, 0, true });
}
static constexpr float EPSILON = 0.001f; // small threshold
// Call this every frame
static void UpdateTeleports()
{
    for (auto it = g_teleportTasks.begin(); it != g_teleportTasks.end(); )
    {
        if (!it->active)
        {
            it = g_teleportTasks.erase(it);
            continue;
        }

        it->timer -= ImGui::GetIO().DeltaTime;
        if (it->timer > 0.0f)
        {
            ++it;
            continue;
        }

        switch (it->step)
        {
        case 0: // pre-state
            EntityPoolFunction(EntityPlayerPtr, 8, "entityState");
            it->timer = 0.5f;
            it->step++;
            break;

        case 1: // apply absolute position
        {
            float curX = 0, curY = 0, curZ = 0;

            // Read current values
            ReadEntityFloat(EntityPlayerPtr, "entityX", curX);
            ReadEntityFloat(EntityPlayerPtr, "entityY", curY);
            ReadEntityFloat(EntityPlayerPtr, "entityZ", curZ);

            // Use scheduled value if non-zero, otherwise keep current
            float finalX = (std::abs(it->x) > EPSILON) ? it->x : curX;
            float finalY = (std::abs(it->y) > EPSILON) ? it->y : curY;
            float finalZ = (std::abs(it->z) > EPSILON) ? it->z : curZ;

            // Write all axes together
            EntityPoolFunction(EntityPlayerPtr, FloatToU32(finalX), "entityX");
            EntityPoolFunction(EntityPlayerPtr, FloatToU32(finalY), "entityY");
            EntityPoolFunction(EntityPlayerPtr, FloatToU32(finalZ), "entityZ");

            it->timer = 0.05f;
            it->step++;
            break;
        }
        case 2: // reset state
            EntityPoolFunction(EntityPlayerPtr, 0, "entityState");
            it->active = false; // mark done
            break;
        }

        ++it;
    }
}
// Toggle state
static bool g_rightClickTeleportEnabled = false;

// Call from menu option
static void ToggleRightClickTeleport()
{
    g_rightClickTeleportEnabled = !g_rightClickTeleportEnabled;
    iprintln(g_rightClickTeleportEnabled ? "Right-click teleport: ON" : "OFF");
}

// Call every frame
static void HandleRightClickTeleport()
{
    if (!g_rightClickTeleportEnabled)
        return;

    static bool rmbHeld = false;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!rmbHeld)
        {
            rmbHeld = true;

            // Get mouse position on screen
            POINT p;
            if (!GetCursorPos(&p))
                return;

            // Current player position
            float curX = 0, curY = 0, curZ = 0;
            if (!ReadEntityFloat(EntityPlayerPtr, "entityX", curX)) return;
            if (!ReadEntityFloat(EntityPlayerPtr, "entityY", curY)) return;
            if (!ReadEntityFloat(EntityPlayerPtr, "entityZ", curZ)) return;

            // Screen center
            constexpr int SCREEN_CENTER_X = 960; // adjust for your resolution
            constexpr int SCREEN_CENTER_Y = 540;

            constexpr float CLICK_SCALE = 1.0f;

            // Apply inverted mouse offset to X/Y only
            float offsetX = (SCREEN_CENTER_X - p.x) * CLICK_SCALE; // inverted horizontal
            float offsetY = (SCREEN_CENTER_Y - p.y) * CLICK_SCALE; // inverted forward/back

            float worldX = curX + offsetX;
            float worldY = curY + offsetY;
            float worldZ = curZ; // keep height

            ScheduleTeleport(worldX, worldY, worldZ);
        }
    }
    else
    {
        rmbHeld = false;
    }
}
static void AddDeltaPosition(
    const std::string& axes,
    float dx = 0.0f,
    float dy = 0.0f,
    float dz = 0.0f
)
{
    auto addAxis = [&](const char* axisType, float delta)
        {
            if (delta == 0.0f) return; // skip if no change

            float current;
            if (!ReadEntityFloat(EntityPlayerPtr, axisType, current))
                return;

            EntityPoolFunction(EntityPlayerPtr, FloatToU32(current + delta), axisType);

        };

    if (axes == "x" || axes == "X")
        addAxis("entityX", dx);
    else if (axes == "y" || axes == "Y")
        addAxis("entityY", dy);
    else if (axes == "z" || axes == "Z")
        addAxis("entityZ", dz);
    else if (axes == "xyz" || axes == "XYZ")
    {
        addAxis("entityX", dx);
        addAxis("entityY", dy);
        addAxis("entityZ", dz);
    }
    else
    {
        iprintln("AddDeltaPosition: Unknown axis string");
    }
}
static void ForceStateRaw(uint8_t StateToForce, uint8_t AnimatonStateToForce, uint8_t activeStateSet)//probably might wanna look into the process getting hung up sometimes on the mod being installed?
{
    EntityPoolFunction(EntityPlayerPtr, AnimatonStateToForce, "animationState");
    EntityPoolFunction(EntityPlayerPtr, StateToForce, "entityState");
    EntityPoolFunction(EntityPlayerPtr, activeStateSet, "entityState");
    EntityPoolFunction(EntityPlayerPtr, AnimatonStateToForce, "animationState");
    if (AnimatonStateToForce == 14 || activeStateSet == 15 || StateToForce == 15)
    {
        EntityPoolFunction(EntityPlayerPtr, activeStateSet, "entityState");
        EntityPoolFunction(EntityPlayerPtr, AnimatonStateToForce, "animationState");
    }
}
// Updated ForceState
static void ForceState(const char* state)
{
    if (!state) return;

    std::string s = ToLower(state);
    
    if (s == "reset")
    {
        ForceStateRaw(0, 0, 0);
        EntityPoolFunction(EntityPlayerPtr, 0, "animationState");
    }
    if (s == "swim")
    {
        ForceStateRaw(17, 47, 2);
        EntityPoolFunction(EntityPlayerPtr, 47, "animationState");
    }
    else if (s == "jump")
    {
        ForceStateRaw(2, 10, 2);
        EntityPoolFunction(EntityPlayerPtr, 10, "animationState");
    }
    else if (s == "jump_moveup")
    {
        ForceStateRaw(3, 10, 3);
        EntityPoolFunction(EntityPlayerPtr, 10, "animationState");
    }
    else if (s == "spinningflip")
    {
        ForceStateRaw(2, 107, 2);
        EntityPoolFunction(EntityPlayerPtr, 107, "animationState");
    }
    else if (s == "peterpanflyglow")
    {
        ForceStateRaw(8, 75, 8);
        EntityPoolFunction(EntityPlayerPtr, 75, "animationState");
    }
    else if (s == "peterpanflynoglow")
    {
        ForceStateRaw(8, 75, 2);
        EntityPoolFunction(EntityPlayerPtr, 75, "animationState");
    }
    else if (s == "superman")
    {
        ForceStateRaw(8, 76, 2);
        EntityPoolFunction(EntityPlayerPtr, 76, "animationState");
    }
    else if (s == "walk")
    {
        ForceStateRaw(2, 2, 2);
        EntityPoolFunction(EntityPlayerPtr, 2, "animationState");
    }
    else if (s == "running")
    {
        ForceStateRaw(2, 5, 2);
        EntityPoolFunction(EntityPlayerPtr, 5, "animationState");
    }
    else if (s == "tarzan")
    {
        ForceStateRaw(24, 53, 24);
        EntityPoolFunction(EntityPlayerPtr, 53, "animationState");
    }
}
static void ForceJumpSora()
{
    constexpr float JUMP_FORCE_Z = -250.0f;       // how much to move Z
    constexpr uint64_t JUMP_COOLDOWN_MS = 250;    // minimum delay between jumps (ms)

    static uint64_t lastJumpTime = 0;
    uint64_t now = GetTickCount64();

    // Check cooldown
    if (now - lastJumpTime < JUMP_COOLDOWN_MS)
        return;

    // Apply jump
    AddDeltaPosition("z", 0.0f, 0.0f, JUMP_FORCE_Z);

    // Update last jump time
    lastJumpTime = now;
}

static void UnlimitedJump()
{
    static bool unljump = false; // toggle state
    constexpr float Z_STEP = -30.0f; // upward movement per tick

    // Flip toggle every time function is called
    unljump = !unljump;

    if (!unljump)
    {
        iprintln("Unlimited Jump: OFF");
        return;
    }

    iprintln("Unlimited Jump: ON");

    // While the toggle is ON, apply movement if space is held
    std::thread([]
        {
            while (unljump)
            {
                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                {
                    AddDeltaPosition("z", 0.0f, 0.0f, Z_STEP);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            }
        }).detach();
}
static void JetpackToggle()
{
    static bool jetpackOn = false;       // toggle state
    static float fuel = 50000.0f;          // max fuel
    static const float maxFuel = 50000.0f;
    static const float fuelBurnPerTick = 35.0f;  // fuel spent per tick
    static const float fuelRechargePerTick = 5.0f; // fuel regained per tick
    static const float Z_STEP = -15.0f;  // movement per tick

    // toggle on/off each call
    jetpackOn = !jetpackOn;

    if (!jetpackOn)
    {
        iprintln("Jetpack: OFF");
        return;
    }

    iprintln("Jetpack: ON");

    // movement thread
    std::thread([]
        {
            while (jetpackOn)
            {
                bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

                if (spaceDown && fuel > 0.0f)
                {
                    // move player up
                    AddDeltaPosition("z", 0.0f, 0.0f, Z_STEP);

                    // burn fuel
                    fuel -= fuelBurnPerTick;
                    if (fuel < 0.0f) fuel = 0.0f;
                }
                else
                {
                    // recharge fuel when not using
                    fuel += fuelRechargePerTick;
                    if (fuel > maxFuel) fuel = maxFuel;
                }

                // optional: show fuel in console
                static int tick = 0;
                if (++tick % 60 == 0) // print once per second (~60 ticks)
                    iprintln("Jetpack Fuel: %.1f", fuel);

                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            }
        }).detach();
}

static void SaveLoadPosition()
{
    // Static variables to hold saved position
    static bool positionSaved = false;
    static float savedX = 0.0f, savedY = 0.0f, savedZ = 0.0f;

    float currentX, currentY, currentZ;

    if (!positionSaved)
    {
        // Read current position
        if (!ReadEntityFloat(EntityPlayerPtr, "entityX", currentX)) return;
        if (!ReadEntityFloat(EntityPlayerPtr, "entityY", currentY)) return;
        if (!ReadEntityFloat(EntityPlayerPtr, "entityZ", currentZ)) return;

        // Save it
        savedX = currentX;
        savedY = currentY;
        savedZ = currentZ;
        positionSaved = true;

        iprintln("[Position] Saved!");
    }
    else
    {
        // Restore saved position
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(savedX), "entityX");
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(savedY), "entityY");
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(savedZ), "entityZ");

        positionSaved = false;
        iprintln("[Position] Restored!");
    }
}
static void MarioModeToggle()
{
    static std::atomic<bool> marioModeOn{ false };
    static std::thread marioThread;

    // toggle
    marioModeOn = !marioModeOn;

    if (!marioModeOn)
    {
        iprintln("Mario Mode: OFF");

        // reset colors to default
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(1.0f), "entityRed");
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(1.0f), "entityGreen");
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(1.0f), "entityBlue");

        // stop thread if running
        if (marioThread.joinable())
        {
            marioThread.join();
        }
        return;
    }

    iprintln("Mario Mode: ON");

    // start color cycling thread
    marioThread = std::thread([]()
        {
            float t = 0.0f;
            while (marioModeOn)
            {
                // sine wave values between 0.0 and 1.0
                float r = std::clamp(0.5f * (std::sin(t) + 1.0f), 0.0f, 1.0f);
                float g = std::clamp(0.5f * (std::sin(t + 2.0f) + 1.0f), 0.0f, 1.0f);
                float b = std::clamp(0.5f * (std::sin(t + 4.0f) + 1.0f), 0.0f, 1.0f);

                EntityPoolFunction(EntityPlayerPtr, FloatToU32(r), "entityRed");
                EntityPoolFunction(EntityPlayerPtr, FloatToU32(g), "entityGreen");
                EntityPoolFunction(EntityPlayerPtr, FloatToU32(b), "entityBlue");

                t += 0.1f; // speed of color change
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

    marioThread.detach();
}

static void NoclipMode(const char* state)
{
    if (!gNoclipModeEnabled.load())
    {
        gNoclipModeEnabled.store(true);
        SetCollision(false);
        ForceState(state);

        gNoclipModeThread = std::thread([state]()
            {
                float zVelocity = 0.0f;          // Current vertical speed
                const float speedIncrement = 0.3f; // How much velocity changes per tick
                const float maxSpeed = 6.65f;      // Maximum delta per tick
                const float damping = 0.9f;       // Slowdown factor per tick

                while (gNoclipModeEnabled.load())
                {
                    bool moved = false;
                    float currentZ = 0.0f;

                    // Read current Z
                    if (!ReadEntityFloat(EntityPlayerPtr, "entityZ", currentZ))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(4));
                        continue; // skip if read fails
                    }

                    // UP
                    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                    {
                        iprintln("move up");
                        zVelocity -= speedIncrement;
                        if (zVelocity < -maxSpeed) zVelocity = -maxSpeed;
                        moved = true;
                    }

                    // DOWN
                    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
                    {
                        iprintln("move down");
                        zVelocity += speedIncrement;
                        if (zVelocity > maxSpeed) zVelocity = maxSpeed;
                        moved = true;
                    }

                    // Apply damping if no input
                    if (!moved)
                    {
                        zVelocity *= damping;
                        if (fabs(zVelocity) < 0.01f)
                            zVelocity = 0.0f;
                    }

                    // Calculate final Z and set it directly
                    float finalZ = currentZ + zVelocity;
                    EntityPoolFunction(EntityPlayerPtr, FloatToU32(finalZ), "entityZ");

                    // Keep noclip stable
                    SetCollision(false);
                    if (!moved)
                        ForceState(state);

                    std::this_thread::sleep_for(std::chrono::milliseconds(4));
                }
            });

        iprintln("Noclip Mode: ON (STATE FROZEN)");
    }
    else
    {
        gNoclipModeEnabled.store(false);

        if (gNoclipModeThread.joinable())
            gNoclipModeThread.join();

        SetCollision(true);
        ForceState("reset");

        iprintln("Noclip Mode: OFF");
    }
}

static void UfoMode()
{
    static bool enabled = false; // remembers state between calls
    enabled = !enabled;          // toggle state
    if (enabled)
    {
        iprintln("Ufomode: ^2On");
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(0.0f), "entityAlpha"); // ON
        NoclipMode("walk");
    }
    else
    {
        iprintln("Ufomode: ^1Off");
        EntityPoolFunction(EntityPlayerPtr, FloatToU32(1.0f), "entityAlpha"); // OFF
        NoclipMode("reset");
    }
}


static uintptr_t gLastPrintedAddr = 0;
static bool GetModuleFromAddress(void* addr, char* outName, size_t outSize, uintptr_t& outBase)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (!EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded))
        return false;

    for (unsigned int i = 0; i < cbNeeded / sizeof(HMODULE); i++)
    {
        MODULEINFO mi{};
        if (!GetModuleInformation(GetCurrentProcess(), hMods[i], &mi, sizeof(mi)))
            continue;

        uintptr_t start = (uintptr_t)mi.lpBaseOfDll;
        uintptr_t end = start + mi.SizeOfImage;

        if ((uintptr_t)addr >= start && (uintptr_t)addr < end)
        {
            outBase = start;
            GetModuleBaseNameA(GetCurrentProcess(), hMods[i], outName, (DWORD)outSize);
            return true;
        }
    }

    return false;
}

// -----------------------------
// Patch monitor thread
// -----------------------------
static void MonitorLevelPatches()
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
static void StartPatchMonitor()
{
    static bool started = false;
    if (!started)
    {
        std::thread(MonitorLevelPatches).detach();
        started = true;
    }
}
static void SetLevelModel(const std::string& model, const std::string& moveset)
{
    std::lock_guard<std::mutex> lock(gPatchMutex);

    gMapModelString = model;
    gMapMoveSetString = moveset;

    // Strip ".mdls" if present
    std::string displayName = model;
    const std::string suffix = ".mdls";
    if (displayName.size() >= suffix.size() &&
        displayName.compare(displayName.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        displayName.erase(displayName.size() - suffix.size());
    }

    iprintln(displayName.c_str()); // <-- pass C-string here

    StartPatchMonitor();
}

// -------------------- GENERIC PARTY MEMBER ENTITY STATE --------------------

static void SetPartyMemFlyMode(const char* partyMember)
{
    const uint8_t FLY_STATE = 8;

    // determine party member
    const char* partyMemberSelection = nullptr;

    if (strcmp(partyMember, "donald") == 0)
        partyMemberSelection = "entityStatePrtyMem1";
    else if (strcmp(partyMember, "goofy") == 0)
        partyMemberSelection = "entityStatePrtyMem2";

    if (!partyMemberSelection)
    {
        iprintln("Unknown party member!");
        return;
    }

    if (!gFlyModeEnabled)
    {
        gFlyModeEnabled = true;
        gCurrentState = FLY_STATE;

        // Capture pointer by value for thread
        gFlyThread = std::thread([partyMemberSelection]() {
            while (gFlyModeEnabled)
            {
                EntityPoolFunction(EntityPlayerPtr, FLY_STATE, partyMemberSelection);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            });

        iprintln("Fly Mode: ON");
    }
    else
    {
        gFlyModeEnabled = false;
        gCurrentState = 0;
        EntityPoolFunction(EntityPlayerPtr, 0, partyMemberSelection);

        if (gFlyThread.joinable())
            gFlyThread.join();

        iprintln("Fly Mode: OFF");
    }
}

static void PrintCurrentLevel()
{
    std::string currentLevel = GetCurrentLevelArea();
    iprintln(("Current Level: " + currentLevel).c_str(), true, 30);
}

static void setAnimationSpeed(float speed)
{
    EntityPoolFunction(EntityPlayerPtr, FloatToU32(speed), "animationState");
}
// ----------------------------
// Menu Structure (Limitless text & options)
// ----------------------------
#pragma endregion

#define function void

function mainMenu()
{
    //Menu Structure
    self addMenuTitle("^1Limit^7less ^1v1");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^1Limit^7less ^1v1", "Main Mods", []() { subMenu("Main Mods", MainMods); });
    self addMenuOpt("^1Limit^7less ^1v1", "^2Munny", []() { subMenu("^2Munny ^7Menu", MunnyMenu); });
    self addMenuOpt("^1Limit^7less ^1v1", "Fun Mods", []() { subMenu("Fun Mods", FunMods); });
    self addMenuOpt("^1Limit^7less ^1v1", "^5P^7lay ^5A^7s", []() { subMenu("^5P^7lay ^5A^7s", ModelsMenu); });
    self addMenuOpt("^1Limit^7less ^1v1", "^3Worlds ^7Menu", []() { subMenu("^3Worlds ^7Menu", WorldMenu); });
    //self addMenuOpt("^1Limit^7less ^1v1", "AI Menu", &iprintln, "Afterlife ai: test4", true, 30);//entity is at 0x4B0 intervals
    self addMenuOpt("^1Limit^7less ^1v1", "Skills Menu", []() { subMenu("Skills Menu", SkillsMenu); });
    self addMenuOpt("^1Limit^7less ^1v1", "^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", []() { subMenu("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", TeleportMenu); });
    //self addMenuOpt("^1Limit^7less ^1v1", "Clone Menu", []() { subMenu("Clone Menu", CloneMenu); });
    self addMenuOpt("^1Limit^7less ^1v1", "Message Menu", []() { subMenu("Self Msg", SelfMsg); });
    self addMenuOpt("^1Limit^7less ^1v1", "Items Menu", []() { subMenu("Items Menu", ItemsMenu); });
    self addMenuOpt("^1Limit^7less ^1v1", "Weapons Menu", []() { subMenu("Weapons Menu", WeaponsMenu); });
    //self addMenuOpt("^1Limit^7less ^1v1", "Ablitys Menu", []() { subMenu("Ablitys Menu", AblitysMenu); });//
    self addMenuOpt("^1Limit^7less ^1v1", "Sounds Menu", []() { subMenu("Sounds Menu", SoundsMenu); });
    //self addMenuOpt("^1Limit^7less ^1v1", "Forge Menu", []() { subMenu("Forge Menu", ForgeMenu); }); diy do it yourself
    //self addMenuOpt("^1Limit^7less ^1v1", "Visions Menu", []() { subMenu("Visions Menu", VisionsMenu); });
}

function MainMods()
{
    self addMenuTitle("Main Mods");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Main Mods", "Print Area Code", &PrintCurrentLevel);
    self addMenuOpt("Main Mods", "Peter Pan Flymode", &SetFlyMode);
    self addMenuOpt("Main Mods", "Party Mem1 Flymode", &SetPartyMemFlyMode, "goofy");
    self addMenuOpt("Main Mods", "Party Mem2 Flymode", &SetPartyMemFlyMode, "donald");//70
    self addMenuOpt("Main Mods", "Ufomode", &UfoMode);
    self addMenuOpt("Main Mods", "Godmode test", &SetSoraEntityState, 8);//SetNoclipFlyMode
    self addMenuOpt("Main Mods", "Noclip Menu", []() { subMenu("Noclip Menu", NoclipModes); });
    self addMenuOpt("Main Mods", "Restore Health", &NoclipMode, "swim");//test my niggah
    self addMenuOpt("Main Mods", "Restore Mana", &SetLevelModel, "xa_lm_0000.mdls", "xa_lm_0000.mset");//cloud = xa_ex_1151
    self addMenuOpt("Main Mods", "Invisible", &EntityPoolFunction, EntityPlayerPtr, FloatToU32(0.0f), "entityAlpha");
    self addMenuOpt("Main Mods", "Visible", &EntityPoolFunction, EntityPlayerPtr, FloatToU32(1.0f), "entityAlpha");
    self addMenuOpt("Main Mods", "Pro-mod", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Main Mods", "Kill yourself", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Main Mods", "Entitys Menu", []() { subMenu("Entitys Menu", EntityMenu); });
    self addMenuOpt("Main Mods", "Go Back", &backMenu);
}
function NoclipModes()
{
    self addMenuTitle("Noclip Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Noclip Menu", "Walking Noclip", &NoclipMode, "walk");
    self addMenuOpt("Noclip Menu", "Peterpan Noclip Glow", &NoclipMode, "peterpanflyglow");
    self addMenuOpt("Noclip Menu", "Peterpan Noclip NoGlow", &NoclipMode, "peterpanflynoglow");
    self addMenuOpt("Noclip Menu", "Sonic Noclip", &NoclipMode, "spinningflip");
    self addMenuOpt("Noclip Menu", "Superman Noclip Fly", &NoclipMode, "superman");
    self addMenuOpt("Noclip Menu", "Jump Noclip", &NoclipMode, "jump");
    self addMenuOpt("Noclip Menu", "Swim Noclip", &NoclipMode, "swim");
    self addMenuOpt("Noclip Menu", "Tarzan Noclip", &NoclipMode, "tarzan");
    self addMenuOpt("Noclip Menu", "Running Noclip", &NoclipMode, "running");
    self addMenuOpt("Noclip Menu", "Go Back", &backMenu);
}
function EntityMenu()
{
    self addMenuTitle("Entitys Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Entitys Menu", "Change Donalds Model", &iprintln, "will add in v2 if people like it", true, 30);
    self addMenuOpt("Entitys Menu", "Change Goofys Model", &iprintln, "will add in v2 if people like it", true, 30);
    self addMenuOpt("Entitys Menu", "Change Special Model", &iprintln, "will add in v2 if people like it", true, 30);
    self addMenuOpt("Entitys Menu", "Donald Godmode", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Donald Fly", &SetPartyMemFlyMode, "donald");
    self addMenuOpt("Entitys Menu", "Donald Inf mp", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Warp Donald", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Goofy Godmode", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Goofy Fly", &SetPartyMemFlyMode, "goofy");
    self addMenuOpt("Entitys Menu", "Goofy Inf mp", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Goofy Donald", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Other Entities", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Entitys Menu", "Go Back", &backMenu);
}
function MunnyMenu()
{
    self addMenuTitle("^2Munny ^7Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^2Munny ^7Menu", "^2+1$ ^7Munny", &giveMunny, 1);
    self addMenuOpt("^2Munny ^7Menu", "^2+10$ ^7Munny", &giveMunny, 10);
    self addMenuOpt("^2Munny ^7Menu", "^2+100$ ^7Munny", &giveMunny, 100);
    self addMenuOpt("^2Munny ^7Menu", "^2+1000$ ^7Munny", &giveMunny, 1000);
    self addMenuOpt("^2Munny ^7Menu", "^2+5000$ ^7Munny", &giveMunny, 5000);
    self addMenuOpt("^2Munny ^7Menu", "^2+10000$ ^7Munny", &giveMunny, 10000);
    self addMenuOpt("^2Munny ^7Menu", "^2+50000$ ^7Munny", &giveMunny, 50000);
    self addMenuOpt("^2Munny ^7Menu", "^2+100000$ ^7Munny", &giveMunny, 100000);
    self addMenuOpt("^2Munny ^7Menu", "^1-100000$ ^7Munny", &giveMunny, -100000);
    self addMenuOpt("^2Munny ^7Menu", "^1-50000$ ^7Munny", &giveMunny, -50000);
    self addMenuOpt("^2Munny ^7Menu", "^1-10000$ ^7Munny", &giveMunny, -10000);
    self addMenuOpt("^2Munny ^7Menu", "^1-5000$ ^7Munny", &giveMunny, -5000);
    self addMenuOpt("^2Munny ^7Menu", "^1-1000$ ^7Munny", &giveMunny, -1000);
    self addMenuOpt("^2Munny ^7Menu", "^1-100$ ^7Munny", &giveMunny, -100);
    self addMenuOpt("^2Munny ^7Menu", "^1-10$ ^7Munny", &giveMunny, -10);
    self addMenuOpt("^2Munny ^7Menu", "^1-1$ ^7Munny", &giveMunny, -1);
    self addMenuOpt("^2Munny ^7Menu", "Go Back", &backMenu);
}

function FunMods()
{
    self addMenuTitle("Fun Mods");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Fun Mods", "Force Jump", &ForceJumpSora);
    self addMenuOpt("Fun Mods", "Unlimited Jump", &UnlimitedJump);
    self addMenuOpt("Fun Mods", "Far Air Key Melee", &ToggleSuperAirHit);
    self addMenuOpt("Fun Mods", "Force T-bag", &Teabag);
    self addMenuOpt("Fun Mods", "Force Up", &ForceUp);
    self addMenuOpt("Fun Mods", "Save & Load", &SaveLoadPosition);
    self addMenuOpt("Fun Mods", "Jet-pack", &JetpackToggle);
    self addMenuOpt("Fun Mods", "Mario-Mode", &MarioModeToggle);//godmode,touch enemies to die, rainbow-skin
    self addMenuOpt("Fun Mods", "Freeze EntityState", &SetSoraEntityState, 4);
    self addMenuOpt("Fun Mods", "Freeze EntityState", &SetSoraEntityState, 0);
    self addMenuOpt("Fun Mods", "Go Back", &backMenu);
}
function ModelsMenu()
{
    self addMenuTitle("^5P^7lay ^5A^7s");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);//depending on the model u might wanna take weapon away
    self addMenuOpt("^5P^7lay ^5A^7s", "^5Sora ^7Menu", []() { subMenu("^5Sora ^7Menu", SoraMenu); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^9Goofy ^7Menu", []() { subMenu("^9Goofy ^7Menu", GoofyMenu); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^6Donald ^7Menu", []() { subMenu("^6Donald ^7Menu", DonaldMenu); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^1Halloween ^7World", []() { subMenu("^1Halloween ^7World", HalloweenWorld); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^4Atlantica ^7World", []() { subMenu("^4Atlantica ^7World", AtlanticaWorld); });//AtlanticaWorld
    self addMenuOpt("^5P^7lay ^5A^7s", "^5Alice ^7Wonderland", []() { subMenu("^5Alice ^7Wonderland", AliceWorld); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^3Peterpan ^7World", []() { subMenu("^3Peterpan ^7World", PeterpanWorld); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^8Beauty beast ^7World", []() { subMenu("^8Beauty beast ^7World", beautybeastWorld); });
    self addMenuOpt("^5P^7lay ^5A^7s", "^0Heart^1less Menu", []() { subMenu("^0Heart^1less Menu", HeartLessMenu); });
    self addMenuOpt("^5P^7lay ^5A^7s", "Riku Soul Eater", SetLevelModel, "xa_ex_1560.mdls", "xa_ex_1560.mset");
    self addMenuOpt("^5P^7lay ^5A^7s", "Dark Riku", SetLevelModel, "xa_ex_1160.mdls", "xa_ex_1160.mset");
    self addMenuOpt("^5P^7lay ^5A^7s", "Tidus", SetLevelModel, "xa_di_1010.mdls", "xa_di_1010.mset");
    self addMenuOpt("^5P^7lay ^5A^7s", "Kairi", SetLevelModel, "xa_ex_1020.mdls", "xa_ex_1020.mset");
    self addMenuOpt("^5P^7lay ^5A^7s", "Go Back", &backMenu);
}

function SoraMenu()
{
    self addMenuTitle("^5Sora ^7Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^5Sora ^7Menu", "Normal Sora", SetLevelModel, "xa_ex_0010.mdls", "xa_ex_0010.mset");
    self addMenuOpt("^5Sora ^7Menu", "Atlantic Sora", SetLevelModel, "xa_lm_0000.mdls", "xa_lm_0000.mset");
    self addMenuOpt("^5Sora ^7Menu", "Halloween Sora", SetLevelModel, "xa_nm_0000.mdls", "xa_nm_0000.mset");
    self addMenuOpt("^5Sora ^7Menu", "Showdow 0 Sora", SetLevelModel, "xa_pp_3020.mdls", "xa_pp_3020.mset");
    self addMenuOpt("^5Sora ^7Menu", "Shadow 1 Sora", SetLevelModel, "xa_pp_3030.mdls", "xa_pp_3030.mset");
    self addMenuOpt("^5Sora ^7Menu", "Sora as a kid", SetLevelModel, "xa_ex_1590.mdls", "xa_ex_1590.mset");
    self addMenuOpt("^5Sora ^7Menu", "Go Back", &backMenu);
}

function GoofyMenu()
{
    self addMenuTitle("^9Goofy ^7Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^9Goofy ^7Menu", "Normal Goofy", SetLevelModel, "xa_dc_1010.mdls", "xa_dc_1010.mset");
    self addMenuOpt("^9Goofy ^7Menu", "Normal Goofy 2", SetLevelModel, "xa_ex_0050.mdls", "xa_ex_0050.mset");
    self addMenuOpt("^9Goofy ^7Menu", "Atlantic Goofy", SetLevelModel, "xa_lm_0020.mdls", "xa_lm_0020.mset");
    self addMenuOpt("^9Goofy ^7Menu", "Halloween Goofy", SetLevelModel, "xa_nm_0020.mdls", "xa_nm_0020.mset");
    self addMenuOpt("^9Goofy ^7Menu", "Knight Goofy", SetLevelModel, "xa_dc_1010.mdls", "xa_dc_1010.mset");
    self addMenuOpt("^9Goofy ^7Menu", "Go Back", &backMenu);
}

function DonaldMenu()
{
    self addMenuTitle("^6Donald ^7Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^6Donald ^7Menu", "Angry Donald", SetLevelModel, "xa_ex_1650.mdls", "xa_ex_1650.mset");
    self addMenuOpt("^6Donald ^7Menu", "Normal Donald", SetLevelModel, "xa_ex_0040.mdls", "xa_ex_0040.mset");
    self addMenuOpt("^6Donald ^7Menu", "Atlantic Donald", SetLevelModel, "xa_lm_0010.mdls", "xa_lm_0010.mset");
    self addMenuOpt("^6Donald ^7Menu", "Halloween Donald", SetLevelModel, "xa_nm_0010.mdls", "xa_nm_0010.mset");
    self addMenuOpt("^6Donald ^7Menu", "Wizard Donald", SetLevelModel, "xa_dc_1000.mdls", "xa_dc_1000.mset");
    self addMenuOpt("^6Donald ^7Menu", "Go Back", &backMenu);
}

function HalloweenWorld()
{
    self addMenuTitle("^1Halloween ^7World");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^1Halloween ^7World", "Jack Skellington", SetLevelModel, "xa_nm_0030.mdls", "xa_nm_0030.mset");
    self addMenuOpt("^1Halloween ^7World", "Sally", SetLevelModel, "xa_nm_1000.mdls", "xa_nm_1000.mset");
    self addMenuOpt("^1Halloween ^7World", "Dr.Finkelstein", SetLevelModel, "xa_nm_1028.mdls", "xa_nm_1028.mset");
    self addMenuOpt("^1Halloween ^7World", "Mayor", SetLevelModel, "xa_nm_1060.mdls", "xa_nm_1060.mset");
    self addMenuOpt("^1Halloween ^7World", "(Big) Oogie Boogie", SetLevelModel, "xa_nm_3000.mdls", "xa_nm_3000.mset");
    self addMenuOpt("^1Halloween ^7World", "Heartless Shadow", SetLevelModel, "xa_ex_2021.mdls", "xa_ex_2021.mset");
    self addMenuOpt("^1Halloween ^7World", "Go Back", &backMenu);
}

function AtlanticaWorld()
{
    self addMenuTitle("^4Atlantica ^7World");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^4Atlantica ^7World", "Aerial", SetLevelModel, "xa_lm_0030.mdls", "xa_lm_0030.mset");
    self addMenuOpt("^4Atlantica ^7World", "King Triton", SetLevelModel, "xa_lm_1000.mdls", "xa_lm_1000.mset");
    self addMenuOpt("^4Atlantica ^7World", "Sebastian", SetLevelModel, "xa_lm_1010.mdls", "xa_lm_1010.mset");
    self addMenuOpt("^4Atlantica ^7World", "Ursula", SetLevelModel, "xa_lm_1050.mdls", "xa_lm_1050.mset");
    self addMenuOpt("^4Atlantica ^7World", "Shark", SetLevelModel, "xa_lm_3030.mdls", "xa_lm_3030.mset");
    self addMenuOpt("^4Atlantica ^7World", "Ursula Boss", SetLevelModel, "xa_lm_3000.mdls", "xa_lm_3000.mset");
    self addMenuOpt("^4Atlantica ^7World", "Go Back", &backMenu);
}

function AliceWorld()
{
    self addMenuTitle("^5Alice ^7Wonderland");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^5Alice ^7Wonderland", "White Rabbit", SetLevelModel, "xa_aw_1000.mdls", "xa_aw_1000.mset");
    self addMenuOpt("^5Alice ^7Wonderland", "Cheshire Cat", SetLevelModel, "xa_aw_1020.mdls", "xa_aw_1020.mset");
    self addMenuOpt("^5Alice ^7Wonderland", "Ace of Spades", SetLevelModel, "xa_aw_1030.mdls", "xa_aw_1030.mset");
    self addMenuOpt("^5Alice ^7Wonderland", "Ace of Hearts", SetLevelModel, "xa_aw_1060.mdls", "xa_aw_1060.mset");
    self addMenuOpt("^5Alice ^7Wonderland", "Queen Of Hearts", SetLevelModel, "xa_aw_1010.mdls", "xa_aw_1010.mset");
    self addMenuOpt("^5Alice ^7Wonderland", "Go Back", &backMenu);
}

function PeterpanWorld()
{
    self addMenuTitle("^3Peterpan ^7World");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^3Peterpan ^7World", "Peter Pan", SetLevelModel, "xa_pp_0000.mdls", "xa_pp_0000.mset");
    self addMenuOpt("^3Peterpan ^7World", "Wendy", SetLevelModel, "xa_pp_1010.mdls", "xa_pp_1010.mset");
    //self addMenuOpt("^3Peterpan ^7World", "Smee", SetLevelModel, "xa_pp_1020.mdls", "xa_pp_1020.mset");
    self addMenuOpt("^3Peterpan ^7World", "(Big) Crocodile", SetLevelModel, "xa_pp_1040.mdls", "xa_pp_1040.mset");
    self addMenuOpt("^3Peterpan ^7World", "Captain Hook", SetLevelModel, "xa_pp_3000.mdls", "xa_pp_3000.mset");
    self addMenuOpt("^3Peterpan ^7World", "Phantom", SetLevelModel, "xa_pp_3010.mdls", "xa_pp_3010.mset");
    self addMenuOpt("^3Peterpan ^7World", "Go Back", &backMenu);
}

function beautybeastWorld()
{
    self addMenuTitle("^8Beauty beast ^7World");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^8Beauty beast ^7World", "The Beast", SetLevelModel, "xa_pc_0000.mdls", "xa_pc_0000.mset");
    self addMenuOpt("^8Beauty beast ^7World", "Belle", SetLevelModel, "xa_pc_1000.mdls", "xa_pc_1000.mset");
    self addMenuOpt("^8Beauty beast ^7World", "Aurora", SetLevelModel, "xa_pc_1010.mdls", "xa_pc_1010.mset");
    //self addMenuOpt("^8Beauty beast ^7World", "Alice", SetLevelModel, "xa_pc_1019.mdls", "xa_pc_1019.mset");
    self addMenuOpt("^8Beauty beast ^7World", "Cinderella", SetLevelModel, "xa_pc_1030.mdls", "xa_pc_1030.mset");
    self addMenuOpt("^8Beauty beast ^7World", "Snow White", SetLevelModel, "xa_pc_1040.mdls", "xa_pc_1040.mset");
    self addMenuOpt("^8Beauty beast ^7World", "Go Back", &backMenu);
}//HeartLessMenu
function HeartLessMenu()
{
    self addMenuTitle("^0Heart^1less Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^0Heart^1less Menu", "Big Guard Armor", SetLevelModel, "xa_tw_3000.mdls", "xa_tw_3000.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Soldier", SetLevelModel, "xa_ex_2010.mdls", "xa_ex_2010.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Shadow", SetLevelModel, "xa_ex_2020.mdls", "xa_ex_2020.mset");
    //self addMenuOpt("^0Heart^1less Menu", "Heartless Shadow HT", SetLevelModel, "xa_ex_2021.mdls", "xa_ex_2021.mset");
    //self addMenuOpt("^0Heart^1less Menu", "Heartless Shadow EW", SetLevelModel, "xa_ex_2022.mdls", "xa_ex_2022.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Powerwilds", SetLevelModel, "xa_ex_2030.mdls", "xa_ex_2030.mset");
    self addMenuOpt("^0Heart^1less Menu", "(Big) Heartless Agrabah", SetLevelModel, "xa_ex_2060.mdls", "xa_ex_2060.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Sea Neon", SetLevelModel, "xa_ex_2070.mdls", "xa_ex_2070.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Bandit", SetLevelModel, "xa_ex_2090.mdls", "xa_ex_2090.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Pirate", SetLevelModel, "xa_ex_2100.mdls", "xa_ex_2100.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Red Nocturne", SetLevelModel, "xa_ex_2110.mdls", "xa_ex_2110.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Pot Spider", SetLevelModel, "xa_ex_2170.mdls", "xa_ex_2170.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Gargoyle", SetLevelModel, "xa_ex_2220.mdls", "xa_ex_2220.mset");
    self addMenuOpt("^0Heart^1less Menu", "Heartless Search Ghost", SetLevelModel, "xa_ex_2230.mdls", "xa_ex_2230.mset");
    self addMenuOpt("^0Heart^1less Menu", "Go Back", &backMenu);
}

/*
    self addMenuOpt("Play As", "Sora (Atlantica)", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Play As", "Sora (Halloween Town)", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Play As", "Shadow Sora", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Play As", "Sora", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
*/
function WorldMenu()
{
    self addMenuTitle("^3Worlds ^7Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^3Worlds ^7Menu", "Slow Animations", &setAnimationSpeed, 0.0005f);
    self addMenuOpt("^3Worlds ^7Menu", "Fast Animations", &setAnimationSpeed, 3.0f);
    self addMenuOpt("^3Worlds ^7Menu", "Super Fast Animations", &setAnimationSpeed, 50.0f);
    self addMenuOpt("^3Worlds ^7Menu", "Normal Animations", &setAnimationSpeed, 1.0f);
    self addMenuOpt("^3Worlds ^7Menu", "Super Speed", &ToggleSuperSpeed);
    self addMenuOpt("^3Worlds ^7Menu", "Super Slow", &ToggleSlowSpeed);
    self addMenuOpt("^3Worlds ^7Menu", "Freeze", &SetSoraEntityState, 4);
    self addMenuOpt("^3Worlds ^7Menu", "Normal", &SetSoraEntityState, 0);
    self addMenuOpt("^3Worlds ^7Menu", "Low Gravity", &ToggleLowGravity);
    self addMenuOpt("^3Worlds ^7Menu", "Super Jump", &ToggleSuperJump);
    self addMenuOpt("^3Worlds ^7Menu", "Go Back", &backMenu);
}
function LevelsMenu()
{
    self addMenuTitle("Levels Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Levels Menu", "Unknown Level", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Levels Menu", "Go Back", &backMenu);
}
/*
* when i have more time...
static void VisionsMenu()
{
    self addMenuTitle("Visions Menu");
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Visions Menu", "Black & Grey", ApplyBlackAndGrey);
    self addMenuOpt("Visions Menu", "Blur", ApplyBlur, 5.0f);
    self addMenuOpt("Visions Menu", "Change Tint", ApplyTint, 1.0f, 0.0f, 0.0f);
    self addMenuOpt("Visions Menu", "Change Bloom", ApplyBloom, 1.5f);
    self addMenuOpt("Visions Menu", "Change Light", ApplyLight, 1.0f);
    self addMenuOpt("Visions Menu", "Cartoon", ApplyCartoon);
    self addMenuOpt("Visions Menu", "Camera Fov", &iprintln, "Change Camera FOV", true, 30);
    self addMenuOpt("Visions Menu", "Normal Animations", &iprintln, "Restore Animations", true, 30);
    self addMenuOpt("Visions Menu", "Change Color Matrix", ApplyColorMatrix);
    self addMenuOpt("Visions Menu", "Change Film Grain Color", ApplyFilmGrainColor, 1.0f, 1.0f, 1.0f);
    self addMenuOpt("Visions Menu", "Chrome Mode", ApplyChromeMode);
    self addMenuOpt("Visions Menu", "Increase Gloss", &AdjustGloss, true);
    self addMenuOpt("Visions Menu", "Decrease Gloss", &AdjustGloss, false);
    self addMenuOpt("Visions Menu", "Go Back", &backMenu);
}
*/
function SkillsMenu()
{
    self addMenuTitle("Skills Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Skills Menu", "+1 health point", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "-1 health point", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "+1 mana point", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "-1 mana point", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "+1 strength point", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "-1 stength point", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "+1 level", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "-1 level", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "+1 exp", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "-1 exp", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "Exp to next lvl 0", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Skills Menu", "Go Back", &backMenu);
}
function TeleportMenu()
{
    self addMenuTitle("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Teleport to mouse", &ToggleRightClickTeleport);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Delta Y Forward", &AddDeltaPosition, "y", 0.0f, 250.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Delta Y Backwards", &AddDeltaPosition, "y", 0.0f, -250.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Delta X Left", &AddDeltaPosition, "x", 250.0f, 0.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Delta X Right", &AddDeltaPosition, "x", -250.0f, 0.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Delta Z Up", &AddDeltaPosition, "z", 0.0f, 0.0f, -250.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Delta Z Down", &AddDeltaPosition, "z", 0.0f, 0.0f, 250.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Tele Y Forward", &ScheduleTeleport, 0.0f, 250.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Tele Y Backwards", &ScheduleTeleport, 0.0f, -250.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Tele X Left", &ScheduleTeleport, 250.0f, 0.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Tele X Right", &ScheduleTeleport, -250.0f, 0.0f, 0.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Tele Z Up", &ScheduleTeleport, 0.0f, 0.0f, -250.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Tele Z Down", &ScheduleTeleport, 0.0f, 0.0f, 250.0f);
    self addMenuOpt("^4T^7e^4l^7e^4p^7o^4r^7t ^4M^7e^4n^7u", "Go Back", &backMenu);
}
/*
function CloneMenu()
{
    self addMenuTitle("Clone Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Clone Menu", "Clone Self", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Clone Menu", "Clone Tp to me", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Clone Menu", "Clone Entity State", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Clone Menu", "Clone Kill", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Clone Menu", "Clone Respawn", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Clone Menu", "Go Back", &backMenu);
}*/
function SelfMsg()
{
    self addMenuTitle("Self Msg");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Self Msg", "Hello", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Self Msg", "???? What", &iprintln, "Afterlife ai: WTF DO YOU WANT!", true, 30);
    self addMenuOpt("Self Msg", "bROOO", &iprintln, "Afterlife ai: I will not bro, fuck you", true, 30);
    self addMenuOpt("Self Msg", "CHEETO'S", &iprintln, "Afterlife ai: CHEETO'S ON JAYCODER YT", true, 30);
    self addMenuOpt("Self Msg", "Thank you", &iprintln, "Afterlife ai: thank you for using the menu", true, 30);
    self addMenuOpt("Self Msg", "Check out yt", &iprintln, "Afterlife ai: YT: Jaycoder", true, 30);
    self addMenuOpt("Self Msg", "life is fake", &iprintln, "Life is fake asf prove me wrong!\nDon't take it out of context\nwhat i mean is i work hard and have nothing to show for it\nfuck you", true, 30);
    self addMenuOpt("Self Msg", "I EAT ASSS BITCH", &iprintln, "Afterlife ai: YT: BITCH I EAT ASS", true, 30);
    self addMenuOpt("Self Msg", "8====> ~ ~ ~", &iprintln, "Afterlife ai: YT: YUP RIGHT ON YOUR MUM", true, 30);
    self addMenuOpt("Self Msg", "A ROOM OF WHITEGUYS", &iprintln, "Afterlife ai: IS A BOX OF CRACKERS ^0NIGGAH", true, 30);
    self addMenuOpt("Self Msg", "YOU MAD???", &iprintln, "Afterlife ai: I got cheeto's", true, 30);
    self addMenuOpt("Self Msg", "PUSSY ^4<3", &iprintln, "Afterlife ai: cheeto's for pussy ^4<3", true, 30);
    self addMenuOpt("Self Msg", "615", &iprintln, "Afterlife ai: I Made goofy my bitch", true, 30);
    self addMenuOpt("Self Msg", "911", &iprintln, "Afterlife ai: Mickey Loves Kids ;)", true, 30);
    self addMenuOpt("Self Msg", "Go Back", &backMenu);
}
function ItemsMenu()
{
    self addMenuTitle("Items Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Items Menu", "+1 Coconut", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Items Menu", "-1 Coconut", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Items Menu", "Go Back", &backMenu);
}
function WeaponsMenu()
{
    self addMenuTitle("Weapons Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Weapons Menu", "Kingdom Keyblade", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Jungle Keyblade", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Wooden Sword", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Olympia", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Lionheart", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Oath Keeper", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Oblivion", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Wishing Star", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Ultima Weapon", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "One-Winged Angel", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Mage Staff", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Morning Star", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Shooting Star", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Magus Staff", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Wisdom Staff", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Warhammer", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Silver Mallet", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Grand Mallet", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Dream Rod", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Save the Queen", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Wizard Relic", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Meteor Strike", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Fantasia", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "No Weapon", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Spear", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Dagger", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Claws", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Dagger", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Dream Sword", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Dream Shield", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Three Wishes", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Fairy Harp", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Pumpkin Head", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Crabclaw", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Divine Rose", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Spellbinder", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Oblivion", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Lady Luck", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Diamond Dust", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Metal Chocobo", &setWeaponIndex, 81);
    self addMenuOpt("Weapons Menu", "Go Back", &backMenu);
}
function AblitysMenu()
{
    self addMenuTitle("Ablitys Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Ablitys Menu", "Unlock Roll Early", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Ablitys Menu", "Unlock Thunder Spell", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Ablitys Menu", "Go Back", &backMenu);
}
function SoundsMenu()
{
    self addMenuTitle("Sounds Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Sounds Menu", "Test sound 1", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Sounds Menu", "Test sound 2", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Sounds Menu", "Go Back", &backMenu);
}//ForgeMenu
function ForgeMenu()
{
    self addMenuTitle("Forge Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Forge Menu", "Pick Up Entity", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Forge Menu", "Drop Entity", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Forge Menu", "Teleport Entity", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Forge Menu", "Edit Entity", &iprintln, "Afterlife ai: Yooo Bro, Hello", true, 30);
    self addMenuOpt("Forge Menu", "Go Back", &backMenu);
}
function EditMenu()
{
    self addMenuTitle("Edit Menu");//Title
    self addVertText("P\nr\no\nj\ne\nc\nt\n \nL\ni\nm\ni\nt\nl\ne\ns\ns\n \nv\n1\n", 22.5);
    self addMenuOpt("Edit Menu", "Change BG Color", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Edit Menu", "Change Line Color", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Edit Menu", "Change Scroller Color", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Edit Menu", "Line Thickness", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Edit Menu", "Font Size", &iprintln, "Afterlife ai: jay was mf here!", true, 30);
    self addMenuOpt("Edit Menu", "Go Back", &backMenu);
}
#pragma region MenuUtils
// ----------------------------
// Menu UI
// ----------------------------
namespace ProjectX
{
    static void MenuUtils() {
        ImGui::SetNextWindowPos(g_windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(g_windowSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse;

        //InstallModelObserver();

        ImGui::Begin("##ShaderTogglerOverlay", nullptr, flags);

        // Handle toggle input
        self MenuControls();
        UpdateTeleports();
        HandleRightClickTeleport();

        // SHOW/HIDE MOUSE CURSOR BASED ON MENU VISIBILITY
        ImGui::GetIO().MouseDrawCursor = g_menuVisible;

        if (!g_menuVisible) {
            ImGui::End();
            DrawMessageBox(); // still draw messages even if menu hidden
            return;
        }

        HandleWindowDrag();

        ImVec2 winPos = ImGui::GetWindowPos();
        self BeginMenu();
        g_menuItemCount = 0;
        float deltaTime = ImGui::GetIO().DeltaTime;
        self UpdateMenuWidth(deltaTime);

        // Draw menu box and HUD
        self menuHuds(winPos);

        // Draw current menu options
        if (g_currentMenuFunc)
            g_currentMenuFunc();

        ImGui::End();

        // Draw your temporary message box
        DrawMessageBox();
    }
}
#pragma endregion