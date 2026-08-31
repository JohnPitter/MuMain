// MultiLanguage.cpp: implementation of the CMultiLanguage class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <cstring>

CMultiLanguage* CMultiLanguage::ms_Singleton = NULL;

CMultiLanguage::CMultiLanguage(std::wstring strSelectedML)
{
    ms_Singleton = this;

    if (wcsicmp(strSelectedML.c_str(), L"ENG") == 0)
    {
        byLanguage = 0;
    }
    else if (wcsicmp(strSelectedML.c_str(), L"POR") == 0)
    {
        byLanguage = 1;
    }
    else if (wcsicmp(strSelectedML.c_str(), L"SPN") == 0)
    {
        byLanguage = 2;
    }
    else
    {
        byLanguage = 0;
    }
}

BYTE CMultiLanguage::GetLanguage()
{
    return byLanguage;
}

/**
 * Converts a game text buffer to UTF-16.
 *
 * Local .bmd/.txt files are a mix of UTF-8 (newer) and Windows-1252 (original
 * POR/SPN client data). Network strings from OpenMU are UTF-8. Try UTF-8
 * strictly first; if that fails, decode as Windows-1252.
 *
 * Reads up to the first null, or @p maxSourceLength bytes when the source is a
 * fixed-size field. Always writes a terminating null when @p target is valid.
 *
 * @param maxTargetChars Destination capacity in wchar_t including the null.
 *        0 means the caller guaranteed enough room for the converted string.
 */
int32_t CMultiLanguage::ConvertFromUtf8(wchar_t* target, const char* source, int maxSourceLength)
{
    return ConvertFromUtf8(target, source, maxSourceLength, 0);
}

int32_t CMultiLanguage::ConvertFromUtf8(wchar_t* target, const char* source, int maxSourceLength, int maxTargetChars)
{
    if (target == nullptr || source == nullptr)
        return 0;

    if (maxTargetChars == 1)
    {
        target[0] = L'\0';
        return 0;
    }

    int srcBytes = 0;
    if (maxSourceLength < 0)
    {
        srcBytes = static_cast<int>(strlen(source));
    }
    else
    {
        while (srcBytes < maxSourceLength && source[srcBytes] != '\0')
            ++srcBytes;
    }

    if (srcBytes == 0)
    {
        target[0] = L'\0';
        return 0;
    }

    auto tryConvert = [&](UINT codePage, DWORD flags) -> int
    {
        const int destCap = (maxTargetChars > 0) ? (maxTargetChars - 1) : 0;
        if (destCap > 0)
        {
            const int written = MultiByteToWideChar(codePage, flags, source, srcBytes, target, destCap);
            if (written <= 0)
                return 0;
            target[written] = L'\0';
            return written;
        }

        const int needed = MultiByteToWideChar(codePage, flags, source, srcBytes, nullptr, 0);
        if (needed <= 0)
            return 0;
        const int written = MultiByteToWideChar(codePage, flags, source, srcBytes, target, needed);
        if (written <= 0)
            return 0;
        target[written] = L'\0';
        return written;
    };

    int written = tryConvert(CP_UTF8, MB_ERR_INVALID_CHARS);
    bool bad = written <= 0;
    if (!bad)
    {
        for (int i = 0; i < written; ++i)
        {
            if (target[i] == 0xFFFD)
            {
                bad = true;
                break;
            }
        }
    }
    if (bad)
        written = tryConvert(1252, 0);

    if (written <= 0)
    {
        target[0] = L'\0';
        return 0;
    }
    if (maxTargetChars > 0)
        target[maxTargetChars - 1] = L'\0';
    return written;
}

int32_t CMultiLanguage::ConvertFromUtf8OrAnsi(wchar_t* target, const char* source, int maxSourceLength, int maxTargetChars)
{
    return ConvertFromUtf8(target, source, maxSourceLength, maxTargetChars);
}

int32_t CMultiLanguage::ConvertToUtf8(char* target, const wchar_t* source, int maxSourceLength)
{
    if (target == nullptr || source == nullptr)
    {
        return 0;
    }

    // In this codebase, maxSourceLength is effectively used as the destination buffer capacity.
    const int requiredBytesWithNull = WideCharToMultiByte(CP_UTF8, 0, source, -1, nullptr, 0, nullptr, nullptr);
    if (requiredBytesWithNull <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    if (maxSourceLength > 0)
    {
        std::string tmp;
        tmp.resize(requiredBytesWithNull);
        const int writtenWithNull = WideCharToMultiByte(CP_UTF8, 0, source, -1, tmp.data(), requiredBytesWithNull, nullptr, nullptr);
        if (writtenWithNull <= 0)
        {
            target[0] = '\0';
            return 0;
        }

        const int available = std::max<int>(0, maxSourceLength - 1);
        const int srcLen = std::max<int>(0, writtenWithNull - 1);
        const int copyLen = std::min<int>(srcLen, available);

        if (copyLen > 0)
        {
            std::memcpy(target, tmp.data(), static_cast<size_t>(copyLen));
        }
        target[copyLen] = '\0';
        return copyLen;
    }

    const int requiredBytes = requiredBytesWithNull;
    if (requiredBytes <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    const int written = WideCharToMultiByte(CP_UTF8, 0, source, -1, target, requiredBytes, nullptr, nullptr);
    if (written <= 0)
    {
        target[0] = '\0';
        return 0;
    }

    // When source length is -1, WinAPI includes the null terminator in 'written'.
    return written > 0 ? (written - 1) : 0;
}


WPARAM CMultiLanguage::ConvertFulltoHalfWidthChar(DWORD wParam)
{
    auto Char = (wchar_t)(wParam);

    if (Char >= 0xFF01 && Char <= 0xFF5A)
        wParam -= 0xFEE0;
    else if (Char == 0x3000)
        wParam = 0x0020;

    return wParam;
}