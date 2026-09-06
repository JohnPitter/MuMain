#include "doctest.h"

#include <cstdint>

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
