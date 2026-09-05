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
    // The "Ajuda" button opens a centered popup explaining each event (what it
    // is and where to join), scrollable with the mouse wheel.
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
            // Shared empty button texture (message-box manager owns the bitmap).
            IMAGE_EVENTS_BTN_HELP = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL,
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
            // "Ajuda" button on the bottom-right corner, mirroring the exit
            // button (13 px inset). Uses the shared message-box empty button
            // texture (owned by CNewUIMessageBoxMng, never reloaded here).
            HELP_BUTTON_X = WINDOW_WIDTH - 13 - 54,
            HELP_BUTTON_Y = 395,
            HELP_BUTTON_WIDTH = 54,
            HELP_BUTTON_HEIGHT = 23,
            // Help popup ("telinha"), centered over this window. Same frame
            // pieces as the schedule window itself.
            HELP_WIDTH = 300,
            HELP_HEIGHT = 380,
            HELP_CONTENT_LEFT = 22,
            HELP_CONTENT_TOP = 40,
            HELP_LINE_HEIGHT = 13,
            HELP_VISIBLE_LINES = (HELP_HEIGHT - HELP_CONTENT_TOP - FRAME_BOTTOM_HEIGHT - 6) / HELP_LINE_HEIGHT,
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
        // Draws the shared inventory-frame pieces (tiled msgbox_back stone,
        // 3-slice header with the baked close "X", side strips, bottom strip)
        // at an arbitrary rect. Used by both this window and the help popup.
        void RenderWindowFrame(float x, float y, float w, float h);
        void GetHelpRect(int* x, int* y) const;
        void RenderHelpWindow();
        void CloseHelp();
        static const wchar_t* StateText(BYTE state);

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_BtnExit;
        CNewUIButton m_BtnHelp;
        // Help popup state: opened by the "Ajuda" button, closed by its own
        // baked "X", a click outside the popup, ESC, or the window closing.
        bool m_HelpOpen;
        int m_HelpScroll;

        Entry m_Entries[kMaxEntries];
        int m_iEntryCount;
        int m_scrollOffset;
        bool m_bReceived;
        DWORD m_dwLastRequestTick;
        // Local wall-clock seconds-of-day when m_Entries' anchors were received;
        // display values are pure decays of the anchors (EventScheduleTime.h),
        // never mutated in place, so the rendered text cannot flicker.
        DWORD m_dwAnchorWallSec;
    };
}

// Routed from WSclient.cpp when the server sends C2 D6 00.
void ReceiveEventSchedule(const BYTE* buffer, int size);
