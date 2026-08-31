// MsgBoxIGSSendGift.cpp: implementation of the CMsgBoxIGSSendGift class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"
#include "Engine/Object/ZzzCharacter.h"
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
#include "MsgBoxIGSSendGift.h"
#include "UI/NewUI/NewUICommon.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Audio/DSPlaySound.h"
#include "MsgBoxIGSCommon.h"
#include "MsgBoxIGSSendGiftConfirm.h"

CMsgBoxIGSSendGift::CMsgBoxIGSSendGift()
    : m_iPackageSeq(0), m_iDisplaySeq(0), m_iPriceSeq(0), m_wItemCode(-1), m_iCashType(0),
      m_iNumNoticeLine(0), m_iMsgBoxHeight(IMAGE_IGS_GIFT_FRAME_HEIGHT), m_iMiddleCount(IGS_GIFT_MIDDLE_COUNT),
      m_bCashTransfer(false), m_bImagesLoaded(false), m_bInputBoxesInitialized(false), m_bReleased(false)
{
    m_szID[0] = L'\0';
    m_szMessage[0] = L'\0';
    m_szName[0] = L'\0';
    m_szPrice[0] = L'\0';
    m_szPeriod[0] = L'\0';
    for (int i = 0; i < NUM_LINE_CMB; ++i)
        m_szNotice[i][0] = L'\0';
}

CMsgBoxIGSSendGift::~CMsgBoxIGSSendGift()
{
    Release();
}

bool CMsgBoxIGSSendGift::Create(float fPriority)
{
    m_bReleased = false;
    if (!LoadImages())
        return false;
    SetAddCallbackFunc();
    if (!CNewUIMessageBoxBase::Create((IMAGE_IGS_WINDOW_WIDTH / 2) - (IMAGE_IGS_FRAME_WIDTH / 2),
        (IMAGE_IGS_WINDOW_HEIGHT / 2) - (m_iMsgBoxHeight / 2), IMAGE_IGS_FRAME_WIDTH, m_iMsgBoxHeight, fPriority))
    {
        UnloadImages();
        return false;
    }
    SetButtonInfo();
    InitInputBox();
    SetMsgBackOpacity();
    return true;
}

void CMsgBoxIGSSendGift::ApplyInputBoxStyle(CUITextInputBox& box)
{
    box.SetTextColor(255, 255, 255, 255);
    box.SetBackColor(0, 0, 0, 0);
    box.SetFont(g_hFont);
    box.SetState(UISTATE_NORMAL);
}

void CMsgBoxIGSSendGift::InitInputBox()
{
    m_IDInputBox.Init(g_hWnd, IMAGE_IGS_INPUT_WIDTH - (2 * IGS_INPUT_INSET_X),
        IMAGE_IGS_INPUT_HEIGHT - (2 * IGS_INPUT_INSET_Y), MAX_USERNAME_SIZE, false);
    ApplyInputBoxStyle(m_IDInputBox);

    m_MessageInputBox.SetMultiline(TRUE);
    m_MessageInputBox.Init(g_hWnd, IMAGE_IGS_MESSAGE_WIDTH - (2 * IGS_INPUT_INSET_X),
        IMAGE_IGS_MESSAGE_HEIGHT - (2 * IGS_INPUT_INSET_Y), IGS_MESSAGE_BOX_LINE_HEIGHT, false);
    ApplyInputBoxStyle(m_MessageInputBox);
    m_MessageInputBox.SetUseScrollbar(FALSE);
    m_MessageInputBox.SetTextLimit(MAX_GIFT_MESSAGE_SIZE);
    ConfigureInputBoxes();
    m_bInputBoxesInitialized = true;
    m_IDInputBox.GiveFocus();
}

