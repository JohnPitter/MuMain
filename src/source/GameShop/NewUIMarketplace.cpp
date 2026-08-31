#include "stdafx.h"
#include "I18N/All.h"
#include "UI/NewUI/NewUISystem.h"
#include "GameShop/NewUIMarketplace.h"
#include "UI/NewUI/NewUICommon.h"
#include "GameShop/MsgBoxIGSCommon.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "Engine/Object/ZzzInventory.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Audio/DSPlaySound.h"
#include "Camera/CameraProjection.h"
#include "Camera/CameraState.h"
#include "Render/Core/GlobalUBO.h"
#include "Render/Core/RenderConfig.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Core/Input/KeyState.h"
#include "Dotnet/Connection.h"
#include "App/Platform/Windows/iexplorer.h"

#include <algorithm>

using namespace SEASON3B;

namespace
{
    constexpr int kBackW = 640;
    constexpr int kBackH = 429;
    constexpr int kCatX = 13;
    constexpr int kCatY = 31;
    constexpr int kCatW = 73;
    constexpr int kCatH = 27;
    constexpr int kCatGap = 6;
    constexpr int kZoneX = 95;
    constexpr int kZoneY = 0;
    constexpr int kZoneW = 76;
    constexpr int kZoneH = 23;
    constexpr int kViewX = 162;
    constexpr int kViewY = 126;
    constexpr int kViewW = 52;
    constexpr int kViewH = 26;
    constexpr int kPresentX = 108;
    constexpr int kSlotDX = 122;
    constexpr int kSlotDY = 121;
    constexpr int kBoxX = 128;
    constexpr int kBoxY = 52;
    constexpr int kBoxSize = 57;
    constexpr int kNameX = 105;
    constexpr int kNameY = 40;
    constexpr int kNameW = 104;
    constexpr int kPriceY = 113;
    constexpr int kItem3DX = 102;
    constexpr int kItem3DY = 51;
    constexpr int kItem3DW = 108;
    constexpr int kItem3DH = 58;
    constexpr int kCharNameX = 498;
    constexpr int kCharNameW = 122;
    constexpr int kCashX = 498;
    constexpr int kCashY = 50;
    constexpr int kCashW = 130;
    constexpr int kIconY = 94;
    constexpr int kIconBtnW = 25;
    constexpr int kIconGap = 8;
    constexpr int kCashBtnX = kCashX + (kCashW - (kIconBtnW * 2 + kIconGap)) / 2;
    constexpr int kRefreshBtnX = kCashBtnX + kIconBtnW + kIconGap;
    constexpr int kCatListY = 172;
    constexpr int kCatListGap = 2;
    constexpr int kFloreioW = 47;
    constexpr int kFloreioH = 40;
    constexpr int kBannerX = 482;
    constexpr int kBannerY = 133;
    constexpr int kBannerW = 153;
    constexpr int kBannerH = 63;
    constexpr int kFeatX = 498;
    constexpr int kFeatY = 138;
    constexpr int kFeatW = 120;
    constexpr int kFeatH = 46;
    // Match NewUIInGameShop storage chrome (baked into IMAGE_IGS_BACK):
    // tabs 486/208, header name 492×96 CENTER, duration/qty 592×34 CENTER,
    // listbox CUIInGameShopListBox(490,360) cols name 0..98 LEFT / period 102..135 RIGHT.
    constexpr int kListTabX = 486;
    constexpr int kListTabY = 208;
    constexpr int kListTabW = 49;
    constexpr int kListTabH = 20;
    constexpr int kListTabGap = -2;
    constexpr int kListX = 490;
    constexpr int kListY = 360;
    constexpr int kStorageHeaderY = 233;
    constexpr int kStorageHeaderNameX = 492;
    constexpr int kStorageHeaderNameW = 96;
    constexpr int kStorageHeaderQtyX = 592;
    constexpr int kStorageHeaderQtyW = 34;
    constexpr int kFeatureCostW = 500;
    constexpr int kFeatureHours = 24;
    constexpr int kAnnounceX = 326;
    constexpr int kDlgW = 320;
    constexpr int kDlgH = 210;
    constexpr BYTE kGroup = 0xD3;

    constexpr wchar_t kRechargeUrl[] = L"http://127.0.0.1:3000/cashpoints.html?login=%ls";

    static float s_PreProj[16];
    static float s_PreView[16];
}

extern Connection* SocketClient;
extern wchar_t LogInID[MAX_USERNAME_SIZE + 1];
extern CameraState g_Camera;

namespace
{
    int s_PendingHighlightId = 0;
    bool s_HasPendingHighlight = false;

