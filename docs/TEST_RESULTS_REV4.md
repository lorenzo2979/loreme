# REV4 local test results — 11 November 2026

Executed against actual source files in this ZIP on the local host (not on WBRG1):

- GCC: `make clean test CC=gcc CFLAGS='-O2 -std=c11 -Wall -Wextra -Werror -pedantic'` — PASS.
- Clang: `make clean test CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer'` — PASS.
- `test_engine`: CRC, 5-frame FIFO, IR RAW assembler, final idle flush, overflow and max frame — PASS.
- `test_stream`: fragmentation, coalescence, max message, bad header/CRC/length and failed session — PASS.
- `test_tcp_loopback.py`: localhost TCP exchanges with MOCK IR receiver/transmitter — PASS.
- Realtek SDK full build, driver integration, hardware IR, flash layout and Android APK — **NOT TESTED**.

The mock server is deliberately bound to 127.0.0.1 and must not be used as firmware or production server. These results do not prove native WBRG1 operability.