void CMsgBoxIGSSendGift::Initialize(int iPackageSeq, int iDisplaySeq, int iPriceSeq, DWORD wItemCode, int iCashType, wchar_t* pszName, wchar_t* pszPrice, wchar_t* pszPeriod)
{
    m_bCashTransfer = false;
    m_iPackageSeq = iPackageSeq;
    m_iDisplaySeq = iDisplaySeq;
    m_iPriceSeq = iPriceSeq;
    m_wItemCode = wItemCode;
    m_iCashType = iCashType;
    m_iMsgBoxHeight = IMAGE_IGS_GIFT_FRAME_HEIGHT;
    m_iMiddleCount = IGS_GIFT_MIDDLE_COUNT;
    SetSize(IMAGE_IGS_FRAME_WIDTH, m_iMsgBoxHeight);
    SetPos((IMAGE_IGS_WINDOW_WIDTH / 2) - (IMAGE_IGS_FRAME_WIDTH / 2),
        (IMAGE_IGS_WINDOW_HEIGHT / 2) - (m_iMsgBoxHeight / 2));
    SetButtonInfo();
    ConfigureInputBoxes();

    mu_swprintf_s(m_szName, I18N::Game::ItemS, pszName);
    mu_swprintf_s(m_szPrice, I18N::Game::PriceS, pszPrice);
    mu_swprintf_s(m_szPeriod, I18N::Game::DurationS, pszPeriod);
    m_iNumNoticeLine = ::DivideStringByPixel(&m_szNotice[0][0], NUM_LINE_CMB, MAX_TEXT_LENGTH,
        I18N::Game::GiftedItemsCannotBeReturnedDeliverTheGiftS, IGS_TEXT_NOTICE_WIDTH);
}

void CMsgBoxIGSSendGift::InitializeCashTransfer()
{
    m_bCashTransfer = true;
    m_iMsgBoxHeight = IMAGE_IGS_TRANSFER_FRAME_HEIGHT;
    m_iMiddleCount = IGS_TRANSFER_MIDDLE_COUNT;
    SetSize(IMAGE_IGS_FRAME_WIDTH, m_iMsgBoxHeight);
    SetPos((IMAGE_IGS_WINDOW_WIDTH / 2) - (IMAGE_IGS_FRAME_WIDTH / 2),
        (IMAGE_IGS_WINDOW_HEIGHT / 2) - (m_iMsgBoxHeight / 2));
    SetButtonInfo();
    m_MessageInputBox.SetMultiline(FALSE);
    m_MessageInputBox.Init(g_hWnd, IMAGE_IGS_INPUT_WIDTH - (2 * IGS_INPUT_INSET_X),
        IMAGE_IGS_INPUT_HEIGHT - (2 * IGS_INPUT_INSET_Y), IGS_MAX_CASH_AMOUNT_SIZE, false);
    ApplyInputBoxStyle(m_MessageInputBox);
    m_MessageInputBox.SetOption(UIOPTION_NUMBERONLY);
    m_MessageInputBox.SetUseScrollbar(FALSE);
    m_MessageInputBox.SetTextLimit(IGS_MAX_CASH_AMOUNT_SIZE);
    ConfigureInputBoxes();
    m_bInputBoxesInitialized = true;
    m_szName[0] = L'\0';
    m_szPrice[0] = L'\0';
    m_szPeriod[0] = L'\0';
    m_iNumNoticeLine = 0;
    m_MessageInputBox.GiveFocus();
}

void CMsgBoxIGSSendGift::ConfigureInputBoxes()
{
    const int idBoxY = m_bCashTransfer ? IGS_TRANSFER_ID_BOX_POS_Y : IGS_ID_BOX_POS_Y;
    m_IDInputBox.SetPosition(GetPos().x + IGS_ID_BOX_POS_X + IGS_INPUT_INSET_X,
        GetPos().y + idBoxY + IGS_INPUT_INSET_Y);
    const int boxX = m_bCashTransfer ? IGS_AMOUNT_BOX_POS_X : IGS_MESSAGE_BOX_POS_X;
    const int boxY = m_bCashTransfer ? IGS_AMOUNT_BOX_POS_Y : IGS_MESSAGE_BOX_POS_Y;
    m_MessageInputBox.SetPosition(GetPos().x + boxX + IGS_INPUT_INSET_X,
        GetPos().y + boxY + IGS_INPUT_INSET_Y);
}

void CMsgBoxIGSSendGift::Release()
{
    if (m_bReleased)
        return;
    m_bReleased = true;

    CUITextInputBox::ReleaseFocus();
    if (m_bInputBoxesInitialized)
    {
        m_IDInputBox.SetState(UISTATE_HIDE);
        m_MessageInputBox.SetState(UISTATE_HIDE);
        m_bInputBoxesInitialized = false;
    }
    CNewUIMessageBoxBase::Release();
    UnloadImages();
}

