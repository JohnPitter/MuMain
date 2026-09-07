#pragma once

#include <cstddef>

// Account VIP flag pushed by the server for the hero and for players in view
// (C1 F3 EC, LuxView-only — see docs/network/luxview-custom-packets.md and
// WSclient.cpp's 0xF3 dispatch). Unlike CharacterTitle, there is no catalog:
// it is a single boolean applied straight to the CHARACTER slot, and it
// defaults to false (see CHARACTER::Initialize), so a server that never sends
// the packet — today's Production — never shows the badge.
namespace VipStatus
{
    // Applies the VIP flag carried by the packet to the addressed player's
    // CHARACTER slot. No-op if the player id is unknown or the buffer is short.
    void ReceiveStatus(const BYTE* buffer, int32_t size);
}
