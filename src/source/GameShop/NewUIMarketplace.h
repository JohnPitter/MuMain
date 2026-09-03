#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/Legacy/UIControls.h"
#include "GameShop/NewUIInGameShop.h"

namespace SEASON3B
{
    class CNewUIMarketplace : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_MP_EXIT_BTN = CNewUIInGameShop::IMAGE_IGS_EXIT_BTN,
            IMAGE_MP_BACK = CNewUIInGameShop::IMAGE_IGS_BACK,
            IMAGE_MP_CATEGORY_BTN = CNewUIInGameShop::IMAGE_IGS_CATEGORY_BTN,
            IMAGE_MP_CATEGORY_DECO_MIDDLE = CNewUIInGameShop::IMAGE_IGS_CATEGORY_DECO_MIDDLE,
            IMAGE_MP_CATEGORY_DECO_DOWN = CNewUIInGameShop::IMAGE_IGS_CATEGORY_DECO_DOWN,
            IMAGE_MP_LEFT_TAB = CNewUIInGameShop::IMAGE_IGS_LEFT_TAB,
            IMAGE_MP_RIGHT_TAB = CNewUIInGameShop::IMAGE_IGS_RIGHT_TAB,
            IMAGE_MP_ZONE_BTN = CNewUIInGameShop::IMAGE_IGS_ZONE_BTN,
            IMAGE_MP_ITEMGIFT_BTN = CNewUIInGameShop::IMAGE_IGS_ITEMGIFT_BTN,
            IMAGE_MP_CASHGIFT_BTN = CNewUIInGameShop::IMAGE_IGS_CASHGIFT_BTN,
            IMAGE_MP_REFRESH = CNewUIInGameShop::IMAGE_IGS_REFRESH_BTN,
            IMAGE_MP_VIEWDETAIL_BTN = CNewUIInGameShop::IMAGE_IGS_VIEWDETAIL_BTN,
            IMAGE_MP_ITEMBOX = CNewUIInGameShop::IMAGE_IGS_ITEMBOX_LOGO,
            IMAGE_MP_PAGE_LEFT = CNewUIInGameShop::IMAGE_IGS_PAGE_LEFT,
            IMAGE_MP_PAGE_RIGHT = CNewUIInGameShop::IMAGE_IGS_PAGE_RIGHT,
            IMAGE_MP_STORAGE_PAGE = CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE,
            IMAGE_MP_STORAGE_PAGE_LEFT = CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE_LEFT,
            IMAGE_MP_STORAGE_PAGE_RIGHT = CNewUIInGameShop::IMAGE_IGS_STORAGE_PAGE_RIGHT,
            IMAGE_MP_BANNER = CNewUIInGameShop::IMAGE_IGS_BANNER,
        };

        static constexpr int kGridCols = 3;
        static constexpr int kGridRows = 3;
        static constexpr int kGridCount = kGridCols * kGridRows;
        static constexpr int kInvRows = 9;
        static constexpr int kCategoryCount = 7;
        static constexpr int kTabCount = 3;
        static constexpr int kSortCount = 3;
        static constexpr int kFeaturedMax = 8;

        enum LISTBOX_TAB
        {
            LISTTAB_INVENTORY = 0,
            LISTTAB_CART = 1,
            LISTTAB_ITEMS = 2,
        };

        struct Listing
        {
            int Id;
            WORD ItemCode;
            BYTE Level;
            BYTE Excellent;
            BYTE Ancient;
            BYTE OptionLevel;
            BYTE OptionType;
            BYTE HasLuck;
            BYTE HasSkill;
            int Price;
            BYTE Quantity;
            BYTE Category;
            BYTE Status;
            BYTE FeatureHours;
            wchar_t Seller[12];
        };

        CNewUIMarketplace();
        virtual ~CNewUIMarketplace();

        static CNewUIMarketplace* NewInstance();
        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();
        void SetPos(int x, int y);

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override { return 10.09f; }
        float GetKeyEventOrder() override { return 10.09f; }

        void OpeningProcess();
        void ClosingProcess();
        void ReceiveList(const BYTE* buffer);
        void ReceiveResult(BYTE result);
        void RequestPage();
        void ConfirmAnnounce(const wchar_t* priceText);
        void ConfirmHighlight();
        void BuyCurrentOffer();

    private:
        void SetBtnInfo();
        void InitTabButtons();
        void InitSortButtons();
        void InitListBoxTabs();
        void InitCategoryButtons();
        void RenderFrame();
        void RenderTexts();
        void RenderButtons();
        void RenderListBox();
        void RenderGrid();
        void RenderListDialog();
        bool BtnProcess();
        bool DialogProcess();
        void CollectInventory();
        void RefreshListBox();
        void FillDemoListings();
        void ApplyVisibleListings();
        void AddToCart(const Listing& listing);
        void ShowOffer(int index);
        void OpenAnnounceDialog();
        void BuySelectedCart();
        void UpdateActionButton();
        BYTE SortPacketValue() const;
        void OpenRechargePage();
        void SendListRequest();
        void SendBuy(int listingId);
        void SendCreate();
        void SendCancel(int listingId);
        void SendFeature(int listingId);
        const wchar_t* CategoryName(int index) const;
        const wchar_t* SortName(int index) const;
        const wchar_t* TabName(int index) const;
        bool IsDemoListing(int listingId) const;
        bool IsCartTab();
        bool IsInvTab();
        bool IsItemsTab();
        void AddToPurchased(const Listing& listing);
        void ClaimPurchased();
        void CollectFromCtrl(CNewUIInventoryCtrl* pCtrl);
        void AddFeatured(const Listing& listing);
        void HighlightListing(int index);
        void AdvanceFeatured();
        void ShowListing(const Listing& listing, bool canBuy);
        void DemoCreateListing(int price, BYTE qty);
        bool CanRenderItem(WORD itemCode) const;
        void SyncSlotButtons();
        void RenderFeaturedPanel();

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        CNewUIRadioGroupButton m_TabButton;
        CNewUIRadioGroupButton m_SortButton;
        CNewUIRadioGroupButton m_ListBoxTabButton;
        CNewUIRadioGroupButton m_CategoryButton;
        CNewUIButton m_CloseButton;
        CNewUIButton m_ListItemButton;
        CNewUIButton m_BuyButton[kGridCount];
        CNewUIButton m_ViewButton[kGridCount];
        CNewUIButton m_PrevButton;
        CNewUIButton m_NextButton;
        CNewUIButton m_InvPrevButton;
        CNewUIButton m_InvNextButton;
        CNewUIButton m_DialogCancel;
        CNewUIButton m_DialogOk;
        CNewUIButton m_CashGiftButton;
        CNewUIButton m_CashChargeButton;
        CNewUIButton m_RefreshButton;
        CNewUIButton m_UseButton;
        CUIInGameShopListBox m_ItemListBox;

        int m_iTab;
        int m_iCategory;
        int m_iSort;
        int m_iPage;
        int m_iTotalPages;
        int m_iListingCount;
        int m_iSelectedListing;
        bool m_bDemoMode;
        Listing m_Listings[kGridCount];
        Listing m_DemoAll[24];
        Listing m_Cart[kGridCount];
        Listing m_Purchased[24];
        Listing m_Featured[kFeaturedMax];
        int m_iDemoCount;
        int m_iCartCount;
        int m_iPurchasedCount;
        int m_iFeaturedCount;
        int m_iFeaturedSlide;
        DWORD m_dwFeaturedTick;

        int m_iInvPage;
        int m_iInvCount;
        int m_iSelectedInv;
        int m_InvSlots[64];
        WORD m_InvTypes[64];
        BYTE m_InvLevels[64];
        BYTE m_InvQty[64];
        BYTE m_InvExcellent[64];
        BYTE m_InvAncient[64];
        BYTE m_InvOption[64];
        BYTE m_InvOptionType[64];
        BYTE m_InvLuck[64];
        BYTE m_InvSkill[64];

        bool m_bShowDialog;
        bool m_bUiReady;
        int m_iDialogCategory;
        int m_iDialogDays;
        wchar_t m_szPrice[16];
        wchar_t m_szQty[8];
        int m_iPriceFocus;
        BYTE m_iCurrency;
    };
}