bool CMsgBoxIGSSendGift::Update()
{
    m_BtnOk.Update();
    m_BtnCancel.Update();
    m_IDInputBox.DoAction();
    m_MessageInputBox.DoAction();
    m_IDInputBox.GetText(m_szID, MAX_USERNAME_SIZE + 1);
    m_MessageInputBox.GetText(m_szMessage, MAX_GIFT_MESSAGE_SIZE + 1);
    if (SEASON3B::IsPress(VK_TAB) == true)
        ChangeInputBoxFocus();
    return true;
}

bool CMsgBoxIGSSendGift::Render()
{
    EnableAlphaTest();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RenderMsgBackColor(true);
    EnableAlphaTest();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RenderFrame();
    RenderInputBoxes();
    RenderTexts();
    RenderButtons();
    DisableAlphaBlend();
    return true;
}

void CMsgBoxIGSSendGift::SetAddCallbackFunc()
{
    AddCallbackFunc(CMsgBoxIGSSendGift::LButtonUp, MSGBOX_EVENT_MOUSE_LBUTTON_UP);
    AddCallbackFunc(CMsgBoxIGSSendGift::OKButtonDown, MSGBOX_EVENT_USER_COMMON_OK);
    AddCallbackFunc(CMsgBoxIGSSendGift::CancelButtonDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
}

CALLBACK_RESULT CMsgBoxIGSSendGift::LButtonUp(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSSendGift*>(pOwner);
    if (pOwnMsgBox == nullptr)
        return CALLBACK_CONTINUE;
    if (pOwnMsgBox->m_BtnOk.IsMouseIn() == true)
    {
        g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_USER_COMMON_OK);
        return CALLBACK_BREAK;
    }
    if (pOwnMsgBox->m_BtnCancel.IsMouseIn() == true)
    {
        g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_USER_COMMON_CANCEL);
        return CALLBACK_BREAK;
    }
    return CALLBACK_CONTINUE;
}

CALLBACK_RESULT CMsgBoxIGSSendGift::OKButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
{
    auto* pOwnMsgBox = dynamic_cast<CMsgBoxIGSSendGift*>(pOwner);
    if (pOwnMsgBox == nullptr || Hero == nullptr || SocketClient == nullptr || SocketClient->ToGameServer() == nullptr)
        return CALLBACK_CONTINUE;
    if (pOwnMsgBox->m_bCashTransfer)
    {
        const int amount = _wtoi(pOwnMsgBox->m_szMessage);
        if (pOwnMsgBox->m_szID[0] == L'\0' || amount <= 0)
        {
            CMsgBoxIGSCommon* pMsgBox = NULL;
            if (CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox) && pMsgBox != nullptr)
                pMsgBox->Initialize(I18N::Game::Error, I18N::Game::CashShopTransferInvalidInput);
        }
        else
        {
            wchar_t command[MAX_CHAT_SIZE + 1] = {};
            mu_swprintf_s(command, L"/sendcash %ls %d", pOwnMsgBox->m_szID, amount);
            SocketClient->ToGameServer()->SendPublicChatMessage(Hero->ID, command);
            SocketClient->ToGameServer()->SendCashShopPointInfoRequest();
            g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
        }
        return CALLBACK_BREAK;
    }
    if (pOwnMsgBox->m_szID[0] == L'\0')
    {
        CMsgBoxIGSCommon* pMsgBox = NULL;
        if (CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox) && pMsgBox != nullptr)
            pMsgBox->Initialize(I18N::Game::Error, I18N::Game::GiftRecipientSIDIsMissing);
    }
    else if (wcscmp(pOwnMsgBox->m_szID, Hero->ID) == 0)
    {
        CMsgBoxIGSCommon* pMsgBox = NULL;
        if (CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSCommonLayout), &pMsgBox) && pMsgBox != nullptr)
            pMsgBox->Initialize(I18N::Game::Error, I18N::Game::YouCannotSendAGiftToYourself);
    }
    else
    {
        CMsgBoxIGSSendGiftConfirm* pMsgBox = NULL;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CMsgBoxIGSSendGiftConfirmLayout), &pMsgBox);
        pMsgBox->Initialize(pOwnMsgBox->m_iPackageSeq, pOwnMsgBox->m_iDisplaySeq, pOwnMsgBox->m_iPriceSeq,
            pOwnMsgBox->m_wItemCode, pOwnMsgBox->m_iCashType, pOwnMsgBox->m_szID, pOwnMsgBox->m_szMessage,
            pOwnMsgBox->m_szName, pOwnMsgBox->m_szPrice, pOwnMsgBox->m_szPeriod);
    }
    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
    return CALLBACK_BREAK;
}