    class CMarketplaceAnnounceLayout : public TMsgBoxLayout<CNewUITextInputMsgBox>
    {
    public:
        bool SetLayout()
        {
            CNewUITextInputMsgBox* pMsgBox = GetMsgBox();
            if (pMsgBox == nullptr)
                return false;
            if (!pMsgBox->Create(MSGBOX_COMMON_TYPE_OKCANCEL, INPUTBOX_TYPE_NUMBER, INPUTBOX_WIDTH, INPUTBOX_HEIGHT, INPUTBOX_TEXTLIMIT))
                return false;
            pMsgBox->SetInputBoxOption(UIOPTION_NUMBERONLY | UIOPTION_PAINTBACK);
            pMsgBox->AddCallbackFunc(OkBtnDown, MSGBOX_EVENT_USER_COMMON_OK);
            pMsgBox->AddCallbackFunc(OkBtnDown, MSGBOX_EVENT_PRESSKEY_RETURN);
            pMsgBox->AddCallbackFunc(CancelBtnDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
            pMsgBox->AddCallbackFunc(CancelBtnDown, MSGBOX_EVENT_PRESSKEY_ESC);
            return true;
        }

        static CALLBACK_RESULT OkBtnDown(CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
        {
            auto* pMsgBox = dynamic_cast<CNewUITextInputMsgBox*>(pOwner);
            if (pMsgBox == nullptr || g_pMarketplace == nullptr)
                return CALLBACK_CONTINUE;

            wchar_t price[16] = {};
            pMsgBox->GetInputBoxText(price);
            if (price[0] == 0 || _wtoi(price) <= 0)
                return CALLBACK_CONTINUE;

            g_pMarketplace->ConfirmAnnounce(price);
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }

        static CALLBACK_RESULT CancelBtnDown(CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
        {
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }
    };

    class CMarketplaceHighlightLayout : public TMsgBoxLayout<CNewUICommonMessageBox>
    {
    public:
        bool SetLayout()
        {
            CNewUICommonMessageBox* pMsgBox = GetMsgBox();
            if (pMsgBox == nullptr)
                return false;
            if (!pMsgBox->Create(MSGBOX_COMMON_TYPE_OKCANCEL))
                return false;
            pMsgBox->AddCallbackFunc(OkBtnDown, MSGBOX_EVENT_USER_COMMON_OK);
            pMsgBox->AddCallbackFunc(OkBtnDown, MSGBOX_EVENT_PRESSKEY_RETURN);
            pMsgBox->AddCallbackFunc(CancelBtnDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
            pMsgBox->AddCallbackFunc(CancelBtnDown, MSGBOX_EVENT_PRESSKEY_ESC);
            return true;
        }

        static CALLBACK_RESULT OkBtnDown(CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
        {
            if (g_pMarketplace != nullptr)
                g_pMarketplace->ConfirmHighlight();
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }

        static CALLBACK_RESULT CancelBtnDown(CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
        {
            s_HasPendingHighlight = false;
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }
    };

    class CMarketplaceOfferLayout : public TMsgBoxLayout<CNewUI3DItemCommonMsgBox>
    {
    public:
        bool SetLayout()
        {
            CNewUI3DItemCommonMsgBox* pMsgBox = GetMsgBox();
            if (pMsgBox == nullptr)
                return false;
            return pMsgBox->Create(MSGBOX_COMMON_TYPE_OK);
        }
    };

    CNewUIMarketplace::Listing s_OfferListing;
    bool s_OfferCanBuy = false;

    void ShowMarketplaceNotice(const wchar_t* title, const wchar_t* body)
    {
        CMsgBoxIGSCommon* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        if (pMsgBox != nullptr)
            pMsgBox->Initialize(title, body);
    }

    void RenderClippedItem3D(float x, float y, float w, float h, int type, int level, int excellent, int ancient, bool pickUp)
    {
        const int px = static_cast<int>(ConvertPosX(x));
        const int py = static_cast<int>(ConvertPosY(y));
        const int pw = static_cast<int>(ConvertX(w));
        const int ph = static_cast<int>(ConvertY(h));
        if (pw < 1 || ph < 1)
            return;

        EnableScissorTest();
        SetScissor(px, static_cast<int>(WindowHeight) - py - ph, pw, ph);
        RenderItem3D(x, y, w, h, type, level, excellent, ancient, pickUp);
        DisableScissorTest();
    }

    void RenderMarketplaceItem(float x, float y, float w, float h, const CNewUIMarketplace::Listing& listing)
    {
        if (listing.ItemCode >= ITEM_ARMOR && listing.ItemCode < ITEM_ARMOR + MAX_ITEM_INDEX)
        {
            y += h * 0.08f;
            h *= 0.80f;
        }
        RenderClippedItem3D(x, y, w, h, listing.ItemCode, listing.Level, listing.Excellent, listing.Ancient, true);
    }

    class CMarketplaceFeaturedLayout : public TMsgBoxLayout<CNewUI3DItemCommonMsgBox>
    {
    public:
        bool SetLayout()
        {
            CNewUI3DItemCommonMsgBox* pMsgBox = GetMsgBox();
            if (pMsgBox == nullptr)
                return false;
            if (!pMsgBox->Create(MSGBOX_COMMON_TYPE_OKCANCEL))
                return false;
            pMsgBox->SetCancelAsTextButton(I18N::Game::Buy1124);
            pMsgBox->AddCallbackFunc(BuyBtnDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
            return true;
        }

        static CALLBACK_RESULT BuyBtnDown(CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
        {
            if (g_pMarketplace != nullptr)
                g_pMarketplace->BuyCurrentOffer();
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }
    };

    ITEM MakeListingItem(const CNewUIMarketplace::Listing& listing)
    {
        ITEM item = {};
        item.Type = listing.ItemCode;
        item.Level = listing.Level;
        item.ExcellentFlags = listing.Excellent;
        item.AncientDiscriminator = listing.Ancient;
        item.OptionLevel = listing.OptionLevel;
        item.OptionType = listing.OptionType;
        item.HasLuck = listing.HasLuck != 0;
        item.HasSkill = listing.HasSkill != 0;
        item.Durability = listing.Quantity > 0 ? listing.Quantity : 255;
        SetItemAttributes(&item);
        return item;
    }

    void AppendOfferStats(CNewUI3DItemCommonMsgBox* pMsgBox, const ITEM& item)
    {
        if (pMsgBox == nullptr)
            return;

        const DWORD blue = RGBA(100, 150, 255, 255);
        wchar_t text[128] = {};
        int mana = 0;
        for (int i = 0; i < item.SpecialNum && i < MAX_ITEM_SPECIAL; ++i)
        {
            text[0] = 0;
            GetSpecialOptionText(item.Type, text, item.Special[i], item.SpecialValue[i], mana);
            if (text[0] != 0)
                pMsgBox->AddMsg(text, blue);
            if (item.Special[i] == AT_LUCK)
            {
                text[0] = 0;
                mu_swprintf(text, I18N::Game::LuckCriticalDamageRate5, 5);
                if (text[0] != 0)
                    pMsgBox->AddMsg(text, blue);
            }
        }
    }

    bool TryDebitLocalW(int price)
    {
        if (price <= 0)
            return true;
        const double cash = g_InGameShopSystem->GetCashCreditCard();
        if (cash < static_cast<double>(price))
            return false;
        g_InGameShopSystem->SetCashCreditCard(cash - static_cast<double>(price));
        return true;
    }

    void ShowNotEnoughW()
    {
        CMsgBoxIGSCommon* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        if (pMsgBox)
            pMsgBox->Initialize(I18N::Game::Error, I18N::Game::MarketplaceNotEnoughW);
    }
}

CNewUIMarketplace* CNewUIMarketplace::NewInstance()
{
    return new CNewUIMarketplace();
}

CNewUIMarketplace::CNewUIMarketplace()
{
    m_pNewUIMng = nullptr;
    m_Pos.x = m_Pos.y = 0;
    m_iTab = 0;
    m_iCategory = 0;
    m_iSort = 0;
    m_iPage = 1;
    m_iTotalPages = 1;
    m_iListingCount = 0;
    m_iSelectedListing = 0;
    m_bDemoMode = true;
    m_iDemoCount = 0;
    m_iCartCount = 0;
    m_iPurchasedCount = 0;
    m_iFeaturedCount = 0;
    m_iFeaturedSlide = 0;
    m_dwFeaturedTick = 0;
    m_iInvPage = 0;
    m_iInvCount = 0;
    m_iSelectedInv = -1;
    m_bShowDialog = false;
    m_bUiReady = false;
    m_iDialogCategory = 0;
    m_iDialogDays = 7;
    m_szPrice[0] = 0;
    m_szQty[0] = L'1';
    m_szQty[1] = 0;
    m_iPriceFocus = 0;
    memset(m_Listings, 0, sizeof(m_Listings));
    memset(m_DemoAll, 0, sizeof(m_DemoAll));
    memset(m_Cart, 0, sizeof(m_Cart));
    memset(m_Purchased, 0, sizeof(m_Purchased));
    memset(m_Featured, 0, sizeof(m_Featured));
    memset(m_InvSlots, 0, sizeof(m_InvSlots));
    memset(m_InvTypes, 0, sizeof(m_InvTypes));
    memset(m_InvLevels, 0, sizeof(m_InvLevels));
    memset(m_InvQty, 0, sizeof(m_InvQty));
    memset(m_InvExcellent, 0, sizeof(m_InvExcellent));
    memset(m_InvAncient, 0, sizeof(m_InvAncient));
    memset(m_InvOption, 0, sizeof(m_InvOption));
    memset(m_InvOptionType, 0, sizeof(m_InvOptionType));
    memset(m_InvLuck, 0, sizeof(m_InvLuck));
    memset(m_InvSkill, 0, sizeof(m_InvSkill));
}

CNewUIMarketplace::~CNewUIMarketplace()
{
    Release();
}

bool CNewUIMarketplace::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MARKETPLACE, this);
    SetPos(x, y);
    Show(false);
    Enable(false);
    g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: Create done");
    return true;
}

void CNewUIMarketplace::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CNewUIMarketplace::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
}

void CNewUIMarketplace::SetBtnInfo()
{
    m_CloseButton.ChangeButtonImgState(true, IMAGE_MP_EXIT_BTN, false);
    m_CloseButton.ChangeButtonInfo(m_Pos.x + 484, m_Pos.y + 392, 36, 29);
    m_CloseButton.ChangeToolTipText(&I18N::Game::Close388, true);

    InitListBoxTabs();
    InitTabButtons();
    InitCategoryButtons();
    InitSortButtons();

    m_ListItemButton.ChangeButtonInfo(-200, -200, 0, 0);

    for (int i = 0; i < kGridCount; ++i)
    {
        const int slotX = (i % kGridCols) * kSlotDX;
        const int slotY = (i / kGridCols) * kSlotDY;
        m_ViewButton[i].ChangeButtonImgState(true, IMAGE_MP_VIEWDETAIL_BTN, true, false, true);
        m_ViewButton[i].ChangeButtonInfo(m_Pos.x + kPresentX + slotX, m_Pos.y + kViewY + slotY, kViewW, kViewH);
        m_ViewButton[i].MoveTextPos(0, -1);
        m_ViewButton[i].ChangeText(&I18N::Game::MarketplaceViewOffer);
        m_BuyButton[i].ChangeButtonImgState(true, IMAGE_MP_VIEWDETAIL_BTN, true, false, true);
        m_BuyButton[i].ChangeButtonInfo(m_Pos.x + kViewX + slotX, m_Pos.y + kViewY + slotY, kViewW, kViewH);
        m_BuyButton[i].MoveTextPos(0, -1);
        m_BuyButton[i].ChangeText(&I18N::Game::MarketplaceCartTab);
    }

    m_CashChargeButton.ChangeButtonImgState(true, IMAGE_MP_CASHGIFT_BTN, true);
    m_CashChargeButton.ChangeButtonInfo(m_Pos.x + kCashBtnX, m_Pos.y + kIconY, kIconBtnW, kIconBtnW);
    m_CashChargeButton.ChangeToolTipText(&I18N::Game::RechargeWCoin);
    m_RefreshButton.ChangeButtonImgState(true, IMAGE_MP_REFRESH, true);
    m_RefreshButton.ChangeButtonInfo(m_Pos.x + kRefreshBtnX, m_Pos.y + kIconY, kIconBtnW, kIconBtnW);
    m_RefreshButton.ChangeToolTipText(&I18N::Game::UpdateInformation);

    m_UseButton.ChangeButtonImgState(true, IMAGE_MP_VIEWDETAIL_BTN, true, false, true);
    m_UseButton.ChangeButtonInfo(m_Pos.x + 572, m_Pos.y + 396, kViewW, kViewH);
    m_UseButton.MoveTextPos(0, -1);
    m_UseButton.ChangeText(&I18N::Game::Buy1124);

    m_PrevButton.ChangeButtonImgState(true, IMAGE_MP_PAGE_LEFT, true);
    m_PrevButton.ChangeButtonInfo(m_Pos.x + 231, m_Pos.y + 397, 20, 23);
    m_NextButton.ChangeButtonImgState(true, IMAGE_MP_PAGE_RIGHT, true);
    m_NextButton.ChangeButtonInfo(m_Pos.x + 307, m_Pos.y + 397, 20, 23);

    m_InvPrevButton.ChangeButtonImgState(true, IMAGE_MP_STORAGE_PAGE_LEFT, true);
    m_InvPrevButton.ChangeButtonInfo(m_Pos.x + 500, m_Pos.y + 369, 20, 22);
    m_InvNextButton.ChangeButtonImgState(true, IMAGE_MP_STORAGE_PAGE_RIGHT, true);
    m_InvNextButton.ChangeButtonInfo(m_Pos.x + 596, m_Pos.y + 369, 20, 22);

    m_DialogCancel.ChangeButtonImgState(true, IMAGE_MP_VIEWDETAIL_BTN, true, false, true);
    m_DialogCancel.ChangeButtonInfo(0, 0, kViewW, kViewH);
    m_DialogCancel.ChangeText(&I18N::Game::InGameShopGiftCancel);
    m_DialogOk.ChangeButtonImgState(true, IMAGE_MP_VIEWDETAIL_BTN, true, false, true);
    m_DialogOk.ChangeButtonInfo(0, 0, kViewW, kViewH);
    m_DialogOk.ChangeText(&I18N::Game::MarketplaceAnnounce);

    m_ItemListBox.SetPosition(m_Pos.x + kListX, m_Pos.y + kListY);
    m_ItemListBox.SetNumRenderLine(9);
    SyncSlotButtons();
}

void CNewUIMarketplace::InitListBoxTabs()
{
    m_ListBoxTabButton.UnRegisterRadioButton();
    m_ListBoxTabButton.CreateRadioGroup(3, IMAGE_MP_LEFT_TAB);
    m_ListBoxTabButton.ChangeRadioButtonInfo(true, m_Pos.x + kListTabX, m_Pos.y + kListTabY,
        kListTabW, kListTabH, kListTabGap);
    m_ListBoxTabButton.ChangeButtonState(SEASON3B::BUTTON_STATE_DOWN, 0);
    m_ListBoxTabButton.ChangeButtonState(LISTTAB_ITEMS, IMAGE_MP_RIGHT_TAB, SEASON3B::BUTTON_STATE_UP, 0);
    m_ListBoxTabButton.ChangeButtonState(LISTTAB_ITEMS, IMAGE_MP_RIGHT_TAB, SEASON3B::BUTTON_STATE_DOWN, 0);
    m_ListBoxTabButton.SetFont(g_hFont);

    std::list<std::wstring> texts;
    texts.push_back(I18N::Game::MarketplaceInvTab);
    std::wstring cartTab = I18N::Game::MarketplaceCartTab;
    if (cartTab.size() > 5)
        cartTab = cartTab.substr(0, 4) + L".";
    texts.push_back(cartTab);
    texts.push_back(I18N::Game::MarketplaceItemsTab);
    m_ListBoxTabButton.ChangeRadioText(texts);
    m_ListBoxTabButton.ChangeFrame(LISTTAB_INVENTORY);
}

void CNewUIMarketplace::InitTabButtons()
{
    m_TabButton.UnRegisterRadioButton();
    m_TabButton.CreateRadioGroup(kTabCount, IMAGE_MP_CATEGORY_BTN, true);
    m_TabButton.ChangeRadioButtonInfo(false, m_Pos.x + kCatX, m_Pos.y + kCatY, kCatW, kCatH, kCatGap);
    m_TabButton.ChangeButtonState(SEASON3B::BUTTON_STATE_DOWN, 2);
    m_TabButton.SetFont(g_hFontBold);

    std::list<std::wstring> texts;
    texts.push_back(I18N::Game::MarketplaceBuyTab);
    texts.push_back(I18N::Game::MarketplaceMySalesTab);
    texts.push_back(I18N::Game::MarketplaceHistoryTab);
    m_TabButton.ChangeRadioText(texts);
    m_TabButton.ChangeFrame(m_iTab);
}

void CNewUIMarketplace::InitCategoryButtons()
{
    m_CategoryButton.UnRegisterRadioButton();
    m_CategoryButton.CreateRadioGroup(kCategoryCount, IMAGE_MP_CATEGORY_BTN, true);
    m_CategoryButton.ChangeRadioButtonInfo(false, m_Pos.x + kCatX, m_Pos.y + kCatListY,
        kCatW, kCatH, kCatListGap);
    m_CategoryButton.ChangeButtonState(SEASON3B::BUTTON_STATE_DOWN, 2);
    m_CategoryButton.SetFont(g_hFontBold);

    std::list<std::wstring> texts;
    for (int i = 0; i < kCategoryCount; ++i)
        texts.push_back(CategoryName(i));
    m_CategoryButton.ChangeRadioText(texts);
    m_CategoryButton.ChangeFrame(m_iCategory);
}

void CNewUIMarketplace::InitSortButtons()
{
    m_SortButton.UnRegisterRadioButton();
    m_SortButton.CreateRadioGroup(kSortCount, IMAGE_MP_ZONE_BTN);
    m_SortButton.ChangeRadioButtonInfo(true, m_Pos.x + kZoneX, m_Pos.y + kZoneY, kZoneW, kZoneH, 1);
    m_SortButton.SetFont(g_hFontBold);

    std::list<std::wstring> texts;
    texts.push_back(I18N::Game::MarketplaceNewest);
    texts.push_back(I18N::Game::MarketplaceLowestPrice);
    texts.push_back(I18N::Game::MarketplaceHighestPrice);
    m_SortButton.ChangeRadioText(texts);
    m_SortButton.ChangeFrame(m_iSort);
}

void CNewUIMarketplace::OpeningProcess()
{
    g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: OpeningProcess enter");
    PlayBuffer(SOUND_CLICK01);
    m_bShowDialog = false;
    g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: SetBtnInfo");
    SetBtnInfo();
    g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: FillDemo");
    FillDemoListings();
    g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: CollectInv");
    CollectInventory();
    ApplyVisibleListings();
    m_bUiReady = true;
    Enable(true);
    SendListRequest();
    if (SocketClient != nullptr && SocketClient->ToGameServer() != nullptr)
        SocketClient->ToGameServer()->SendCashShopPointInfoRequest();
    g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: OpeningProcess ok listings=%d inv=%d", m_iListingCount, m_iInvCount);
}

void CNewUIMarketplace::ClosingProcess()
{
    PlayBuffer(SOUND_CLICK01);
    m_bShowDialog = false;
}

void CNewUIMarketplace::OpenRechargePage()
{
    wchar_t url[288];
    mu_swprintf(url, kRechargeUrl, LogInID[0] ? LogInID : L"");
    leaf::OpenExplorer(url);

    CMsgBoxIGSCommon* pMsgBox = nullptr;
    CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
    pMsgBox->Initialize(I18N::Game::MarketplaceTitle, I18N::Game::MarketplaceRechargeHint);
}

void CNewUIMarketplace::SendListRequest()
{
    if (SocketClient == nullptr)
        return;

    BYTE packet[8] = { 0xC1, 8, kGroup, 0x00, static_cast<BYTE>(m_iPage), static_cast<BYTE>(m_iCategory), static_cast<BYTE>(m_iTab), SortPacketValue() };
    SocketClient->Send(packet, 8);
}

void CNewUIMarketplace::SendBuy(int listingId)
{
    if (IsDemoListing(listingId))
    {
        CMsgBoxIGSCommon* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        pMsgBox->Initialize(I18N::Game::MarketplaceTitle, I18N::Game::MarketplaceDemoNotice);
        return;
    }

    if (SocketClient == nullptr)
        return;

    BYTE packet[8] = { 0xC1, 8, kGroup, 0x01 };
    memcpy(packet + 4, &listingId, 4);
    SocketClient->Send(packet, 8);
}

void CNewUIMarketplace::SendCancel(int listingId)
{
    if (IsDemoListing(listingId) || SocketClient == nullptr)
        return;

    BYTE packet[8] = { 0xC1, 8, kGroup, 0x03 };
    memcpy(packet + 4, &listingId, 4);
    SocketClient->Send(packet, 8);
}

void CNewUIMarketplace::SendCreate()
{
    if (m_iSelectedInv < 0 || m_iSelectedInv >= m_iInvCount)
        return;

    const int price = _wtoi(m_szPrice);
    if (price <= 0)
        return;

    BYTE qty = static_cast<BYTE>(_wtoi(m_szQty));
    if (qty == 0)
        qty = 1;

    if (m_bDemoMode)
    {
        DemoCreateListing(price, qty);
        return;
    }

    if (SocketClient == nullptr)
        return;

    BYTE packet[12] = { 0xC1, 12, kGroup, 0x02 };
    packet[4] = static_cast<BYTE>(m_InvSlots[m_iSelectedInv]);
    memcpy(packet + 5, &price, 4);
    packet[9] = qty;
    packet[10] = static_cast<BYTE>(m_iDialogDays);
    packet[11] = static_cast<BYTE>(m_iDialogCategory);
    SocketClient->Send(packet, 12);
}

bool CNewUIMarketplace::IsDemoListing(int listingId) const
{
    return listingId < 0;
}

void CNewUIMarketplace::ReceiveList(const BYTE* buffer)
{
    if (buffer == nullptr)
        return;

    m_iPage = buffer[4];
    m_iTotalPages = buffer[5] == 0 ? 1 : buffer[5];
    m_iListingCount = buffer[6];
    if (m_iListingCount > kGridCount)
        m_iListingCount = kGridCount;

    if (m_iListingCount <= 0)
    {
        m_bDemoMode = true;
        ApplyVisibleListings();
        return;
    }

    m_bDemoMode = false;
    memset(m_Listings, 0, sizeof(m_Listings));
    for (int i = 0; i < m_iListingCount; ++i)
    {
        const BYTE* src = buffer + 8 + (i * 27);
        memcpy(&m_Listings[i].Id, src, 4);
        memcpy(&m_Listings[i].ItemCode, src + 4, 2);
        m_Listings[i].Level = src[6];
        m_Listings[i].Excellent = src[7];
        m_Listings[i].Ancient = src[8];
        memcpy(&m_Listings[i].Price, src + 9, 4);
        m_Listings[i].Quantity = src[13];
        m_Listings[i].Category = src[14];
        m_Listings[i].Status = src[15];
        char seller[11] = { 0 };
        memcpy(seller, src + 16, 10);
        MultiByteToWideChar(CP_UTF8, 0, seller, -1, m_Listings[i].Seller, 12);
    }

    SyncSlotButtons();
    RefreshListBox();
}

void CNewUIMarketplace::ReceiveResult(BYTE result)
{
    CMsgBoxIGSCommon* pMsgBox = nullptr;
    CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
    switch (result)
    {
    case 0:
        pMsgBox->Initialize(I18N::Game::MarketplaceTitle, I18N::Game::MarketplaceListed);
        m_bShowDialog = false;
        CollectInventory();
        SendListRequest();
        if (SocketClient != nullptr && SocketClient->ToGameServer() != nullptr)
            SocketClient->ToGameServer()->SendCashShopPointInfoRequest();
        break;
    case 2:
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::MarketplaceNotEnoughZen);
        break;
    case 3:
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::NotEnoughSpacePleaseCheckFreeSpaceInYourInventory);
        break;
    case 4:
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::MarketplaceNotFound);
        break;
    case 5:
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::MarketplaceOwnItem);
        break;
    case 6:
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::MarketplaceLimit);
        break;
    default:
        pMsgBox->Initialize(I18N::Game::Error, I18N::Game::ThereHasBeenAnError);
        break;
    }
}

