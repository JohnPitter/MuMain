#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"

namespace SEASON3B
{
    // Side panel shown only while the Auto Battler session is active.
    // Tracks elapsed time, EXP gain (level-up safe), and net Zen (profit)
    // with hourly rates. Built exclusively from native client widgets and
    // shared textures (no new texture ids, no raw drawn frames).
    class CNewUIHuntAnalyzer : public CNewUIObj
    {
    public:
        // Reuses texture ids already owned by the inventory/messagebox family,
        // loaded with the exact same image files (same id + same file = same
        // shared texture, no ownership change, no new id to collide).
        enum IMAGE_LIST
        {
            IMAGE_HA_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_HA_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_HA_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_HA_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_HA_BTN = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY,
            IMAGE_HA_TABLE_TOP_LEFT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_LEFT,
            IMAGE_HA_TABLE_TOP_RIGHT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_RIGHT,
            IMAGE_HA_TABLE_BOTTOM_LEFT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_LEFT,
            IMAGE_HA_TABLE_BOTTOM_RIGHT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_RIGHT,
            IMAGE_HA_TABLE_TOP_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_PIXEL,
            IMAGE_HA_TABLE_BOTTOM_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_PIXEL,
            IMAGE_HA_TABLE_LEFT_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_LEFT_PIXEL,
            IMAGE_HA_TABLE_RIGHT_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_RIGHT_PIXEL,
            IMAGE_HA_PROGRESS_BG = CNewUIMessageBoxMng::IMAGE_MSGBOX_PROGRESS_BG,
            IMAGE_HA_PROGRESS_BAR = CNewUIMessageBoxMng::IMAGE_MSGBOX_PROGRESS_BAR,
        };

        CNewUIHuntAnalyzer();
        virtual ~CNewUIHuntAnalyzer();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        // Session lifecycle driven by the Auto Battler state machine.
        // Idempotent: repeated "true" while already running keeps the current
        // session (reopening the Auto Battle window never resets metrics).
        // szReason is logged ("HUNTANALYZER:") for lifecycle diagnostics.
        void NotifySession(bool bActive, const char* szReason = "status");

        // Called by CNewUISystem when the interface is hidden globally.
        void ClosingProcess() {}

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override { return 10.07f; }
        float GetKeyEventOrder() override { return 10.07f; }

    private:
        void LoadImages();
        void SetBtnInfo();
        void StartSession();
        void StopSession();
        void SampleOnce();
        void RenderFrame();
        void RenderTexts();
        void RenderPanel(float x, float y, float width, float height, const wchar_t* title);
        bool BtnProcess();

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIButton m_VipButton;

        bool m_bSessionActive;
        bool m_bUserHidden;      // closed via X; re-armed on next session start
        bool m_bShownLast;       // last visibility driven by Update() (transition log)
        bool m_bImagesLoaded;
        bool m_bBaselinesReady;
        bool m_bLastMasterExp;

        // Time: monotonic accumulation, independent of the system clock.
        DWORD m_dwStartTick;     // tick at session start (confirmed)
        DWORD m_dwLastSample;    // last 1 Hz sample tick
        int m_iElapsedSeconds;   // accumulated seconds (wrap-safe via DWORD delta)

        // EXP: level-up safe accumulation.
        uint64_t m_ullLastExp;
        uint64_t m_ullLastNextExp;
        uint64_t m_ullTotalExp;
        WORD m_wLastLevel;
        short m_sLastMasterLevel;

        // Zen: net session profit from deltas (may be negative while hunting).
        DWORD m_dwLastZen;
        int64_t m_llTotalProfit;
        int m_iBaselineHeroKey;
    };
}
