#pragma once

// "Rádio" config window — same native NewUI molde as the Novidades popup
// (msgbox_back tile + inventory frame strips + native widgets): a station
// combo box, a volume slider (0..100, same drag/wheel interaction as the
// Options window sliders) and a Ligar/Desligar button. Pure UI: all state
// lives in Audio::Radio (RadioPlayer) and GameConfig.

#include <vector>

#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Widgets/NewUIComboBox.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    class CNewUIRadioWindow : public CNewUIObj
    {
        enum eIMAGE_LIST
        {
            IMAGE_RADIO_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_RADIO_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_RADIO_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_RADIO_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_RADIO_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_RADIO_BTN_EXIT = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN,
            // Small native plate (64x29, 3 stacked frames) — the same one the
            // message boxes use for "Cancelar". The old 108x29 plate read as
            // a wide slab under the window (owner request: smaller, readable).
            IMAGE_RADIO_BTN_POWER = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_SMALL,
        };

        enum eWINDOW_SIZE
        {
            WINDOW_WIDTH = 320,
            WINDOW_HEIGHT = 250,
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
            STATION_LABEL_Y = 56,
            COMBO_Y = 72,
            // Options-window combo look: 16px rows, 5 visible rows in the open
            // list — identical to the Idioma/Fonte combos of NewUIOptionWindow
            // (same CNewUIComboBox widget; only the old row height/row count
            // differed). Width stays CONTENT_WIDTH because station names are
            // far longer than the options window's 148px labels.
            COMBO_HEIGHT = 16,
            COMBO_MAX_VISIBLE = 5,
            VOLUME_LABEL_Y = 104,
            VOLUME_TRACK_Y = 122,
            VOLUME_TRACK_WIDTH = 124,
            VOLUME_TRACK_HEIGHT = 14,
            VOLUME_HIT_PADDING = 8,
            NOW_PLAYING_Y = 148,
            NOW_PLAYING_LINES = 2,
            LINE_HEIGHT = 13,
            EXIT_BUTTON_X = 13,
            EXIT_BUTTON_WIDTH = 36,
            EXIT_BUTTON_HEIGHT = 29,
            // Footer buttons live INSIDE the bottom frame strip, next to the
            // bottom border (owner request: "mais perto do fundo"). 8px into
            // the 45px strip, 8px of clearance below the plates.
            EXIT_BUTTON_Y = WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT + 8,
            POWER_BUTTON_WIDTH = 64,
            POWER_BUTTON_HEIGHT = 29,
            POWER_BUTTON_X = WINDOW_WIDTH - 13 - POWER_BUTTON_WIDTH,
            POWER_BUTTON_Y = EXIT_BUTTON_Y,
            // Baked close "X" in the header right cap (same geometry as the
            // changelog window).
            CLOSE_X_FROM_RIGHT = 21,
            CLOSE_Y = 7,
            CLOSE_W = 13,
            CLOSE_H = 12,
        };

    public:
        static constexpr int kWindowWidth = WINDOW_WIDTH;
        static constexpr int kWindowHeight = WINDOW_HEIGHT;

        CNewUIRadioWindow();
        virtual ~CNewUIRadioWindow();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();
        void SetPos(int x, int y);

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;

        float GetLayerDepth() override;
        float GetKeyEventOrder() override;

    private:
        void LoadImages();
        void UnloadImages();
        void InitButtons();
        void InitStationCombo();
        void RenderWindowFrame(float x, float y, float w, float h);
        void RenderFooterButtons();
        void RenderVolumeSlider();
        bool HandleVolumeSlider();
        void ApplyPowerState();
        void RefreshPowerButton();

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_BtnExit;
        CNewUIButton m_BtnPower;
        CNewUIComboBox m_StationCombo;

        // Combo label storage: the combo consumes plain `const wchar_t* const*`,
        // so the wide strings and their pointer array both live here.
        std::vector<std::wstring> m_StationNames;
        std::vector<const wchar_t*> m_StationLabels;

        int m_iVolumeLevel = 0;
        int m_cachedStationCount = -1;   // rebuilt when InitializeRadio lands
        bool m_bSwallowClickHold = false;
    };
}