void CNewUIMarketplace::FillDemoListings()
{
    auto setItem = [this](int index, int id, WORD code, BYTE level, BYTE excellent, int price, BYTE qty, BYTE category, BYTE status, const wchar_t* seller, BYTE luck = 0, BYTE skill = 0, BYTE option = 0)
    {
        Listing& item = m_DemoAll[index];
        memset(&item, 0, sizeof(item));
        item.Id = id;
        item.ItemCode = code;
        item.Level = level;
        item.Excellent = excellent;
        item.HasLuck = luck;
        item.HasSkill = skill;
        item.OptionLevel = option;
        item.Price = price;
        item.Quantity = qty;
        item.Category = category;
        item.Status = status;
        wcsncpy(item.Seller, seller, 11);
        item.Seller[11] = 0;
    };

    setItem(0, -1, static_cast<WORD>(ITEM_SWORD + 16), 15, 0x20, 2500000, 1, 1, 0, L"LordKain", 1, 1, 4);
    setItem(1, -2, static_cast<WORD>(ITEM_STAFF + 9), 9, 0x08, 1950000, 1, 1, 0, L"Archmage", 1, 0, 0);
    setItem(2, -3, static_cast<WORD>(ITEM_SWORD + 14), 13, 0, 890000, 1, 1, 0, L"BladeBoy", 1, 0, 4);
    setItem(3, -4, static_cast<WORD>(ITEM_ARMOR + 15), 13, 0x10, 1750000, 1, 2, 0, L"ForgeKing", 1, 0, 0);
    setItem(4, -5, static_cast<WORD>(ITEM_SHIELD + 14), 11, 0, 980000, 1, 2, 0, L"Paladin", 1, 0, 0);
    setItem(5, -6, static_cast<WORD>(ITEM_GUARDIAN_HELM), 13, 0, 720000, 1, 2, 0, L"IronHead");
    setItem(6, -7, static_cast<WORD>(ITEM_ARMOR + 15), 0, 0, 3800000, 1, 2, 0, L"NightElf");
    setItem(7, -8, static_cast<WORD>(ITEM_SHIELD + 6), 0, 0, 2100000, 1, 2, 0, L"SkyRider");
    setItem(8, -9, static_cast<WORD>(ITEM_POTION + 14), 0, 0, 450000, 1, 4, 0, L"Blacksmith");
    setItem(9, -10, static_cast<WORD>(ITEM_POTION + 13), 0, 0, 420000, 1, 4, 0, L"GemCutter");
    setItem(10, -11, static_cast<WORD>(ITEM_POTION + 16), 0, 0, 380000, 1, 4, 0, L"JewelShop");
    setItem(11, -12, static_cast<WORD>(ITEM_POTION + 3), 0, 0, 15000, 30, 5, 0, L"Alchemist");
    setItem(12, -13, static_cast<WORD>(ITEM_POTION + 1), 0, 0, 8000, 20, 5, 0, L"Healer");
    setItem(13, -14, static_cast<WORD>(ITEM_SWORD + 0), 0, 0, 1200000, 1, 1, 0, L"BeastMstr");
    setItem(14, -15, static_cast<WORD>(ITEM_POTION + 13), 0, 0, 650000, 1, 4, 0, L"GemCutter");
    setItem(15, -16, static_cast<WORD>(ITEM_STAFF + 0), 0, 0, 980000, 1, 1, 0, L"Stable");

    const wchar_t* selfName = (Hero && Hero->ID[0]) ? Hero->ID : L"TesteBR";
    setItem(16, -17, static_cast<WORD>(ITEM_ARMOR + 8), 0, 0, 3500000, 1, 2, 0, selfName);
    setItem(17, -18, static_cast<WORD>(ITEM_ARMOR + 15), 12, 0, 1300000, 1, 2, 0, selfName);
    setItem(18, -19, static_cast<WORD>(ITEM_POTION + 14), 0, 0, 250000, 2, 4, 1, selfName);
    m_iDemoCount = 19;

    if (m_iPurchasedCount == 0)
    {
        m_Purchased[0] = m_DemoAll[8];
        m_Purchased[1] = m_DemoAll[9];
        m_iPurchasedCount = 2;
    }

    if (m_iFeaturedCount == 0)
    {
        AddFeatured(m_DemoAll[0]);
        AddFeatured(m_DemoAll[3]);
        AddFeatured(m_DemoAll[7]);
        m_dwFeaturedTick = GetTickCount();
    }
}

