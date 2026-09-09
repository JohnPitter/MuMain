#pragma once

#include "UI/NewUI/Changelog/ChangelogLayout.h" // shared wrap_text buffer size
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Maintenance/MaintenanceLayout.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    // "Manutenção" popup — the small sibling of the "Novidades" window. While
    // the notice is active on the server, the login push (C2 F3 EF, once per
    // login a few seconds after entering the world) opens this compact window:
    // title, optional schedule line, wrapped message and one OK button. No
    // scrolling, no expanded view, nothing else to ask the server for.
    class CNewUIMaintenanceWindow : public CNewUIObj
    {
        enum eIMAGE_LIST
        {
            IMAGE_MAINTENANCE_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_MAINTENANCE_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_MAINTENANCE_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_MAINTENANCE_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_MAINTENANCE_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_MAINTENANCE_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
            // Shared empty button texture for the labeled OK button — the same
            // native plate the changelog's "Mostrar tudo" button uses (the
            // message-box manager owns the bitmap; frames are 108x29 stacked
            // vertically: up / hover / press).
            IMAGE_MAINTENANCE_BTN_OK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY,
        };

        enum eWINDOW_SIZE
        {
            WINDOW_WIDTH = 260,
            WINDOW_HEIGHT = 256,
            FRAME_TOP_HEIGHT = 64,
            FRAME_SIDE_WIDTH = 21,
            FRAME_BOTTOM_HEIGHT = 45,
            FRAME_SIDE_TEXTURE_HEIGHT = 320,
        };

        enum eLAYOUT
        {
            TITLE_Y = 12,
            CONTENT_LEFT = 20,
            CONTENT_WIDTH = WINDOW_WIDTH - 2 * CONTENT_LEFT,
            CONTENT_TOP = 56,
            LINE_HEIGHT = 14,
            SCHEDULE_GAP = 4,
            OK_BUTTON_WIDTH = 108,
            OK_BUTTON_HEIGHT = 29,
            // Same row geometry as the changelog footer: above the bottom strip.
            OK_BUTTON_X = (WINDOW_WIDTH - OK_BUTTON_WIDTH) / 2,
            OK_BUTTON_Y = WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT - 32,
            MESSAGE_LINE_CHARS = maintenance_layout::kMessageLineChars,
            MESSAGE_LINES = maintenance_layout::kMaxMessageLines,
            // Baked close "X" in the header right cap (same geometry as the
            // changelog window's).
            CLOSE_X_FROM_RIGHT = 21,
            CLOSE_Y = 7,
            CLOSE_W = 13,
            CLOSE_H = 12,
        };

    public:
        // Public so the UI system can center the window on the 640x480 layout.
        static constexpr int kWindowWidth = WINDOW_WIDTH;
        static constexpr int kWindowHeight = WINDOW_HEIGHT;

        CNewUIMaintenanceWindow();
        virtual ~CNewUIMaintenanceWindow();

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

        // Parses a C2 F3 EF payload from the game server. Only an active
        // notice arms the auto-show; the client never opens the window for a
        // flag-0 packet (the server should not send one, this is the safety).
        void ReceiveNotice(const BYTE* buffer, int size);

        // True exactly once per login push: the WSclient glue shows the window
        // when this returns true after ReceiveNotice.
        bool ConsumeAutoShow();

    private:
        void LoadImages();
        void UnloadImages();
        void InitButtons();
        void RenderWindowFrame(float x, float y, float w, float h);

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_BtnOk;
        bool m_bAutoShowPending;
        maintenance_layout::ParsedNotice m_Notice;
    };
}

// Routed from WSclient.cpp when the server sends C2 F3 EF.
void ReceiveMaintenance(const BYTE* buffer, int size);
