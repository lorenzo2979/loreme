# CP-23 — Native SDK baseline probe (REVIEW BRANCH ONLY)

Date: 2026-11-12. **NO FLASH.** No change to the running GW006/OpenBeken 1.18.321.

This diagnostic branch temporarily allows the existing REV21 native build workflow to run on its own review-branch workflow update. It operates ONLY on the already-present, SHA-256-verified REV21 GitHub source delta ZIP. This is **NOT** the local CP-22 locked source package and **NOT** a CP-22 native build.

The local CP-22 bundle has 137 SHA-256-locked source files and 34 mapped C modules; the repository main has only 21 standalone C sources (some sources may exist inside archived deltas). The full CP-22 source sync and its native gate must be published separately, reviewed, and rerun.

SDK pinned: Ameba-AIoT/ameba-rtos-d commit 7569200fb8304daa61a9f09ccf439ada38d6de2e.
Workflow: .github/workflows/sdk-native-rev21.yml; outputs diagnostics only, no installable firmware delivered.
Never merge this branch before reviewing the impact of the extra push trigger on main.
