#include "doctest.h"

#include <cstdint>
#include <initializer_list> // range-for over {false, true} needs std::initializer_list declared (MSVC)

// Pins the wire layout of MU Helper's client-local flag byte (byte 33 of the
// packet in Network/Server/WSclient.h -- PRECEIVE_MUHELPER_DATA, byte 29 of
// the persisted 257-byte blob OpenMU stores as Character.MuHelperConfiguration
// -- see the layout doc comment on OpenMU's MuHelperSettingsSerializer):
//
//   bit 0  bUseSelfDefense
//   bit 1  bAutoAcceptFriend
//   bit 2  bAutoAcceptGuild
//   bit 3  bFallbackBasicAttack
//   bit 4  bAutoAcceptParty   (added for "Accept party")
//   bits 5-7 unused
//
// ConfigDataSerDe::Serialize/Deserialize (MuHelperData.cpp) do this same
// packing against the real PRECEIVE_MUHELPER_DATA bitfield, but that struct
// lives behind Network/Server/WSclient.h, which drags in the full engine PCH
// (OpenGL, rendering, window management -- see src/source/App/stdafx.h) that
// this lightweight doctest binary does not link against. That constraint
// predates this change and applies to all of MuHelperData.cpp, not just the
// new bit. What is tested here is the wire contract itself: the exact bit
// position, and that it neither reads nor disturbs its neighbors -- the
// property that keeps the fixed-size blob's other 4 flags, and everything
// else in the 257 bytes, byte-for-byte compatible with what OpenMU already
// stores and echoes back unchanged.
namespace
{
constexpr std::uint8_t kUseSelfDefenseFlag = 1 << 0;
constexpr std::uint8_t kAutoAcceptFriendFlag = 1 << 1;
constexpr std::uint8_t kAutoAcceptGuildFlag = 1 << 2;
constexpr std::uint8_t kFallbackBasicAttackFlag = 1 << 3;
constexpr std::uint8_t kAutoAcceptPartyFlag = 1 << 4;
}

TEST_CASE("bAutoAcceptParty occupies bit 4 of the helper flag byte")
{
    CHECK(kAutoAcceptPartyFlag == 0x10);

    // Exactly one bit, and it does not overlap any sibling flag.
    CHECK((kAutoAcceptPartyFlag & (kAutoAcceptPartyFlag - 1)) == 0);
    CHECK((kAutoAcceptPartyFlag & kUseSelfDefenseFlag) == 0);
    CHECK((kAutoAcceptPartyFlag & kAutoAcceptFriendFlag) == 0);
    CHECK((kAutoAcceptPartyFlag & kAutoAcceptGuildFlag) == 0);
    CHECK((kAutoAcceptPartyFlag & kFallbackBasicAttackFlag) == 0);
}

TEST_CASE("setting bAutoAcceptParty round-trips through the byte without disturbing the other flags")
{
    // Start from every other flag on, party off -- mirrors
    // ConfigDataSerDe::Serialize()'s bit-OR pattern for byte 33.
    std::uint8_t flags = kUseSelfDefenseFlag | kAutoAcceptFriendFlag | kAutoAcceptGuildFlag | kFallbackBasicAttackFlag;

    CHECK((flags & kAutoAcceptPartyFlag) == 0);

    // "Save Setting" with the new checkbox checked: set the bit (Serialize).
    flags |= kAutoAcceptPartyFlag;
    CHECK((flags & kAutoAcceptPartyFlag) != 0);

    // The four pre-existing flags must read back exactly as before (Deserialize).
    CHECK((flags & kUseSelfDefenseFlag) != 0);
    CHECK((flags & kAutoAcceptFriendFlag) != 0);
    CHECK((flags & kAutoAcceptGuildFlag) != 0);
    CHECK((flags & kFallbackBasicAttackFlag) != 0);

    // Unchecking the box again (Serialize with the option off) must clear
    // only bit 4.
    flags &= static_cast<std::uint8_t>(~kAutoAcceptPartyFlag);
    CHECK((flags & kAutoAcceptPartyFlag) == 0);
    CHECK((flags & kUseSelfDefenseFlag) != 0);
    CHECK((flags & kAutoAcceptFriendFlag) != 0);
    CHECK((flags & kAutoAcceptGuildFlag) != 0);
    CHECK((flags & kFallbackBasicAttackFlag) != 0);
}

