# Portable Kotlin module for eventual Android app

There is **no APK, Activity, remote socket service or UI** in this folder. Three Kotlin/JVM source files are ready to integrate into a native Android project (API 26+):

- `GwirV3.kt`: length-checked GWIR packet codec, CRC32, IR frame model, RX capture token + TX frame conversion.
- `AndroidCaptureInbox.kt`: stage each received frame in a verified local file **before** sending RX_ACK; duplicate capture IDs after reconnect do not create duplicate frames.
- `AndroidCodeStore.kt`: final files for 96 buttons; each button may contain one to five IR frames; atomic-style replacement and integrity verification.

Use `context.filesDir` as the base directory. Do not put code files in public storage. Give capture staging and final store a *private* app directory. On an Android process restart, load any pending staging entries before starting a new capture; re-read and ACK pending GW006 frames, then `commit()` only after checking queue stats (no frames dropped).

**Important limits:** The existing stage token is unique only per device uptime, not universally. The future app must associate staging with a deliberate capture session and prevent reuse after unexpected device reboot. The stage currently has no encrypted backup/export implementation, no pairing, no automation of network IO, and has not been tested on an Android phone. File CRC is for data integrity, not secrecy or authenticity.
