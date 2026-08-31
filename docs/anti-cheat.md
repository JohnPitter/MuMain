# Client anti-cheat

The Windows client performs a background integrity check when a heartbeat endpoint is configured. It scans for a small blacklist of commonly used debugging/packet-inspection processes and reports SHA-256 hashes for the executable and loaded modules.

Configure the launcher or environment before starting the client:

- `MU_ANTICHEAT_HEARTBEAT_URL`: HTTPS (or HTTP) endpoint receiving a JSON `POST`.
- `MU_ANTICHEAT_INTERVAL_SECONDS`: heartbeat interval, minimum 10 seconds (default 60).

The check is telemetry only: the client does not terminate processes or ban locally. The server must authenticate and validate reports, and should treat missing or failed heartbeats according to its own policy. No endpoint is contacted when the URL is unset.
