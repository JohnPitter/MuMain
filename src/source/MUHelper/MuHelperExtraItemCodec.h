#pragma once

// Pure UTF-8 encoder for a single "Add extra" item-name wire slot
// (PRECEIVE_MUHELPER_DATA::ExtraItems in Network/Server/WSclient.h, 15 bytes
// per slot: 14 usable + a null terminator -- see OpenMU's
// MuHelperSettingsSerializer, which decodes the same slot as a null
// terminated ASCII/UTF-8 run).
//
// ConfigDataSerDe::Serialize() (MuHelperData.cpp) used to encode with
// wcstombs(), which converts using the current C locale rather than UTF-8 --
// a mismatch with Deserialize()'s CMultiLanguage::ConvertFromUtf8(), which
// always expects UTF-8 -- and, on truncation (name too long for the 15-byte
// slot), wiped the whole slot to zero instead of keeping a shortened name.
// Any real, multi-word MU item name of 15+ encoded bytes therefore vanished
// completely on save and never came back after relogging: the checkbox that
// enables the filter ("Add extra") persisted fine, but the name list itself
// read back empty. See tests/muhelper/test_muhelper_extra_item_codec.cpp for
// the regression coverage (a wiped-to-zero slot is exactly what this encoder
// must never produce for a non-empty input).
//
// This header owns only the encode-with-safe-truncation contract, expressed
// as a hand-rolled UTF-16 (wchar_t, as MSVC represents it) to UTF-8 encoder,
// so it can be unit tested without the engine PCH that CMultiLanguage (and
// therefore MuHelperData.cpp itself) requires -- see the doc comment on
// tests/muhelper/test_muhelper_settings.cpp for the same constraint. The
// production code in MuHelperData.cpp calls this instead of wcstombs().

#include <cstddef>
#include <cstdint>

namespace MUHelper::ItemFilter
{
// Encodes `source` (a null-terminated UTF-16 string) as UTF-8 into `dest`,
// writing at most `destCapacity` bytes and always leaving `dest`
// null-terminated (a `destCapacity` of 0 writes nothing and is a no-op).
// If the full UTF-8 encoding does not fit in `destCapacity - 1` bytes, the
// output is truncated at the last complete UTF-8 sequence that fits -- it
// never emits a partial multi-byte sequence, and it never leaves `dest`
// empty when `source` has at least one encodable character and
// `destCapacity > 1`. Returns the number of bytes written, not counting the
// terminator.
inline std::size_t EncodeUtf8Truncated(char* dest, std::size_t destCapacity, const wchar_t* source)
{
    if (destCapacity == 0)
    {
        return 0;
    }

    if (dest == nullptr)
    {
        return 0;
    }

    std::size_t written = 0;
    const std::size_t budget = destCapacity - 1; // room to always null-terminate

    if (source != nullptr)
    {
        for (const wchar_t* p = source; *p != L'\0';)
        {
            std::uint32_t codepoint = static_cast<std::uint16_t>(*p);
            std::size_t unitsConsumed = 1;

            // Combine a UTF-16 surrogate pair into one code point. An
            // unpaired low surrogate (malformed input) is left as-is and
            // encoded as its raw 16-bit value, same as the high-surrogate
            // case below when no low surrogate follows.
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
            {
                const wchar_t next = *(p + 1);
                const std::uint32_t low = static_cast<std::uint16_t>(next);
                if (next != L'\0' && low >= 0xDC00 && low <= 0xDFFF)
                {
                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                    unitsConsumed = 2;
                }
            }

            // UTF-8 sequence length for this code point.
            std::size_t seqLen;
            if (codepoint <= 0x7F) seqLen = 1;
            else if (codepoint <= 0x7FF) seqLen = 2;
            else if (codepoint <= 0xFFFF) seqLen = 3;
            else seqLen = 4;

            if (written + seqLen > budget)
            {
                break; // would overflow -- stop before emitting a partial sequence
            }

            unsigned char buf[4];
            switch (seqLen)
            {
            case 1:
                buf[0] = static_cast<unsigned char>(codepoint);
                break;
            case 2:
                buf[0] = static_cast<unsigned char>(0xC0 | (codepoint >> 6));
                buf[1] = static_cast<unsigned char>(0x80 | (codepoint & 0x3F));
                break;
            case 3:
                buf[0] = static_cast<unsigned char>(0xE0 | (codepoint >> 12));
                buf[1] = static_cast<unsigned char>(0x80 | ((codepoint >> 6) & 0x3F));
                buf[2] = static_cast<unsigned char>(0x80 | (codepoint & 0x3F));
                break;
            default:
                buf[0] = static_cast<unsigned char>(0xF0 | (codepoint >> 18));
                buf[1] = static_cast<unsigned char>(0x80 | ((codepoint >> 12) & 0x3F));
                buf[2] = static_cast<unsigned char>(0x80 | ((codepoint >> 6) & 0x3F));
                buf[3] = static_cast<unsigned char>(0x80 | (codepoint & 0x3F));
                break;
            }

            for (std::size_t i = 0; i < seqLen; ++i)
            {
                dest[written++] = static_cast<char>(buf[i]);
            }

            p += unitsConsumed;
        }
    }

    dest[written] = '\0';
    return written;
}
}