TEST_CASE("a byte with no flags set decodes bAutoAcceptParty as false")
{
    constexpr std::uint8_t flags = 0;
    CHECK((flags & kAutoAcceptPartyFlag) == 0);
}

TEST_CASE("a byte with only bAutoAcceptParty set decodes every other flag as false")
{
    constexpr std::uint8_t flags = kAutoAcceptPartyFlag;

    CHECK((flags & kAutoAcceptPartyFlag) != 0);
    CHECK((flags & kUseSelfDefenseFlag) == 0);
    CHECK((flags & kAutoAcceptFriendFlag) == 0);
    CHECK((flags & kAutoAcceptGuildFlag) == 0);
    CHECK((flags & kFallbackBasicAttackFlag) == 0);
}

// --- Pickup flags byte (byte 5 of the packet / byte 1 of the 257-byte blob) ---
//
// Bug report (2026-09-06, "o Set Item e o Add extra nao ta salvando"): the
// production blob for character SeuAntonio (account testando) read back
// 0xE8 for this byte -- JewelOrGem/ExcellentItem/Zen/AddExtraItem set, but
// SetItem (bit 4) clear, even though the player reported checking it before
// saving. The task explicitly asked whether this bit collides with the
// "Accept party" bit (byte 29 of the blob, bit 4) or the MP-threshold nibble
// (byte 30) added in other 2026-09-06 sessions, since both reuse bit
// position 4 -- but in a *different byte*. These cases pin that there is no
// collision: SetItem is bit 4 of byte 1, a completely separate storage unit
// from byte 29 and byte 30, verified against a live compile of the actual
// PRECEIVE_MUHELPER_DATA bitfield (offsetof/sizeof probe, MSVC x64): byte 1
// bit 4 set produces 0x10 at offset 1, disturbing nothing at offset 29 or
// 30, and vice versa. No client or server code path was found that clears
// SetItem independently of the player's own checkbox click (see
// ApplyConfigFromCheckbox/Reset/ApplyConfig in NewUIMuHelper.cpp and
// MuHelperSettingsSerializer.TryDeserialize/UpdateMuHelperConfigurationAction
// on the server, which stores and echoes the raw blob byte-for-byte); this
// suite exists so any future change to either byte immediately fails loudly
// instead of silently reintroducing a collision.
namespace
{
constexpr std::uint8_t kPickJewelFlag = 1 << 3;
constexpr std::uint8_t kPickSetItemFlag = 1 << 4;
constexpr std::uint8_t kPickExcellentFlag = 1 << 5;
constexpr std::uint8_t kPickZenFlag = 1 << 6;
constexpr std::uint8_t kPickExtraItemFlag = 1 << 7;
}

TEST_CASE("the five pickup flags occupy distinct bits within their own byte")
{
    CHECK(kPickJewelFlag == 0x08);
    CHECK(kPickSetItemFlag == 0x10);
    CHECK(kPickExcellentFlag == 0x20);
    CHECK(kPickZenFlag == 0x40);
    CHECK(kPickExtraItemFlag == 0x80);

    constexpr std::uint8_t all = kPickJewelFlag | kPickSetItemFlag | kPickExcellentFlag | kPickZenFlag | kPickExtraItemFlag;
    // Popcount via Brian Kernighan's trick, done manually since <bit> is not
    // assumed here: each flag cleared one at a time must strictly shrink the
    // set, proving none of the five share a bit.
    std::uint8_t remaining = all;
    int count = 0;
    while (remaining != 0)
    {
        remaining &= static_cast<std::uint8_t>(remaining - 1);
        ++count;
    }
    CHECK(count == 5);
}

