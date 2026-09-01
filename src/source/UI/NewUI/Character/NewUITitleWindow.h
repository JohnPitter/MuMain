#pragma once

#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    class CNewUITitleWindow : public CNewUIObj
    {
        enum eIMAGE_LIST
        {
            IMAGE_TITLE_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_TITLE_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_TITLE_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_TITLE_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_TITLE_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_TITLE_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
        };

        enum eWINDOW_SIZE
        {
            WINDOW_WIDTH = 190,
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
            CONTENT_TOP = 48,
            ROW_HEIGHT = 16,
            VISIBLE_ROWS = 20,
            EXIT_BUTTON_X = 13,
            EXIT_BUTTON_Y = 392,
            EXIT_BUTTON_WIDTH = 36,
            EXIT_BUTTON_HEIGHT = 29,
        };

    public:
        CNewUITitleWindow();
        virtual ~CNewUITitleWindow();

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

    private:
        void LoadImages();
        void UnloadImages();
        void InitButtons();
        void RenderBaseWindow();
        int HitRow() const;

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_BtnExit;
        int m_scrollOffset;
    };
}