void CNewUIMarketplace::ApplyVisibleListings()
{
    Listing filtered[24];
    int count = 0;
    const wchar_t* selfName = (Hero && Hero->ID[0]) ? Hero->ID : L"";

    for (int i = 0; i < m_iDemoCount && count < 24; ++i)
    {
        const Listing& src = m_DemoAll[i];
        const bool own = selfName[0] && _wcsicmp(src.Seller, selfName) == 0;
        if (m_iTab == 1)
        {
            if (!own || src.Status != 0)
                continue;
        }
        else if (m_iTab == 2)
        {
            if (!own || src.Status == 0)
                continue;
        }
        else if (src.Status != 0 || own)
        {
            continue;
        }

        if (m_iCategory > 0 && src.Category != m_iCategory)
            continue;

        filtered[count++] = src;
    }

    auto cmpPriceAsc = [](const Listing& a, const Listing& b) { return a.Price < b.Price; };
    auto cmpPriceDesc = [](const Listing& a, const Listing& b) { return a.Price > b.Price; };
    auto cmpNewest = [](const Listing& a, const Listing& b) { return a.Id < b.Id; };
    if (m_iSort == 1)
        std::sort(filtered, filtered + count, cmpPriceAsc);
    else if (m_iSort == 2)
        std::sort(filtered, filtered + count, cmpPriceDesc);
    else
        std::sort(filtered, filtered + count, cmpNewest);

    m_iTotalPages = count == 0 ? 1 : (count + kGridCount - 1) / kGridCount;
    if (m_iPage < 1)
        m_iPage = 1;
    if (m_iPage > m_iTotalPages)
        m_iPage = m_iTotalPages;

    const int start = (m_iPage - 1) * kGridCount;
    memset(m_Listings, 0, sizeof(m_Listings));
    m_iListingCount = 0;
    for (int i = 0; i < kGridCount && start + i < count; ++i)
    {
        m_Listings[i] = filtered[start + i];
        ++m_iListingCount;
    }

    SyncSlotButtons();
    RefreshListBox();
}

BYTE CNewUIMarketplace::SortPacketValue() const
{
    if (m_iSort == 1)
        return 2;
    if (m_iSort == 2)
        return 3;
    return 0;
}

bool CNewUIMarketplace::IsCartTab()
{
    return m_ListBoxTabButton.GetCurButtonIndex() == LISTTAB_CART;
}

bool CNewUIMarketplace::IsInvTab()
{
    return m_ListBoxTabButton.GetCurButtonIndex() == LISTTAB_INVENTORY;
}

bool CNewUIMarketplace::IsItemsTab()
{
    return m_ListBoxTabButton.GetCurButtonIndex() == LISTTAB_ITEMS;
}

void CNewUIMarketplace::UpdateActionButton()
{
    if (IsCartTab())
        m_UseButton.ChangeText(&I18N::Game::Buy1124);
    else if (IsItemsTab())
        m_UseButton.ChangeText(&I18N::Game::Use);
    else
        m_UseButton.ChangeText(&I18N::Game::MarketplaceAnnounce);
}

void CNewUIMarketplace::AddToPurchased(const Listing& listing)
{
    if (m_iPurchasedCount >= 24)
        return;
    m_Purchased[m_iPurchasedCount] = listing;
    ++m_iPurchasedCount;
}

void CNewUIMarketplace::AddFeatured(const Listing& listing)
{
    if (!CanRenderItem(listing.ItemCode))
        return;
    Listing featured = listing;
    if (featured.FeatureHours == 0)
        featured.FeatureHours = kFeatureHours;
    for (int i = 0; i < m_iFeaturedCount; ++i)
    {
        if (m_Featured[i].Id == featured.Id)
        {
            m_Featured[i] = featured;
            return;
        }
    }
    if (m_iFeaturedCount >= kFeaturedMax)
        m_iFeaturedCount = kFeaturedMax - 1;
    m_Featured[m_iFeaturedCount] = featured;
    ++m_iFeaturedCount;
}

void CNewUIMarketplace::HighlightListing(int index)
{
    if (index < 0 || index >= m_iListingCount)
        return;

    if (g_InGameShopSystem->GetCashCreditCard() < kFeatureCostW)
    {
        ShowMarketplaceNotice(I18N::Game::Error, I18N::Game::MarketplaceNotEnoughW);
        return;
    }

    s_PendingHighlightId = m_Listings[index].Id;
    s_HasPendingHighlight = true;

    CNewUICommonMessageBox* pMsgBox = nullptr;
    CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMarketplaceHighlightLayout), &pMsgBox);
    if (pMsgBox == nullptr)
        return;

    wchar_t confirm[256] = {};
    mu_swprintf(confirm, I18N::Game::MarketplaceFeatureConfirm, kFeatureCostW, kFeatureHours);
    pMsgBox->AddMsg(I18N::Game::MarketplaceHighlightButton, 0xFFFEB048, MSGBOX_FONT_BOLD);
    pMsgBox->AddMsg(L" ");
    pMsgBox->AddMsg(confirm);
}