CALLBACK_RESULT CMsgBoxIGSSendGift::CancelButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf&)
{
    PlayBuffer(SOUND_CLICK01);
    g_MessageBox->SendEvent(pOwner, MSGBOX_EVENT_DESTROY);
    return CALLBACK_BREAK;
}

void CMsgBoxIGSSendGift::SetButtonInfo()
{
    const int buttonY = m_bCashTransfer ? IGS_TRANSFER_BTN_POS_Y : IGS_GIFT_BTN_POS_Y;
    m_BtnOk.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + IGS_BTN_OK_POS_X, GetPos().y + buttonY,
        IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT, CNewUIMessageBoxButton::MSGBOX_BTN_CUSTOM, true);
    m_BtnOk.MoveTextPos(0, -1);
    m_BtnOk.SetText(m_bCashTransfer ? I18N::Game::OK : I18N::Game::InGameShopGiftConfirm);
    m_BtnCancel.SetInfo(IMAGE_IGS_BUTTON, GetPos().x + IGS_BTN_CANCEL_POS_X, GetPos().y + buttonY,
        IMAGE_IGS_BTN_WIDTH, IMAGE_IGS_BTN_HEIGHT, CNewUIMessageBoxButton::MSGBOX_BTN_CUSTOM, true);
    m_BtnCancel.MoveTextPos(0, -1);
    m_BtnCancel.SetText(m_bCashTransfer ? I18N::Game::Cancel : I18N::Game::InGameShopGiftCancel);
}

void CMsgBoxIGSSendGift::RenderFrame()
{
    int iY = GetPos().y;
    RenderImage(IMAGE_IGS_BACK, GetPos().x, iY, IMAGE_IGS_FRAME_WIDTH, m_iMsgBoxHeight);
    RenderImage(IMAGE_IGS_UP, GetPos().x, iY, IMAGE_IGS_FRAME_WIDTH, IMAGE_IGS_UP_HEIGHT);
    iY += IMAGE_IGS_UP_HEIGHT;
    for (int i = 0; i < m_iMiddleCount; ++i)
    {
        RenderImage(IMAGE_IGS_LEFTLINE, GetPos().x, iY, IMAGE_IGS_LINE_WIDTH, IMAGE_IGS_LINE_HEIGHT);
        RenderImage(IMAGE_IGS_RIGHTLINE, GetPos().x + IMAGE_IGS_FRAME_WIDTH - IMAGE_IGS_LINE_WIDTH, iY,
            IMAGE_IGS_LINE_WIDTH, IMAGE_IGS_LINE_HEIGHT);
        iY += IMAGE_IGS_LINE_HEIGHT;
    }
    RenderImage(IMAGE_IGS_DOWN, GetPos().x, iY, IMAGE_IGS_FRAME_WIDTH, IMAGE_IGS_DOWN_HEIGHT);
}

