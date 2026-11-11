# REV5 test report — 11 November 2026

Host verification completed in this session, no GW006 connected:

* GCC C11 `make clean test`: **PASS** engine, streaming parser, mock socket reconnect/ACK.
* Clang C11 with AddressSanitizer + UndefinedBehaviorSanitizer: **PASS** same tests.
* Kotlin JVM 1.9 with JRE 21: **PASS** protocol 3 encode/decode, CRC rejection, carrier/pulse limits, staging IDs and duplicate handling, full 96-button file storage, 5 frames per button, 2048 pulses/frame, corruption rejection and reload after simulated process restart.

Cases exercised: TCP header fragmented to single bytes, pipelined messages, invalid CRC, invalid header/size, last RX frame idle flush, five frames in order, FIFO overflow, RX_READ repeated without ACK, wrong ACK rejected, duplicate ACK idempotent, disconnect/reconnect while RAM stays powered, RX_ARM BUSY with pending queue, host-only TX stub.

**Not tested:** physical Realtek IR, actual Wi-Fi, actual Android OS/filesystem/app UI, device reset loss handling, KM0/KM4 build, secure authentication or recovery flashing.

## Reproduce where compilers are available (or in a remote runner)

```
make clean test
make clean
make CC=clang CFLAGS='-O1 -g -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer' test
kotlinc android/core/*.kt android/tests/*.kt -include-runtime -d gwir_v3_test.jar
java -jar gwir_v3_test.jar
```

These commands do not flash firmware.
