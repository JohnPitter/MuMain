// MsgBoxIGSSendGift.h: interface for the CMsgBoxIGSSendGift class.
//////////////////////////////////////////////////////////////////////

#pragma once
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Options/NewUIOptionWindow.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"

using namespace SEASON3B;

class CMsgBoxIGSSendGift : public CNewUIMessageBoxBase
{
public:
    enum IMAGE_IGS_SEND_GIFT
    {
        IMAGE_IGS_BUTTON = BITMAP_IGS_MSGBOX_BUTTON,
        IMAGE_IGS_BACK = CNewUIOptionWindow::IMAGE_OPTION_FRAME_BACK,
        IMAGE_IGS_UP = CNewUIOptionWindow::IMAGE_OPTION_FRAME_UP,
        IMAGE_IGS_DOWN = CNewUIOptionWindow::IMAGE_OPTION_FRAME_DOWN,
        IMAGE_IGS_LEFTLINE = CNewUIOptionWindow::IMAGE_OPTION_FRAME_LEFT,
        IMAGE_IGS_RIGHTLINE = CNewUIOptionWindow::IMAGE_OPTION_FRAME_RIGHT,
        IMAGE_IGS_TEXTBOX = BITMAP_IGS_MGSBOX_BUY_CONFIRM_TEXT_BOX,
    };

    enum IMAGESIZE_IGS_SEND_GIFT
    {
        IMAGE_IGS_WINDOW_WIDTH = 640,
        IMAGE_IGS_WINDOW_HEIGHT = 429,
        IMAGE_IGS_FRAME_WIDTH = 190,
        IMAGE_IGS_GIFT_FRAME_HEIGHT = 319,
        IMAGE_IGS_TRANSFER_FRAME_HEIGHT = 179,
        IMAGE_IGS_UP_HEIGHT = 64,
        IMAGE_IGS_DOWN_HEIGHT = 45,
        IMAGE_IGS_LINE_WIDTH = 21,
        IMAGE_IGS_LINE_HEIGHT = 10,
        IMAGE_IGS_BTN_WIDTH = 52,
        IMAGE_IGS_BTN_HEIGHT = 26,
        IMAGE_IGS_INPUT_WIDTH = 76,
        IMAGE_IGS_INPUT_HEIGHT = 17,
        IMAGE_IGS_MESSAGE_WIDTH = 160,
        IMAGE_IGS_MESSAGE_HEIGHT = 65,
        IGS_MESSAGE_BOX_LINE_HEIGHT = 50,
        IGS_MAX_CASH_AMOUNT_SIZE = 8,
        IGS_INPUT_INSET_X = 3,
        IGS_INPUT_INSET_Y = 2,
        IGS_GIFT_MIDDLE_COUNT = 21,
        IGS_TRANSFER_MIDDLE_COUNT = 7,
    };

