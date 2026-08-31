#pragma once

namespace AntiCheat
{
    // Starts the opt-in process/module scanner and HTTP heartbeat worker.
    // Set MU_ANTICHEAT_HEARTBEAT_URL to enable reporting.
    void Start();
    void Stop();
}