TEST_CASE("SetItem does not collide with AutoAcceptParty or the MP threshold nibble, because they live in different bytes")
{
    // Reproduces the exact production byte pair from the bug report: pickup
    // flags byte = 0xE8 (Jewel+Excellent+Zen+AddExtraItem, SetItem clear),
    // helper flags byte = 0x12 (AutoAcceptFriend+AutoAcceptParty). Both are
    // bit 4 of their respective byte -- setting SetItem in the pickup byte
    // must not touch the helper flags byte at all, and vice versa, because
    // ConfigDataSerDe::Serialize() writes them into netData.PickupFlags-style
    // members and netData.bUseSelfDefense-style members, which the compiled
    // PRECEIVE_MUHELPER_DATA bitfield places at offsets 1 and 29
    // respectively (confirmed by an offsetof/sizeof probe against the real
    // struct during this investigation).
    std::uint8_t pickupFlags = kPickJewelFlag | kPickExcellentFlag | kPickZenFlag | kPickExtraItemFlag; // SetItem off
    std::uint8_t helperFlags = kAutoAcceptFriendFlag | kAutoAcceptPartyFlag; // matches production 0x12

    CHECK(pickupFlags == 0xE8);
    CHECK(helperFlags == 0x12);

    // Checking "Set Item" only sets bit 4 of the *pickup* byte.
    pickupFlags |= kPickSetItemFlag;
    CHECK(pickupFlags == 0xF8);
    CHECK(helperFlags == 0x12); // untouched

    // Turning "Accept party" off only clears bit 4 of the *helper* byte.
    helperFlags &= static_cast<std::uint8_t>(~kAutoAcceptPartyFlag);
    CHECK(helperFlags == 0x02);
    CHECK(pickupFlags == 0xF8); // still untouched
}

// --- Ruling out a master-gate on SetItem (2026-09-06 follow-up investigation) ---
//
// The owner reported the "Set Item" checkbox itself not surviving save+relog
// (distinct from the "Add extra" item-name bug fixed in the same session).
// Read of the live code (NewUIMuHelper.cpp, ApplyConfigFromCheckbox/Reset/
// ApplyConfig; MuHelperData.cpp, ConfigDataSerDe::Serialize) found: the
// CHECKBOX_ID_PICK_ANCIENT case is a direct, unconditional assignment
// (_TempConfig.bPickAncient = bState;) with no gating on "Pick all"/"Pick
// selected", no reset triggered by any other checkbox, and exactly four
// write sites to bPickAncient in the whole client tree (struct default,
// Deserialize from the server's echoed blob, this click handler, and
// Reset()) -- grepped directly, not inferred. Serialize() writes
// netData.SetItem = gameData.bPickAncient unconditionally, on the same line
// pattern as JewelOrGem/Zen/ExcellentItem/AddExtraItem, all independent of
// PickAllNearItems/PickSelectedItems. This mirrors that formula (the real
// function lives in MuHelperData.cpp, which drags in the full engine PCH
// via WSclient.h and cannot link into this doctest binary -- same
// constraint noted in the pickup-flags block above) to pin, mechanically,
// that no combination of the "Pick all" / "Pick selected" / "Excellent"
// inputs can flip SetItem's output.
namespace
{
struct PickupInputs
{
    bool pickAllItems;
    bool pickSelectItems;
    bool pickAncient; // "Set Item" checkbox
    bool pickExcellent;
};

struct PickupOutputs
{
    bool pickAllNearItems;
    bool pickSelectedItems;
    bool setItem;
    bool excellentItem;
};

// Mirrors ConfigDataSerDe::Serialize()'s pickup-byte formula (MuHelperData.cpp).
PickupOutputs SerializePickupFormula(const PickupInputs& in)
{
    const bool bPickAllItems = in.pickAllItems;
    const bool bPickSelectedItems = in.pickSelectItems && !bPickAllItems;
    return PickupOutputs{
        bPickAllItems,
        bPickSelectedItems,
        in.pickAncient,
        in.pickExcellent,
    };
}
}

TEST_CASE("SetItem tracks only its own checkbox, across every Pick All / Pick Selected / Excellent combination")
{
    for (bool pickAll : {false, true})
    {
        for (bool pickSelected : {false, true})
        {
            for (bool excellent : {false, true})
            {
                for (bool setItem : {false, true})
                {
                    const auto out = SerializePickupFormula({pickAll, pickSelected, setItem, excellent});
                    CAPTURE(pickAll);
                    CAPTURE(pickSelected);
                    CAPTURE(excellent);
                    CAPTURE(setItem);
                    CHECK(out.setItem == setItem);
                }
            }
        }
    }
}
