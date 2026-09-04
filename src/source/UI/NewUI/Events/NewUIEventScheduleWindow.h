#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    // "Eventos" window (hotkey O). Lists every event the connected channel actually
    // runs, together with the time until it starts / ends. The server owns the
    // schedule (C2 D6 00); this window only renders it and ticks the countdown
    // locally between refreshes so the numbers move every second.
    class CNewUIEventScheduleWindow : public CNewUIObj
    {
        enum eIMAGE_LIST
        {
            IMAGE_EVENTS_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_EVENTS_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_EVENTS_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_EVENTS_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_EVENTS_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_EVENTS_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
        };

        enum eWINDOW_SIZE
        {
            WINDOW_WIDTH = 320,
            WINDOW_HEIGHT = 429,
            FRAME_TOP_HEIGHT = 64,
            FRAME_SIDE_WIDTH = 21,
            FRAME_BOTTOM_HEIGHT = 45,
            FRAME_SIDE_TEXTURE_HEIGHT = 320,
        };

        enum eLAYOUT
        {
            TITLE_Y = 12,
            CONTENT_LEFT = 22,
            CONTENT_WIDTH = WINDOW_WIDTH - 2 * CONTENT_LEFT,
            HEADER_TOP = 44,
            CONTENT_TOP = 62,
            ROW_HEIGHT = 16,
            VISIBLE_ROWS = 20,
            COL_NAME_X = 0,
            COL_NAME_W = 100,
            COL_STATE_X = 102,
            COL_STATE_W = 74,
            COL_TIME_X = 178,
            COL_TIME_W = CONTENT_WIDTH - COL_TIME_X,
            EXIT_BUTTON_X = 13,
            EXIT_BUTTON_Y = 392,
            EXIT_BUTTON_WIDTH = 36,
            EXIT_BUTTON_HEIGHT = 29,
        };

    public:
        // Public so the UI system can center the window on the 640x480 layout.
        static constexpr int kWindowWidth = WINDOW_WIDTH;
        static constexpr int kWindowHeight = WINDOW_HEIGHT;

        // Mirrors MUnique.OpenMU.GameLogic.Events.EventScheduleState.
        enum eEVENT_STATE
        {
            EVENT_STATE_UPCOMING = 0,
            EVENT_STATE_OPEN = 1,
            EVENT_STATE_RUNNING = 2,
        };

        static constexpr int kMaxEntries = 32;
        static constexpr int kNameBytes = 24;
        static constexpr int kEntryBytes = 37;
        static constexpr int kHeaderBytes = 6;
        static constexpr BYTE kGroup = 0xD6;
        static constexpr BYTE kListSubCode = 0x00;
        static constexpr DWORD kRefreshIntervalMs = 15000;

        struct Entry
        {
            wchar_t Name[kNameBytes + 1];
            BYTE State;
            DWORD SecondsToStart;
            DWORD SecondsToEnd;
            WORD MinLevel;
            WORD MaxLevel;
        };

        CNewUIEventScheduleWindow();
        virtual ~CNewUIEventScheduleWindow();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();
        void SetPos(int x, int y);

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;

        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

        void OpenningProcess();
        void ClosingProcess();

        // Parses a C2 D6 00 payload coming from the game server.
        void ReceiveSchedule(const BYTE* buffer, int size);
        void RequestSchedule();

    private:
        void LoadImages();
        void UnloadImages();
        void InitButtons();
        void RenderBaseWindow();
        void TickCountdown();
        static const wchar_t* StateText(BYTE state);
        static void FormatDuration(DWORD seconds, wchar_t* target, size_t targetCount);
        // Wall-clock time (local HH:MM) at which an event starts, derived from the
        // seconds-until-start the server sent plus the client's current local time.
        static void FormatOpenClock(DWORD secondsFromNow, wchar_t* target, size_t targetCount);

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_BtnExit;

        Entry m_Entries[kMaxEntries];
        int m_iEntryCount;
        int m_scrollOffset;
        bool m_bReceived;
        DWORD m_dwLastRequestTick;
        DWORD m_dwLastCountdownTick;
    };
}

// Routed from WSclient.cpp when the server sends C2 D6 00.
void ReceiveEventSchedule(const BYTE* buffer, int size);