void CNewUIMarketplace::ConfirmHighlight()
{
    if (!s_HasPendingHighlight)
        return;

    const int listingId = s_PendingHighlightId;
    s_HasPendingHighlight = false;

    int index = -1;
    for (int i = 0; i < m_iListingCount; ++i)
    {
        if (m_Listings[i].Id == listingId)
        {
            index = i;
            break;
        }
    }
    if (index < 0)
        return;

    const double cash = g_InGameShopSystem->GetCashCreditCard();
    if (cash < kFeatureCostW)
    {
        ShowMarketplaceNotice(I18N::Game::Error, I18N::Game::MarketplaceNotEnoughW);
        return;
    }

    g_InGameShopSystem->SetCashCreditCard(cash - kFeatureCostW);
    AddFeatured(m_Listings[index]);
    for (int i = 0; i < m_iFeaturedCount; ++i)
    {
        if (m_Featured[i].Id == listingId)
        {
            m_iFeaturedSlide = i;
            break;
        }
    }
    m_dwFeaturedTick = GetTickCount();
}

void CNewUIMarketplace::SyncSlotButtons()
{
    for (int i = 0; i < kGridCount; ++i)
    {
        m_BuyButton[i].ChangeText(m_iTab == 1 ? &I18N::Game::MarketplaceCancelSale : &I18N::Game::MarketplaceCartTab);
        m_ViewButton[i].ChangeText(m_iTab == 1 ? &I18N::Game::MarketplaceHighlightButton : &I18N::Game::MarketplaceViewOffer);
    }
}

void CNewUIMarketplace::AdvanceFeatured()
{
    if (m_iFeaturedCount <= 1)
        return;
    const DWORD now = GetTickCount();
    if (now - m_dwFeaturedTick < 4000)
        return;
    m_dwFeaturedTick = now;
    m_iFeaturedSlide = (m_iFeaturedSlide + 1) % m_iFeaturedCount;
}

bool CNewUIMarketplace::CanRenderItem(WORD itemCode) const
{
    if (itemCode >= MAX_ITEM)
        return false;
    if (ItemAttribute == nullptr || ItemAttribute[itemCode].Name[0] == 0)
        return false;
    const int helper = static_cast<int>(itemCode) - ITEM_HELPER;
    if (helper >= 0 && helper <= 1)
        return false;
    if (helper == 4 || helper == 5)
        return false;
    return true;
}

void CNewUIMarketplace::DemoCreateListing(int price, BYTE qty)
{
    if (m_iDemoCount >= 24)
        m_iDemoCount = 23;

    Listing item = {};
    item.Id = -100 - m_iDemoCount;
    item.ItemCode = m_InvTypes[m_iSelectedInv];
    item.Level = m_InvLevels[m_iSelectedInv];
    item.Excellent = m_InvExcellent[m_iSelectedInv];
    item.Ancient = m_InvAncient[m_iSelectedInv];
    item.OptionLevel = m_InvOption[m_iSelectedInv];
    item.OptionType = m_InvOptionType[m_iSelectedInv];
    item.HasLuck = m_InvLuck[m_iSelectedInv];
    item.HasSkill = m_InvSkill[m_iSelectedInv];
    item.Price = price;
    item.Quantity = qty;
    item.Category = m_iDialogCategory;
    item.Status = 0;
    item.FeatureHours = kFeatureHours;
    const wchar_t* selfName = (Hero && Hero->ID[0]) ? Hero->ID : L"Teste";
    wcsncpy(item.Seller, selfName, 11);
    item.Seller[11] = 0;

    m_DemoAll[m_iDemoCount] = item;
    ++m_iDemoCount;
    m_iTab = 1;
    m_iPage = 1;
    m_TabButton.ChangeFrame(1);
    ApplyVisibleListings();

    CMsgBoxIGSCommon* pMsgBox = nullptr;
    CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
    pMsgBox->Initialize(I18N::Game::MarketplaceAnnounce, I18N::Game::MarketplaceListed);
}

void CNewUIMarketplace::AddToCart(const Listing& listing)
{
    for (int i = 0; i < m_iCartCount; ++i)
    {
        if (m_Cart[i].Id == listing.Id)
        {
            m_iSelectedListing = i;
            m_ListBoxTabButton.ChangeFrame(LISTTAB_CART);
            RefreshListBox();
            return;
        }
    }

    if (m_iCartCount >= kGridCount)
        m_iCartCount = kGridCount - 1;

    m_Cart[m_iCartCount] = listing;
    m_iSelectedListing = m_iCartCount;
    ++m_iCartCount;
    m_ListBoxTabButton.ChangeFrame(LISTTAB_CART);
    RefreshListBox();
}

void CNewUIMarketplace::ShowOffer(int index)
{
    if (index < 0 || index >= m_iListingCount)
        return;
    ShowListing(m_Listings[index], false);
}

void CNewUIMarketplace::BuyCurrentOffer()
{
    if (!s_OfferCanBuy)
        return;
    AddToCart(s_OfferListing);
}

void CNewUIMarketplace::ShowListing(const Listing& listing, bool canBuy)
{
    s_OfferListing = listing;
    s_OfferCanBuy = canBuy;

    wchar_t name[64] = {};
    wchar_t title[80] = {};
    wchar_t price[32] = {};
    wchar_t sellerLine[96] = {};
    wchar_t priceLine[96] = {};
    GetItemName(listing.ItemCode, listing.Level, name);
    if (listing.Excellent > 0)
        mu_swprintf(title, L"%ls %ls", I18N::Game::Excellent, name);
    else
        wcsncpy(title, name, 79);
    title[79] = 0;
    ConvertGold(listing.Price, price);
    mu_swprintf(sellerLine, L"%ls: %ls", I18N::Game::MarketplaceSeller, listing.Seller);
    mu_swprintf(priceLine, L"%ls: %ls W", I18N::Game::MarketplacePrice, price);

    ITEM preview = MakeListingItem(listing);

    CNewUI3DItemCommonMsgBox* pMsgBox = nullptr;
    if (canBuy)
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMarketplaceFeaturedLayout), &pMsgBox);
    else
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMarketplaceOfferLayout), &pMsgBox);
    if (pMsgBox == nullptr)
        return;
    pMsgBox->Set3DItem(&preview);
    pMsgBox->AddMsg(title, RGBA(255, 238, 161, 255), MSGBOX_FONT_BOLD);
    pMsgBox->AddMsg(sellerLine);
    pMsgBox->AddMsg(priceLine);
    if (listing.FeatureHours > 0)
    {
        wchar_t featuredLine[64] = {};
        mu_swprintf(featuredLine, I18N::Game::MarketplaceFeatureHours, listing.FeatureHours);
        pMsgBox->AddMsg(featuredLine, RGBA(255, 238, 161, 255));
    }
    AppendOfferStats(pMsgBox, preview);
    PlayBuffer(SOUND_CLICK01);
}

void CNewUIMarketplace::OpenAnnounceDialog()
{
    CollectInventory();
    m_ListBoxTabButton.ChangeFrame(LISTTAB_INVENTORY);
    RefreshListBox();

    IGS_StorageItem* selected = m_ItemListBox.GetSelectedText();
    if (selected)
        m_iSelectedInv = selected->m_iStorageItemSeq;

    if (m_iSelectedInv < 0 || m_iSelectedInv >= m_iInvCount)
    {
        CMsgBoxIGSCommon* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        pMsgBox->Initialize(I18N::Game::MarketplaceAnnounce, I18N::Game::MarketplaceEmptyInv);
        PlayBuffer(SOUND_CLICK01);
        return;
    }

    wchar_t name[64] = {};
    GetItemName(m_InvTypes[m_iSelectedInv], m_InvLevels[m_iSelectedInv], name);

    CNewUITextInputMsgBox* pMsgBox = nullptr;
    CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMarketplaceAnnounceLayout), &pMsgBox);
    if (pMsgBox == nullptr)
        return;
    pMsgBox->AddMsg(I18N::Game::MarketplaceAnnounce, RGBA(255, 238, 161, 255), MSGBOX_FONT_BOLD);
    pMsgBox->AddMsg(name);
    pMsgBox->AddMsg(I18N::Game::EnterSellingPrice);
    PlayBuffer(SOUND_CLICK01);
}

void CNewUIMarketplace::ConfirmAnnounce(const wchar_t* priceText)
{
    if (priceText == nullptr || priceText[0] == 0)
        return;
    wcsncpy(m_szPrice, priceText, 15);
    m_szPrice[15] = 0;
    m_szQty[0] = L'1';
    m_szQty[1] = 0;
    m_iDialogDays = 7;
    SendCreate();
}

void CNewUIMarketplace::BuySelectedCart()
{
    if (m_iCartCount <= 0)
        return;

    IGS_StorageItem* selected = m_ItemListBox.GetSelectedText();
    int index = selected ? selected->m_iStorageItemSeq : m_iSelectedListing;
    if (index < 0 || index >= m_iCartCount)
        index = 0;

    if (IsDemoListing(m_Cart[index].Id))
    {
        if (!TryDebitLocalW(m_Cart[index].Price))
        {
            ShowNotEnoughW();
            return;
        }
        AddToPurchased(m_Cart[index]);
        for (int i = index; i < m_iCartCount - 1; ++i)
            m_Cart[i] = m_Cart[i + 1];
        --m_iCartCount;
        m_ListBoxTabButton.ChangeFrame(LISTTAB_ITEMS);
        RefreshListBox();
        CMsgBoxIGSCommon* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox);
        pMsgBox->Initialize(I18N::Game::MarketplaceTitle, I18N::Game::MarketplaceDemoNotice);
        return;
    }

    SendBuy(m_Cart[index].Id);
}

