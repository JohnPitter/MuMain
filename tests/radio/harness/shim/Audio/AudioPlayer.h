// Harness stand-in for the client's AudioPlayer facade: the harness owns a
// real SDL_mixer mixer (created exactly like AudioPlayer::Initialize does) and
// hands it to RadioEngine through this same GetMixer() seam.
#pragma once

struct MIX_Mixer;

namespace AudioPlayer
{
    MIX_Mixer* GetMixer();
}