void CMsgBoxIGSSendGift::RenderInputBoxFrame(int x, int y, int width, int height)
{
    EnableAlphaTest();
    glColor4f(0.f, 0.f, 0.f, 0.35f);
    RenderColor(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    EndRenderColor();
    glColor4ub(176, 176, 176, 255);
    RenderColor(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 1.f);
    RenderColor(static_cast<float>(x), static_cast<float>(y + height - 1), static_cast<float>(width), 1.f);
    RenderColor(static_cast<float>(x), static_cast<float>(y), 1.f, static_cast<float>(height));
    RenderColor(static_cast<float>(x + width - 1), static_cast<float>(y), 1.f, static_cast<float>(height));
    EndRenderColor();
    glColor4f(1.f, 1.f, 1.f, 1.f);
}

void CMsgBoxIGSSendGift::RenderFieldText(int x, int y, int width, const wchar_t* pszText, bool bCaret)
{
    glColor4f(1.f, 1.f, 1.f, 1.f);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    SIZE textSize = {};
    g_pRenderText->RenderText(x, y, pszText != nullptr ? pszText : L"", width, 0, RT3_SORT_LEFT, &textSize);
    if (!bCaret)
        return;
    if ((static_cast<int>(WorldTime) / 250) % 2 != 0)
        return;
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderColor(static_cast<float>(x + textSize.cx), static_cast<float>(y), 1.f, 10.f);
    EndRenderColor();
}

void CMsgBoxIGSSendGift::RenderInputBoxes()
{
    const int idBoxY = m_bCashTransfer ? IGS_TRANSFER_ID_BOX_POS_Y : IGS_ID_BOX_POS_Y;
    RenderInputBoxFrame(GetPos().x + IGS_ID_BOX_POS_X, GetPos().y + idBoxY,
        IMAGE_IGS_INPUT_WIDTH, IMAGE_IGS_INPUT_HEIGHT);
    if (m_bCashTransfer)
    {
        RenderInputBoxFrame(GetPos().x + IGS_AMOUNT_BOX_POS_X, GetPos().y + IGS_AMOUNT_BOX_POS_Y,
            IMAGE_IGS_INPUT_WIDTH, IMAGE_IGS_INPUT_HEIGHT);
        return;
    }
    RenderInputBoxFrame(GetPos().x + IGS_MESSAGE_BOX_POS_X, GetPos().y + IGS_MESSAGE_BOX_POS_Y,
        IMAGE_IGS_MESSAGE_WIDTH, IMAGE_IGS_MESSAGE_HEIGHT);
}

void CMsgBoxIGSSendGift::RenderTexts()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->RenderText(GetPos().x, GetPos().y + IGS_TEXT_TITLE_POS_Y,
        m_bCashTransfer ? I18N::Game::CashShopTransferTitle : I18N::Game::InGameShopGiftTitle,
        IMAGE_IGS_FRAME_WIDTH, 0, RT3_SORT_CENTER);
    g_pRenderText->SetFont(g_hFont);
    const int idTitleY = m_bCashTransfer ? IGS_TRANSFER_ID_TITLE_POS_Y : IGS_TEXT_ID_TITLE_POS_Y;
    const int secondTitleY = m_bCashTransfer ? IGS_TRANSFER_AMOUNT_TITLE_POS_Y : IGS_TEXT_MESSAGE_TITLE_POS_Y;
    g_pRenderText->RenderText(GetPos().x + IGS_TEXT_ID_TITLE_POS_X, GetPos().y + idTitleY,
        m_bCashTransfer ? I18N::Game::CashShopTransferTargetLabel : I18N::Game::InGameShopGiftRecipient,
        IGS_TEXT_ID_TITLE_WIDTH, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(GetPos().x + IGS_TEXT_MESSAGE_TITLE_POS_X, GetPos().y + secondTitleY,
        m_bCashTransfer ? I18N::Game::CashShopTransferAmountLabel : I18N::Game::InGameShopGiftMessage,
        IGS_TEXT_MESSAGE_TITLE_WIDTH, 0, RT3_SORT_LEFT);

    const int idBoxY = m_bCashTransfer ? IGS_TRANSFER_ID_BOX_POS_Y : IGS_ID_BOX_POS_Y;
    const int idTextX = GetPos().x + IGS_ID_BOX_POS_X + IGS_INPUT_INSET_X;
    const int idTextY = GetPos().y + idBoxY + IGS_INPUT_INSET_Y;
    RenderFieldText(idTextX, idTextY, IMAGE_IGS_INPUT_WIDTH - (2 * IGS_INPUT_INSET_X),
        m_szID, m_IDInputBox.HaveFocus() == TRUE);
    if (m_bCashTransfer)
    {
        RenderFieldText(GetPos().x + IGS_AMOUNT_BOX_POS_X + IGS_INPUT_INSET_X,
            GetPos().y + IGS_AMOUNT_BOX_POS_Y + IGS_INPUT_INSET_Y,
            IMAGE_IGS_INPUT_WIDTH - (2 * IGS_INPUT_INSET_X),
            m_szMessage, m_MessageInputBox.HaveFocus() == TRUE);
        return;
    }
    RenderFieldText(GetPos().x + IGS_MESSAGE_BOX_POS_X + IGS_INPUT_INSET_X,
        GetPos().y + IGS_MESSAGE_BOX_POS_Y + IGS_INPUT_INSET_Y,
        IMAGE_IGS_MESSAGE_WIDTH - (2 * IGS_INPUT_INSET_X),
        m_szMessage, m_MessageInputBox.HaveFocus() == TRUE);
    g_pRenderText->SetTextColor(247, 186, 0, 255);
    g_pRenderText->RenderText(GetPos().x + IGS_TEXT_ITEM_INFO_POS_X, GetPos().y + IGS_TEXT_ITEM_INFO_NAME_POS_Y,
        m_szName, IGS_TEXT_ITEM_INFO_WIDTH, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(GetPos().x + IGS_TEXT_ITEM_INFO_POS_X, GetPos().y + IGS_TEXT_ITEM_INFO_PRICE_POS_Y,
        m_szPrice, IGS_TEXT_ITEM_INFO_WIDTH, 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText(GetPos().x + IGS_TEXT_ITEM_INFO_POS_X, GetPos().y + IGS_TEXT_ITEM_INFO_PERIOD_POS_Y,
        m_szPeriod, IGS_TEXT_ITEM_INFO_WIDTH, 0, RT3_SORT_LEFT);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    for (int i = 0; i < m_iNumNoticeLine; ++i)
        g_pRenderText->RenderText(GetPos().x + IGS_CONTENT_POS_X, GetPos().y + IGS_TEXT_NOTICE_POS_Y + i * 10,
            m_szNotice[i], IGS_TEXT_NOTICE_WIDTH, 0, RT3_SORT_CENTER);
}

void CMsgBoxIGSSendGift::RenderButtons()
{
    m_BtnOk.Render();
    m_BtnCancel.Render();
}

void CMsgBoxIGSSendGift::ChangeInputBoxFocus()
{
    if (m_IDInputBox.HaveFocus() == TRUE)
        m_MessageInputBox.GiveFocus();
    else
        m_IDInputBox.GiveFocus();
}

bool CMsgBoxIGSSendGift::LoadImages()
{
    if (m_bImagesLoaded)
        return true;
    m_bImagesLoaded = true;
    if (!LoadBitmap(L"Interface\\InGameShop\\Ingame_Bt03.tga", IMAGE_IGS_BUTTON, GL_LINEAR)
        || !LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_IGS_BACK, GL_LINEAR)
        || !LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_IGS_DOWN, GL_LINEAR)
        || !LoadBitmap(L"Interface\\newui_option_top.tga", IMAGE_IGS_UP, GL_LINEAR)
        || !LoadBitmap(L"Interface\\newui_option_back06(L).tga", IMAGE_IGS_LEFTLINE, GL_LINEAR)
        || !LoadBitmap(L"Interface\\newui_option_back06(R).tga", IMAGE_IGS_RIGHTLINE, GL_LINEAR))
    {
        UnloadImages();
        return false;
    }
    m_bImagesLoaded = true;
    return true;
}

void CMsgBoxIGSSendGift::UnloadImages()
{
    if (!m_bImagesLoaded)
        return;
    DeleteBitmap(IMAGE_IGS_BUTTON);
    DeleteBitmap(IMAGE_IGS_BACK);
    DeleteBitmap(IMAGE_IGS_DOWN);
    DeleteBitmap(IMAGE_IGS_UP);
    DeleteBitmap(IMAGE_IGS_LEFTLINE);
    DeleteBitmap(IMAGE_IGS_RIGHTLINE);
    m_bImagesLoaded = false;
}

bool CMsgBoxIGSSendGiftLayout::SetLayout()
{
    CMsgBoxIGSSendGift* pMsgBox = GetMsgBox();
    if (pMsgBox == nullptr)
        return false;
    return pMsgBox->Create();
}

#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