void CNewUIMarketplace::ClaimPurchased()
{
    if (m_iPurchasedCount <= 0)
        return;

    IGS_StorageItem* selected = m_ItemListBox.GetSelectedText();
    int index = selected ? selected->m_iStorageItemSeq : 0;
    if (index < 0 || index >= m_iPurchasedCount)
        index = 0;

    for (int i = index; i < m_iPurchasedCount - 1; ++i)
        m_Purchased[i] = m_Purchased[i + 1];
    --m_iPurchasedCount;
    RefreshListBox();
}

void CNewUIMarketplace::CollectFromCtrl(CNewUIInventoryCtrl* pCtrl)
{
    if (pCtrl == nullptr)
        return;

    const int count = static_cast<int>(pCtrl->GetNumberOfItems());
    for (int i = 0; i < count && m_iInvCount < 64; ++i)
    {
        ITEM* pItem = pCtrl->GetItem(i);
        if (pItem == nullptr || pItem->Type < 0 || pItem->Type >= MAX_ITEM)
            continue;
        m_InvSlots[m_iInvCount] = pCtrl->GetIndexByItem(pItem);
        m_InvTypes[m_iInvCount] = static_cast<WORD>(pItem->Type);
        m_InvLevels[m_iInvCount] = static_cast<BYTE>(pItem->Level);
        m_InvQty[m_iInvCount] = pItem->Durability > 0 ? pItem->Durability : 1;
        m_InvExcellent[m_iInvCount] = pItem->ExcellentFlags;
        m_InvAncient[m_iInvCount] = pItem->AncientDiscriminator;
        m_InvOption[m_iInvCount] = pItem->OptionLevel;
        m_InvOptionType[m_iInvCount] = pItem->OptionType;
        m_InvLuck[m_iInvCount] = pItem->HasLuck ? 1 : 0;
        m_InvSkill[m_iInvCount] = pItem->HasSkill ? 1 : 0;
        ++m_iInvCount;
    }
}

void CNewUIMarketplace::CollectInventory()
{
    const int previous = m_iSelectedInv;
    m_iInvCount = 0;

    if (g_pMyInventory)
        CollectFromCtrl(g_pMyInventory->GetInventoryCtrl());

    const int pages = m_iInvCount == 0 ? 1 : (m_iInvCount + kInvRows - 1) / kInvRows;
    if (m_iInvPage >= pages)
        m_iInvPage = pages - 1;
    m_iSelectedInv = (previous >= 0 && previous < m_iInvCount) ? previous : -1;
}

void CNewUIMarketplace::RefreshListBox()
{
    m_ItemListBox.Clear();
    auto addListing = [this](const Listing& src, int index, bool showPrice)
    {
        IGS_StorageItem item = {};
        GetItemName(src.ItemCode, src.Level, item.m_szName);
        if (showPrice)
            ConvertGold(src.Price, item.m_szPeriod);
        else
            mu_swprintf(item.m_szPeriod, L"%u", src.Quantity);
        // Keep qty/price only in the right column — m_iNum>1 would append "(N)" to the name
        // inside CUIInGameShopListBox::RenderDataLine and duplicate the Qtd. column.
        item.m_iNum = 1;
        item.m_wItemCode = src.ItemCode;
        item.m_iStorageSeq = src.Id;
        item.m_iStorageItemSeq = index;
        wcsncpy(item.m_szSendUserName, src.Seller, MAX_USERNAME_SIZE);
        m_ItemListBox.AddText(item);
    };

    if (IsInvTab())
    {
        const int start = m_iInvPage * kInvRows;
        const int last = (start + kInvRows < m_iInvCount) ? start + kInvRows : m_iInvCount;
        for (int index = last - 1; index >= start; --index)
        {
            IGS_StorageItem item = {};
            GetItemName(m_InvTypes[index], m_InvLevels[index], item.m_szName);
            mu_swprintf(item.m_szPeriod, L"%u", m_InvQty[index]);
            item.m_iNum = 1; // qty lives in m_szPeriod / Qtd. column only
            item.m_wItemCode = m_InvTypes[index];
            item.m_iStorageSeq = m_InvSlots[index];
            item.m_iStorageItemSeq = index;
            m_ItemListBox.AddText(item);
        }
        if (m_iSelectedInv >= start && m_iSelectedInv < last)
            m_ItemListBox.SLSetSelectLine(last - m_iSelectedInv);
        UpdateActionButton();
        return;
    }

    if (IsItemsTab())
    {
        for (int i = m_iPurchasedCount - 1; i >= 0; --i)
            addListing(m_Purchased[i], i, true);
        UpdateActionButton();
        return;
    }

    for (int i = m_iCartCount - 1; i >= 0; --i)
        addListing(m_Cart[i], i, true);

    if (m_iCartCount > 0)
    {
        int selected = m_iSelectedListing;
        if (selected < 0 || selected >= m_iCartCount)
            selected = 0;
        m_ItemListBox.SLSetSelectLine(selected + 1);
    }

    UpdateActionButton();
}

const wchar_t* CNewUIMarketplace::TabName(int index) const
{
    switch (index)
    {
    case 1: return I18N::Game::MarketplaceMySalesTab;
    case 2: return I18N::Game::MarketplaceHistoryTab;
    default: return I18N::Game::MarketplaceBuyTab;
    }
}

const wchar_t* CNewUIMarketplace::SortName(int index) const
{
    switch (index)
    {
    case 1: return I18N::Game::MarketplaceLowestPrice;
    case 2: return I18N::Game::MarketplaceHighestPrice;
    default: return I18N::Game::MarketplaceNewest;
    }
}

const wchar_t* CNewUIMarketplace::CategoryName(int index) const
{
    switch (index)
    {
    case 1: return I18N::Game::MarketplaceWeapons;
    case 2: return I18N::Game::MarketplaceArmors;
    case 3: return I18N::Game::MarketplaceWings;
    case 4: return I18N::Game::MarketplaceJewels;
    case 5: return I18N::Game::MarketplaceConsumables;
    case 6: return I18N::Game::MarketplacePets;
    default: return I18N::Game::MarketplaceAll;
    }
}

bool CNewUIMarketplace::Update()
{
    AdvanceFeatured();
    return true;
}

