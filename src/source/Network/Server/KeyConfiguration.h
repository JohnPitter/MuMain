#ifndef _KEYCONFIGURATION_H_
#define _KEYCONFIGURATION_H_

#pragma once

#include <cstddef>
#include <cstdint>

// Layout of the Season 6 key configuration blob (C1 F3 30 in both directions).
// The server stores the 30 bytes verbatim and hands them back on enter-world,
// so the writer (SaveOptions) and the reader (ReceiveOption) must agree on
// every offset. They did not: the writer emitted the four potion levels as
// four plain bytes 26..29 (Q, W, E, R) while the reader mapped the same four
// bytes onto a little-endian `int` and unpacked it big-endian, which made
// byte 29 the Q level and byte 26 the R level. Q/R and W/E therefore traded
// levels on every login, and a level that does not match the item in the
// inventory makes CNewUIItemHotKey::GetHotKeyItemIndex() find nothing: the
// slot renders empty and the key does nothing, which reads to the player as
// "the server did not save my potions".
//
// Everything here is pure (no engine types, no globals) so it can be unit
// tested; see tests/keyconfig.
namespace KeyConfiguration
{
    /// <summary>Total size of the configuration blob, in bytes.</summary>
    inline constexpr std::size_t Size = 30;

    /// <summary>Number of item hotkey slots: Q, W, E, R (in that order).</summary>
    inline constexpr std::size_t SlotCount = 4;

    /// <summary>Value stored in an item slot which is not bound to anything.</summary>
    inline constexpr std::uint8_t Unbound = 0xFF;

    /// <summary>
    /// Highest item level a real item can carry. Anything above it can never match an
    /// item in the inventory, so it is normalized to 0 instead of being kept and saved
    /// back - that is what heals the blobs the old asymmetric mapping already corrupted.
    /// </summary>
    inline constexpr int MaxItemLevel = 15;

    /// <summary>Offset of the item index of slot i (Q, W, E, R). R sits behind the chat box byte.</summary>
    inline constexpr std::size_t ItemOffset[SlotCount] = { 21, 22, 23, 25 };

    /// <summary>Offset of the item level of slot i (Q, W, E, R).</summary>
    inline constexpr std::size_t LevelOffset[SlotCount] = { 26, 27, 28, 29 };

    /// <summary>One potion hotkey slot. <c>ItemIndex</c> is the index inside the potion
    /// item group, or -1 when the slot is unbound.</summary>
    struct PotionSlot
    {
        int ItemIndex;
        int ItemLevel;
    };

    /// <summary>Clamps an item level to the range a real item can have.</summary>
    inline int SanitizeItemLevel(int itemLevel)
    {
        return (itemLevel < 0 || itemLevel > MaxItemLevel) ? 0 : itemLevel;
    }

    /// <summary>Reads one potion slot out of a configuration blob of <see cref="Size"/> bytes.</summary>
    inline PotionSlot ReadPotionSlot(const std::uint8_t* configuration, std::size_t slot)
    {
        PotionSlot result{ -1, 0 };
        if (configuration == nullptr || slot >= SlotCount)
        {
            return result;
        }

        const std::uint8_t itemIndex = configuration[ItemOffset[slot]];
        if (itemIndex == Unbound)
        {
            return result;
        }

        result.ItemIndex = static_cast<int>(itemIndex);
        result.ItemLevel = SanitizeItemLevel(static_cast<int>(configuration[LevelOffset[slot]]));
        return result;
    }

    /// <summary>Writes one potion slot into a configuration blob of <see cref="Size"/> bytes.</summary>
    inline void WritePotionSlot(std::uint8_t* configuration, std::size_t slot, PotionSlot value)
    {
        if (configuration == nullptr || slot >= SlotCount)
        {
            return;
        }

        if (value.ItemIndex < 0 || value.ItemIndex >= static_cast<int>(Unbound))
        {
            configuration[ItemOffset[slot]] = Unbound;
            configuration[LevelOffset[slot]] = 0;
            return;
        }

        configuration[ItemOffset[slot]] = static_cast<std::uint8_t>(value.ItemIndex);
        configuration[LevelOffset[slot]] = static_cast<std::uint8_t>(SanitizeItemLevel(value.ItemLevel));
    }
}

#endif	// _KEYCONFIGURATION_H_
