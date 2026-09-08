#pragma once

#include "UI/NewUI/Changelog/ChangelogLayout.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    // "Novidades" popup. The server pushes the three newest changelog entries
    // once per login, a few seconds after the player enters the world (never
    // on map change); this window renders them as a centered popup with native
    // NewUI components. Its "Mostrar tudo" button asks the server for the full
    // list (C1 F3 ED) and expands into a mouse-wheel-scrollable view of every
    // entry (server-capped at 30).
    class CNewUIChangelogWindow : public CNewUIObj
    {
        enum eIMAGE_LIST
        {
            IMAGE_CHANGELOG_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_CHANGELOG_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_CHANGELOG_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_CHANGELOG_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_CHANGELOG_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_CHANGELOG_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
            // Shared empty button texture for labeled wide buttons — the same
            // native plate the Gold Bowman / Lucky Coin exchange windows use
            // (message-box manager owns the bitmap; frames are 108x29 stacked
            // vertically: up / hover / press).
            IMAGE_CHANGELOG_BTN_SHOW_ALL = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY,
        };

        enum eWINDOW_SIZE
        {
            WINDOW_WIDTH = 320,
            WINDOW_HEIGHT = 380,
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
            CONTENT_TOP = 56,
            CONTENT_BOTTOM_MARGIN = 6,
            LINE_HEIGHT = 14,
            BLOCK_GAP = 6,
            DESC_LINES_PER_ENTRY = changelog_layout::kDescLinesPerEntry,
            BLOCK_HEIGHT = (1 + DESC_LINES_PER_ENTRY) * LINE_HEIGHT + BLOCK_GAP,
            DATE_TEXT_WIDTH = 44,
            EXIT_BUTTON_X = 13,
            EXIT_BUTTON_WIDTH = 36,
            EXIT_BUTTON_HEIGHT = 29,
            EXIT_BUTTON_Y = WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT - 32,
            // Native plate of newui_btn_empty.tga (108x87, 3 frames of 108x29):
            // the button must match the texture frame exactly — CNewUIButton
            // samples the texture at the button's own pixel size, so a wider
            // button than the frame clamps the edge column into a smear (the
            // raw-looking footer of the first build). Same row as the exit X.
            SHOW_ALL_BUTTON_WIDTH = 108,
            SHOW_ALL_BUTTON_HEIGHT = 29,
            SHOW_ALL_BUTTON_X = WINDOW_WIDTH - 13 - SHOW_ALL_BUTTON_WIDTH,
            SHOW_ALL_BUTTON_Y = EXIT_BUTTON_Y,
            SCROLL_HINT_Y_OFFSET = 14,
        };

    public:
        // Public so the UI system can center the window on the 640x480 layout.
        static constexpr int kWindowWidth = WINDOW_WIDTH;
        static constexpr int kWindowHeight = WINDOW_HEIGHT;

        // Mirrors InGameChangelogService.cs.
        static constexpr int kPreviewEntries = 3;

        CNewUIChangelogWindow();
        virtual ~CNewUIChangelogWindow();

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

        // Parses a C2 F3 ED payload from the game server and updates the list.
        // The first (login) push leaves the popup in preview mode; the reply to
        // a "Mostrar tudo" request only replaces the entries of the expanded view.
        void ReceiveEntries(const BYTE* buffer, int size);

        // True exactly once per login push: the WSclient glue shows the popup
        // when this returns true after ReceiveEntries. Replies to a
        // "Mostrar tudo" request never set it, so closing the popup while the
        // reply is in flight cannot reopen it.
        bool ConsumeAutoShow();

        void RequestFullList();

    private:
        void LoadImages();
        void UnloadImages();
        void InitButtons();
        void RenderWindowFrame(float x, float y, float w, float h);
        void RenderFooterButtons();
        int VisibleEntryCount() const;
        int VisibleBlocks() const;

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_BtnExit;
        CNewUIButton m_BtnShowAll;
        bool m_bExpanded;
        bool m_bAwaitingFullList;
        bool m_bAutoShowPending;
        int m_iEntryCount;
        int m_iScrollBlock;
        changelog_layout::ParsedEntry m_Entries[changelog_layout::kMaxEntries];
    };
}

// Routed from WSclient.cpp when the server sends C2 F3 ED.
void ReceiveChangelog(const BYTE* buffer, int size);