bool CNewUIMarketplace::UpdateKeyEvent()
{
    if (!IsVisible())
        return true;

    if (IsPress(VK_ESCAPE))
    {
        if (m_bShowDialog)
        {
            m_bShowDialog = false;
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        g_pNewUISystem->Hide(INTERFACE_MARKETPLACE);
        return false;
    }

    if (m_bShowDialog && m_iPriceFocus == 0)
    {
        if (IsPress(VK_BACK) && m_szPrice[0] != 0)
        {
            m_szPrice[wcslen(m_szPrice) - 1] = 0;
            return false;
        }

        for (int key = '0'; key <= '9'; ++key)
        {
            if (IsPress(key) && wcslen(m_szPrice) < 10)
            {
                const wchar_t ch = static_cast<wchar_t>(key);
                const size_t len = wcslen(m_szPrice);
                m_szPrice[len] = ch;
                m_szPrice[len + 1] = 0;
                return false;
            }
        }
    }

    return true;
}

bool CNewUIMarketplace::UpdateMouseEvent()
{
    if (!IsVisible())
        return true;

    if (m_bShowDialog && DialogProcess())
        return false;

    if (BtnProcess())
        return false;

    if (CheckMouseIn(m_Pos.x, m_Pos.y, kBackW, kBackH))
    {
        m_ItemListBox.DoAction();
        if (IsInvTab())
        {
            IGS_StorageItem* selected = m_ItemListBox.GetSelectedText();
            if (selected)
                m_iSelectedInv = selected->m_iStorageItemSeq;
        }
        if (IsPress(VK_LBUTTON) || IsPress(VK_RBUTTON))
            return false;
        return false;
    }

    return true;
}

bool CNewUIMarketplace::BtnProcess()
{
    if (m_CloseButton.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(INTERFACE_MARKETPLACE);
        return true;
    }

    const int tabIndex = m_TabButton.UpdateMouseEvent();
    if (tabIndex != RADIOGROUPEVENT_NONE)
    {
        m_iTab = m_TabButton.GetCurButtonIndex();
        m_iPage = 1;
        if (m_bDemoMode)
            ApplyVisibleListings();
        SendListRequest();
        return true;
    }

    const int sortIndex = m_SortButton.UpdateMouseEvent();
    if (sortIndex != RADIOGROUPEVENT_NONE)
    {
        m_iSort = m_SortButton.GetCurButtonIndex();
        m_iPage = 1;
        if (m_bDemoMode)
            ApplyVisibleListings();
        SendListRequest();
        return true;
    }

    if (m_ListBoxTabButton.UpdateMouseEvent() != RADIOGROUPEVENT_NONE)
    {
        if (IsInvTab())
            CollectInventory();
        RefreshListBox();
        return true;
    }

    const int catIndex = m_CategoryButton.UpdateMouseEvent();
    if (catIndex != RADIOGROUPEVENT_NONE)
    {
        m_iCategory = m_CategoryButton.GetCurButtonIndex();
        m_iPage = 1;
        if (m_bDemoMode)
            ApplyVisibleListings();
        SendListRequest();
        return true;
    }

    if (m_ListItemButton.UpdateMouseEvent())
        return true;

    if (m_CashChargeButton.UpdateMouseEvent())
    {
        OpenRechargePage();
        return true;
    }

    if (m_RefreshButton.UpdateMouseEvent())
    {
        CollectInventory();
        SendListRequest();
        SocketClient->ToGameServer()->SendCashShopPointInfoRequest();
        if (m_bDemoMode)
            ApplyVisibleListings();
        return true;
    }

    if (m_iFeaturedCount > 0 && IsPress(VK_LBUTTON)
        && CheckMouseIn(m_Pos.x + kBannerX, m_Pos.y + kBannerY, kBannerW, kBannerH))
    {
        ShowListing(m_Featured[m_iFeaturedSlide], true);
        return true;
    }

    if (m_UseButton.UpdateMouseEvent())
    {
        if (IsCartTab())
            BuySelectedCart();
        else if (IsItemsTab())
            ClaimPurchased();
        else
            OpenAnnounceDialog();
        return true;
    }

    for (int i = 0; i < m_iListingCount; ++i)
    {
        if ((m_iTab == 0 || m_iTab == 1) && m_ViewButton[i].UpdateMouseEvent())
        {
            if (m_iTab == 1)
                HighlightListing(i);
            else
                ShowOffer(i);
            return true;
        }
        if (m_BuyButton[i].UpdateMouseEvent())
        {
            if (m_iTab == 1)
                SendCancel(m_Listings[i].Id);
            else
                AddToCart(m_Listings[i]);
            return true;
        }

        const int slotX = (i % kGridCols) * kSlotDX;
        const int slotY = (i / kGridCols) * kSlotDY;
        if (IsPress(VK_LBUTTON) && CheckMouseIn(m_Pos.x + kBoxX + slotX, m_Pos.y + kBoxY + slotY, kBoxSize, kBoxSize))
        {
            m_iSelectedListing = i;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }

    if (m_PrevButton.UpdateMouseEvent() && m_iPage > 1)
    {
        --m_iPage;
        if (m_bDemoMode)
            ApplyVisibleListings();
        SendListRequest();
        return true;
    }

    if (m_NextButton.UpdateMouseEvent() && m_iPage < m_iTotalPages)
    {
        ++m_iPage;
        if (m_bDemoMode)
            ApplyVisibleListings();
        SendListRequest();
        return true;
    }

    const int invPages = m_iInvCount == 0 ? 1 : (m_iInvCount + kInvRows - 1) / kInvRows;
    if (m_InvPrevButton.UpdateMouseEvent() && m_iInvPage > 0)
    {
        --m_iInvPage;
        RefreshListBox();
        return true;
    }

    if (m_InvNextButton.UpdateMouseEvent() && m_iInvPage + 1 < invPages)
    {
        ++m_iInvPage;
        RefreshListBox();
        return true;
    }

    return false;
}

bool CNewUIMarketplace::DialogProcess()
{
    const int dlgX = m_Pos.x + (kBackW - kDlgW) / 2;
    const int dlgY = m_Pos.y + 150;
    m_DialogCancel.ChangeButtonInfo(dlgX + 170, dlgY + 172, kViewW, kViewH);
    m_DialogOk.ChangeButtonInfo(dlgX + 230, dlgY + 172, kViewW, kViewH);

    if (m_DialogCancel.UpdateMouseEvent())
    {
        m_bShowDialog = false;
        return true;
    }

    if (m_DialogOk.UpdateMouseEvent())
    {
        SendCreate();
        return true;
    }

    if (IsPress(VK_LBUTTON) && CheckMouseIn(dlgX + kDlgW - 20, dlgY + 6, 16, 16))
    {
        m_bShowDialog = false;
        return true;
    }

    if (IsPress(VK_LBUTTON) && CheckMouseIn(dlgX + 170, dlgY + 78, 140, 16))
    {
        m_iDialogCategory = (m_iDialogCategory + 1) % kCategoryCount;
        return true;
    }

    if (IsPress(VK_LBUTTON) && CheckMouseIn(dlgX + 20, dlgY + 148, 120, 16))
    {
        m_iDialogDays = m_iDialogDays == 1 ? 3 : (m_iDialogDays == 3 ? 7 : 1);
        return true;
    }

    IGS_StorageItem* selected = m_ItemListBox.GetSelectedText();
    if (selected && IsInvTab())
        m_iSelectedInv = selected->m_iStorageItemSeq;

    if (CheckMouseIn(dlgX, dlgY, kDlgW, kDlgH))
        return IsPress(VK_LBUTTON) || !IsNone(VK_LBUTTON);

    return false;
}

bool CNewUIMarketplace::Render()
{
    if (!m_bUiReady || !IsVisible())
        return true;

    static bool s_loggedFirstRender = false;
    if (!s_loggedFirstRender)
    {
        s_loggedFirstRender = true;
        g_ConsoleDebug->Write(MCD_ERROR, L"MP-TRACE: first Render listings=%d", m_iListingCount);
    }

    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderFrame();
    RenderButtons();
    RenderTexts();
    RenderListBox();
    RenderGrid();
    DisableAlphaBlend();
    return true;
}

void CNewUIMarketplace::RenderFrame()
{
    RenderImage(IMAGE_MP_BACK, static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y), static_cast<float>(kBackW), static_cast<float>(kBackH));

    POINT deco;
    deco.x = m_TabButton.GetPos(0).x + (kCatW / 2) - 2;
    for (int i = 0; i < kTabCount - 1; ++i)
    {
        deco.y = m_TabButton.GetPos(i).y + kCatH - 1;
        RenderImage(IMAGE_MP_CATEGORY_DECO_MIDDLE, static_cast<float>(deco.x), static_cast<float>(deco.y), 4.f, 8.f);
    }

    const POINT lastTab = m_TabButton.GetPos(kTabCount - 1);
    RenderImage(
        IMAGE_MP_CATEGORY_DECO_DOWN,
        static_cast<float>(m_Pos.x),
        static_cast<float>(lastTab.y + kCatH + 4),
        static_cast<float>(kFloreioW),
        static_cast<float>(kFloreioH));

    for (int i = 0; i < kCategoryCount - 1; ++i)
    {
        deco.y = m_CategoryButton.GetPos(i).y + kCatH - 1;
        RenderImage(IMAGE_MP_CATEGORY_DECO_MIDDLE, static_cast<float>(deco.x), static_cast<float>(deco.y), 4.f, 8.f);
    }

    for (int cnt = m_iListingCount; cnt < kGridCount; ++cnt)
    {
        RenderImage(IMAGE_MP_ITEMBOX,
            static_cast<float>(m_Pos.x + kBoxX + ((cnt % kGridCols) * kSlotDX)),
            static_cast<float>(m_Pos.y + kBoxY + ((cnt / kGridCols) * kSlotDY)),
            static_cast<float>(kBoxSize), static_cast<float>(kBoxSize));
    }

    RenderImage(IMAGE_MP_STORAGE_PAGE, static_cast<float>(m_Pos.x + 518), static_cast<float>(m_Pos.y + 366), 80.f, 30.f);
    RenderFeaturedPanel();
}

void CNewUIMarketplace::RenderFeaturedPanel()
{
    if (m_iFeaturedCount > 0)
        return;

    BITMAP_t* pImage = &Bitmaps[IMAGE_MP_BANNER];
    if (pImage == nullptr || pImage->Width <= 1.f)
        return;

    RenderImageStretch(
        IMAGE_MP_BANNER,
        static_cast<float>(m_Pos.x + kBannerX),
        static_cast<float>(m_Pos.y + kBannerY),
        static_cast<float>(kBannerW),
        static_cast<float>(kBannerH),
        0.f,
        0.f,
        pImage->Width,
        pImage->Height);
}

void CNewUIMarketplace::RenderButtons()
{
    m_TabButton.Render();
    m_CategoryButton.Render();
    m_SortButton.Render();
    m_ListBoxTabButton.Render();
    for (int i = 0; i < m_iListingCount; ++i)
    {
        if (m_iTab == 0 || m_iTab == 1)
            m_ViewButton[i].Render();
        m_BuyButton[i].Render();
    }
    m_CashChargeButton.Render();
    m_RefreshButton.Render();
    m_UseButton.Render();
    m_PrevButton.Render();
    m_NextButton.Render();
    m_InvPrevButton.Render();
    m_InvNextButton.Render();
    m_CloseButton.Render();
}

void CNewUIMarketplace::RenderTexts()
{
    wchar_t szText[128];
    wchar_t szValue[64];
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetFont(g_hFontBold);

    g_pRenderText->SetTextColor(255, 255, 255, 255);
    for (int i = 0; i < m_iListingCount; ++i)
    {
        GetItemName(m_Listings[i].ItemCode, m_Listings[i].Level, szText);
        g_pRenderText->RenderText(m_Pos.x + kNameX + ((i % kGridCols) * kSlotDX), m_Pos.y + kNameY + ((i / kGridCols) * kSlotDY), szText, kNameW, 0, RT3_SORT_CENTER);
        ConvertGold(m_Listings[i].Price, szValue);
        mu_swprintf(szText, L"%ls W", szValue);
        g_pRenderText->SetTextColor(255, 238, 161, 255);
        g_pRenderText->RenderText(m_Pos.x + kNameX + ((i % kGridCols) * kSlotDX), m_Pos.y + kPriceY + ((i / kGridCols) * kSlotDY), szText, kNameW, 0, RT3_SORT_CENTER);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
    }

    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(m_Pos.x + kCashX, m_Pos.y + 65,
        I18N::Game::MarketplaceFeatured, kCashW, 0, RT3_SORT_LEFT);
    {
        wchar_t cost[64] = {};
        mu_swprintf(cost, I18N::Game::MarketplaceFeatureCost, kFeatureCostW, kFeatureHours);
        g_pRenderText->SetTextColor(255, 200, 120, 255);
        g_pRenderText->RenderText(m_Pos.x + kCashX, m_Pos.y + 80,
            cost, kCashW, 0, RT3_SORT_LEFT);
    }

    if (m_iFeaturedCount > 0)
    {
        const Listing& featured = m_Featured[m_iFeaturedSlide];
        wchar_t hours[32] = {};
        mu_swprintf(hours, I18N::Game::MarketplaceFeatureHours, featured.FeatureHours);
        wchar_t featName[64] = {};
        GetItemName(featured.ItemCode, featured.Level, featName);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->RenderText(m_Pos.x + kBannerX, m_Pos.y + kBannerY + kBannerH - 24, featName, kBannerW, 0, RT3_SORT_CENTER);
        g_pRenderText->SetTextColor(255, 238, 161, 255);
        g_pRenderText->RenderText(m_Pos.x + kBannerX, m_Pos.y + kBannerY + kBannerH - 12, hours, kBannerW, 0, RT3_SORT_CENTER);
    }
    else
    {
        g_pRenderText->SetTextColor(255, 238, 161, 255);
        g_pRenderText->RenderText(m_Pos.x + kBannerX, m_Pos.y + kBannerY + kBannerH - 16,
            I18N::Game::MarketplaceBannerSubtitle, kBannerW, 0, RT3_SORT_CENTER);
    }

    g_pRenderText->SetTextColor(255, 255, 255, 255);
    const wchar_t* heroName = (Hero && Hero->ID[0]) ? Hero->ID : L"";
    g_pRenderText->RenderText(m_Pos.x + kCharNameX, m_Pos.y + 23, heroName, kCharNameW, 0, RT3_SORT_CENTER);

    ConvertGold(g_InGameShopSystem->GetCashCreditCard(), szValue);
    mu_swprintf(szText, I18N::Game::MyWCoinS, szValue);
    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(m_Pos.x + kCashX, m_Pos.y + kCashY, szText, kCashW, 0, RT3_SORT_LEFT);

    g_pRenderText->SetTextColor(255, 255, 255, 255);
    // Same slots as CNewUIInGameShop::RenderFrame storage headers — CENTER inside the
    // chrome columns so tabs / header / listbox share the baked panel limits.
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->RenderText(m_Pos.x + kStorageHeaderNameX, m_Pos.y + kStorageHeaderY,
        I18N::Game::ItemName, kStorageHeaderNameW, 0, RT3_SORT_CENTER);
    g_pRenderText->RenderText(m_Pos.x + kStorageHeaderQtyX, m_Pos.y + kStorageHeaderY,
        IsInvTab() ? I18N::Game::MarketplaceQty : L"W", kStorageHeaderQtyW, 0, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);

    g_pRenderText->RenderText(m_Pos.x + 251 + 23, m_Pos.y + 404, L"/", 10, 0, RT3_SORT_CENTER);
    mu_swprintf(szText, L"%d", m_iPage);
    g_pRenderText->RenderText(m_Pos.x + 256, m_Pos.y + 404, szText, 15, 0, RT3_SORT_RIGHT);
    mu_swprintf(szText, L"%d", m_iTotalPages);
    g_pRenderText->RenderText(m_Pos.x + 287, m_Pos.y + 404, szText, 15, 0, RT3_SORT_LEFT);

    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(m_Pos.x + 100, m_Pos.y + 404, I18N::Game::MarketplaceFeeNotice, 120, 0, RT3_SORT_LEFT);

    const int invPages = m_iInvCount == 0 ? 1 : (m_iInvCount + kInvRows - 1) / kInvRows;
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText(m_Pos.x + 518 + 35, m_Pos.y + 376, L"/", 10, 0, RT3_SORT_CENTER);
    mu_swprintf(szText, L"%d", m_iInvPage + 1);
    g_pRenderText->RenderText(m_Pos.x + 530, m_Pos.y + 376, szText, 20, 0, RT3_SORT_RIGHT);
    mu_swprintf(szText, L"%d", invPages);
    g_pRenderText->RenderText(m_Pos.x + 566, m_Pos.y + 376, szText, 20, 0, RT3_SORT_LEFT);
}

void CNewUIMarketplace::RenderListBox()
{
    m_ItemListBox.Render();
}

void CNewUIMarketplace::RenderGrid()
{
    EndBitmap();
    memcpy(s_PreProj, GlobalUBO::Instance().GetProj(), sizeof(s_PreProj));
    memcpy(s_PreView, GlobalUBO::Instance().GetView(), sizeof(s_PreView));
    SaveCameraPerspective();
    glViewport2(0, 0, WindowWidth, WindowHeight);
    gluPerspective2(2.0f, (float)WindowWidth / (float)WindowHeight, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR);
    {
        float aspect = (float)WindowWidth / (float)WindowHeight;
        float fovRad = 2.0f * 0.5f * Q_PI / 180.0f;
        float f = 1.0f / tanf(fovRad);
        float cpuProj[16];
        BuildPerspectiveProjection(f, aspect, RENDER_ITEMVIEW_NEAR, RENDER_ITEMVIEW_FAR, cpuProj);
        float cpuView[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        GlobalUBO::Instance().SetProj(cpuProj);
        GlobalUBO::Instance().SetView(cpuView);
        static const float s_IdentityCameraMatrix[3][4] = {
            {1.f,0.f,0.f,0.f}, {0.f,1.f,0.f,0.f}, {0.f,0.f,1.f,0.f}
        };
        memcpy(g_Camera.Matrix, s_IdentityCameraMatrix, sizeof(g_Camera.Matrix));
    }
    EnableDepthTest();
    EnableDepthMask();
    ClearDepthBuffer();

    for (int i = 0; i < m_iListingCount; ++i)
    {
        if (!CanRenderItem(m_Listings[i].ItemCode))
            continue;
        const int x = m_Pos.x + kItem3DX + (kSlotDX * (i % kGridCols));
        const int y = m_Pos.y + kItem3DY + (kSlotDY * (i / kGridCols));
        RenderMarketplaceItem(static_cast<float>(x), static_cast<float>(y), static_cast<float>(kItem3DW), static_cast<float>(kItem3DH),
            m_Listings[i]);
    }

    if (m_iFeaturedCount > 0 && CanRenderItem(m_Featured[m_iFeaturedSlide].ItemCode))
    {
        const Listing& featured = m_Featured[m_iFeaturedSlide];
        RenderMarketplaceItem(static_cast<float>(m_Pos.x + kFeatX), static_cast<float>(m_Pos.y + kFeatY),
            static_cast<float>(kFeatW), static_cast<float>(kFeatH), featured);
    }

    UpdateMousePositionn();
    RestoreCameraPerspective();
    GlobalUBO::Instance().SetProj(s_PreProj);
    GlobalUBO::Instance().SetView(s_PreView);
    BeginBitmap();
}

void CNewUIMarketplace::RenderListDialog()
{
    const int dlgX = m_Pos.x + (kBackW - kDlgW) / 2;
    const int dlgY = m_Pos.y + 150;
    EnableAlphaTest();
    glColor4f(0.08f, 0.08f, 0.08f, 0.94f);
    RenderColor(static_cast<float>(dlgX), static_cast<float>(dlgY), static_cast<float>(kDlgW), static_cast<float>(kDlgH));
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderImage(IMAGE_MP_ITEMBOX, static_cast<float>(dlgX + 24), static_cast<float>(dlgY + 52), static_cast<float>(kBoxSize), static_cast<float>(kBoxSize));

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(dlgX, dlgY + 8, I18N::Game::MarketplaceAnnounce, kDlgW, 0, RT3_SORT_CENTER);

    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->RenderText(dlgX + 170, dlgY + 40, I18N::Game::ItemName, 140, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 170, dlgY + 62, I18N::Game::MarketplaceCategories, 140, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 170, dlgY + 102, I18N::Game::MarketplacePrice, 70, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 250, dlgY + 102, I18N::Game::MarketplaceQty, 70, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 20, dlgY + 148, I18N::Game::MarketplaceDuration, 80, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 20, dlgY + 176, I18N::Game::MarketplaceFeeNotice, 140, 0, RT3_SORT_LEFT);

    wchar_t szName[64] = L"";
    if (m_iSelectedInv >= 0 && m_iSelectedInv < m_iInvCount)
        GetItemName(m_InvTypes[m_iSelectedInv], m_InvLevels[m_iSelectedInv], szName);

    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText(dlgX + 170, dlgY + 52, szName[0] ? szName : I18N::Game::MarketplaceSelectInv, 150, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 170, dlgY + 78, CategoryName(m_iDialogCategory), 140, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 170, dlgY + 118, m_szPrice[0] ? m_szPrice : L"0", 70, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(dlgX + 250, dlgY + 118, m_szQty, 70, 0, RT3_SORT_LEFT);

    wchar_t szDays[32];
    mu_swprintf(szDays, I18N::Game::MarketplaceDays, m_iDialogDays);
    g_pRenderText->RenderText(dlgX + 90, dlgY + 148, szDays, 80, 0, RT3_SORT_LEFT);

    m_DialogCancel.Render();
    m_DialogOk.Render();
}

void CNewUIMarketplace::RequestPage()
{
    SendListRequest();
}