    enum IGS_SEND_GIFT_POS
    {
        IGS_CONTENT_POS_X = 15,
        IGS_CONTENT_POS_Y = IMAGE_IGS_UP_HEIGHT,
        IGS_CONTENT_WIDTH = IMAGE_IGS_FRAME_WIDTH - (2 * IGS_CONTENT_POS_X),
        IGS_CONTENT_GIFT_HEIGHT = IMAGE_IGS_GIFT_FRAME_HEIGHT - IMAGE_IGS_UP_HEIGHT - IMAGE_IGS_DOWN_HEIGHT,
        IGS_CONTENT_TRANSFER_HEIGHT = IMAGE_IGS_TRANSFER_FRAME_HEIGHT - IMAGE_IGS_UP_HEIGHT - IMAGE_IGS_DOWN_HEIGHT,
        IGS_ID_BOX_POS_X = 98,
        IGS_ID_BOX_POS_Y = 125,
        IGS_TRANSFER_ID_BOX_POS_Y = 72,
        IGS_ID_TEXT_POS_X = 18,
        IGS_ID_TEXT_POS_Y = 130,
        IGS_AMOUNT_BOX_POS_X = 98,
        IGS_AMOUNT_BOX_POS_Y = 102,
        IGS_AMOUNT_TEXT_POS_X = 18,
        IGS_AMOUNT_TEXT_POS_Y = 107,
        IGS_MESSAGE_BOX_POS_X = 15,
        IGS_MESSAGE_BOX_POS_Y = 180,
        IGS_BTN_OK_POS_X = 35,
        IGS_BTN_CANCEL_POS_X = 105,
        IGS_GIFT_BTN_POS_Y = 275,
        IGS_TRANSFER_BTN_POS_Y = 140,
        IGS_TEXT_TITLE_POS_Y = 10,
        IGS_TEXT_ITEM_INFO_POS_X = 18,
        IGS_TEXT_ITEM_INFO_NAME_POS_Y = 72,
        IGS_TEXT_ITEM_INFO_PRICE_POS_Y = 84,
        IGS_TEXT_ITEM_INFO_PERIOD_POS_Y = 96,
        IGS_TEXT_ITEM_INFO_WIDTH = IGS_CONTENT_WIDTH,
        IGS_TEXT_ID_TITLE_POS_X = 18,
        IGS_TEXT_ID_TITLE_POS_Y = 130,
        IGS_TRANSFER_ID_TITLE_POS_Y = 77,
        IGS_TEXT_ID_TITLE_WIDTH = 78,
        IGS_TEXT_MESSAGE_TITLE_POS_X = 18,
        IGS_TEXT_MESSAGE_TITLE_POS_Y = 160,
        IGS_TRANSFER_AMOUNT_TITLE_POS_Y = 107,
        IGS_TEXT_MESSAGE_TITLE_WIDTH = 78,
        IGS_TEXT_NOTICE_POS_Y = 255,
        IGS_TEXT_NOTICE_WIDTH = IGS_CONTENT_WIDTH,
    };

public:
    CMsgBoxIGSSendGift();
    ~CMsgBoxIGSSendGift();

    bool Create(float fPriority = 3.f);
    void Release();
    bool Update();
    bool Render();

    void Initialize(int iPackageSeq, int iDisplaySeq, int iPriceSeq, DWORD wItemCode, int iCashType, wchar_t* pszName, wchar_t* pszPrice, wchar_t* pszPeriod);
    void InitializeCashTransfer();

    static CALLBACK_RESULT LButtonUp(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam);
    static CALLBACK_RESULT OKButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam);
    static CALLBACK_RESULT CancelButtonDown(class CNewUIMessageBoxBase* pOwner, const leaf::xstreambuf& xParam);

private:
    void SetAddCallbackFunc();
    void SetButtonInfo();
    void RenderFrame();
    void RenderInputBoxes();
    void RenderTexts();
    void RenderButtons();
    void ChangeInputBoxFocus();
    bool LoadImages();
    void UnloadImages();
    void InitInputBox();
    void ConfigureInputBoxes();
    void ApplyInputBoxStyle(CUITextInputBox& box);
    void RenderInputBoxFrame(int x, int y, int width, int height);
    void RenderFieldText(int x, int y, int width, const wchar_t* pszText, bool bCaret);

private:
    CNewUIMessageBoxButton m_BtnOk;
    CNewUIMessageBoxButton m_BtnCancel;
    CUITextInputBox m_IDInputBox;
    CUITextInputBox m_MessageInputBox;
    int m_iPackageSeq;
    int m_iDisplaySeq;
    int m_iPriceSeq;
    DWORD m_wItemCode;
    int m_iCashType;
    wchar_t m_szID[MAX_USERNAME_SIZE + 1];
    wchar_t m_szMessage[MAX_GIFT_MESSAGE_SIZE + 1];
    wchar_t m_szName[MAX_TEXT_LENGTH];
    wchar_t m_szPrice[MAX_TEXT_LENGTH];
    wchar_t m_szPeriod[MAX_TEXT_LENGTH];
    wchar_t m_szNotice[NUM_LINE_CMB][MAX_TEXT_LENGTH];
    int m_iNumNoticeLine;
    int m_iMsgBoxHeight;
    int m_iMiddleCount;
    bool m_bCashTransfer;
    bool m_bImagesLoaded;
    bool m_bInputBoxesInitialized;
    bool m_bReleased;
};

class CMsgBoxIGSSendGiftLayout : public TMsgBoxLayout<CMsgBoxIGSSendGift>
{
public:
    bool SetLayout();
};

#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM
