#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "MUHelper/MuHelperData.h"

namespace SEASON3B
{
    class CNewUIAutoBattler : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_AB_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_AB_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_AB_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_AB_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_AB_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_AB_BTN = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY,
            IMAGE_AB_BTN_SMALL = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_SMALL,
            IMAGE_AB_BTN_MAP = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL,
            IMAGE_AB_TABLE_TOP_LEFT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_LEFT,
            IMAGE_AB_TABLE_TOP_RIGHT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_RIGHT,
            IMAGE_AB_TABLE_BOTTOM_LEFT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_LEFT,
            IMAGE_AB_TABLE_BOTTOM_RIGHT = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_RIGHT,
            IMAGE_AB_TABLE_TOP_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_TOP_PIXEL,
            IMAGE_AB_TABLE_BOTTOM_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_BOTTOM_PIXEL,
            IMAGE_AB_TABLE_LEFT_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_LEFT_PIXEL,
            IMAGE_AB_TABLE_RIGHT_PIXEL = CNewUIInventoryCtrl::IMAGE_ITEM_TABLE_RIGHT_PIXEL,
            IMAGE_AB_ITEMBOX = CNewUIInventoryCtrl::IMAGE_ITEM_SQUARE,
        };

        static constexpr int kMapSlots = 8;

        CNewUIAutoBattler();
        virtual ~CNewUIAutoBattler();

        static CNewUIAutoBattler* NewInstance();
        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();
        void SetPos(int x, int y);

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override { return 10.08f; }
        float GetKeyEventOrder() override { return 10.08f; }

        void OpeningProcess();
        void ClosingProcess();
        void ReceiveStatus(const BYTE* buffer);
        void ReceiveCatalog(const BYTE* buffer, int size);
        void TickSession();

        // After LoadWorld: queue this map's hunt mob BMDs and load 1/frame.
        // Never OpenMonsterModel from OpeningProcess (WER 0xc0000374).
        static void NotifyWorldLoaded(int world);
        static void TickPreviewPreload();
        static void QueueHuntPreviewModels(int huntIndex);

    private:
        void LoadImages();
        void SetBtnInfo();
        void InitMapButtons();
        void RefreshMapButtons();
        void RefreshActivateButton();
        void RenderFrame();
        void RenderTexts();
        void RenderButtons();
        void RenderGrid();
        void RenderTable(float x, float y, float width, float height, float titleHeight);
        bool BtnProcess();
        void SelectHunt(int huntIndex);
        void SendStatusRequest();
        void SendCatalogRequest();
        void SendStart();
        void SendStop();
        void TryStartHunt();
        void StopHunt(const char* szReason = "stop");
        void OnHelperAutoStop(const char* szReason);
        void ApplyHelperHunt();
        void RestoreHelperHunt();
        void ScanNearbyMonsters();
        bool InventoryIsFull() const;
        void ShowNotice(const wchar_t* text);
        int HuntCount() const;
        int MapPageCount() const;
        int SelectedSlot() const;
        const wchar_t* HuntMapName(int huntIndex) const;

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIRadioGroupButton m_MapButton;
        CNewUIButton m_StartButton;
        CNewUIButton m_PrevButton;
        CNewUIButton m_NextButton;

        MUHelper::ConfigData m_HelperBackup;
        int m_iHunt;
        int m_iMapPage;
        int m_iRemaining;
        int m_iQuota;
        bool m_bVip;
        bool m_bRunning;
        bool m_bSessionActive;
        bool m_bUiReady;
        bool m_bHelperOverlaid;
        DWORD m_dwLastTick;
        DWORD m_dwLastSync;
        bool m_bStartConfirmed;   // session start acknowledged (reply or fallback)
        DWORD m_dwStartSentTick;  // tick when SendStart was issued (0 = idle)
        int m_iDropScroll;
        int m_iMobScroll;
    };
}
