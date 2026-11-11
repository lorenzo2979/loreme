CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
CORE = firmware/core/gwir_protocol.c firmware/core/gwir_engine.c firmware/core/gwir_capture_builder.c firmware/core/gwir_stream.c firmware/core/gwir_auth.c firmware/core/gwir_auth_dispatch.c
INCLUDE = -Ifirmware/core

.PHONY: test clean arm-smoke

test: build/test_engine build/test_stream build/test_amebad_tx_codec build/test_amebad_tx_fifo build/test_amebad_rx_fifo build/test_amebad_rx_irq build/test_amebad_bus build/test_auth build/test_auth_dispatch build/test_sdk_hmac build/mock_tcp_server
	./build/test_engine
	./build/test_stream
	./build/test_amebad_tx_codec
	./build/test_amebad_tx_fifo
	./build/test_amebad_rx_fifo
	./build/test_amebad_rx_irq
	./build/test_amebad_bus
	./build/test_auth >/dev/null
	./build/test_auth_dispatch
	./build/test_sdk_hmac
	python3 tests/test_tcp_loopback.py

build/test_engine: tests/test_engine.c $(CORE) firmware/core/gwir_protocol.h firmware/core/gwir_engine.h firmware/core/gwir_capture_builder.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) $(CORE) tests/test_engine.c -o $@


build/test_amebad_tx_codec: tests/test_amebad_tx_codec.c firmware/realtek/gwir_amebad_tx_codec.c firmware/realtek/gwir_amebad_tx_codec.h firmware/core/gwir_protocol.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) -Ifirmware/realtek firmware/realtek/gwir_amebad_tx_codec.c tests/test_amebad_tx_codec.c -o $@

build/test_amebad_tx_fifo: tests/test_amebad_tx_fifo.c firmware/realtek/gwir_amebad_tx_fifo.c firmware/realtek/gwir_amebad_tx_vendor.c firmware/realtek/gwir_amebad_tx_codec.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) -Ifirmware/realtek -Itests/stubs firmware/realtek/gwir_amebad_tx_fifo.c firmware/realtek/gwir_amebad_tx_vendor.c firmware/realtek/gwir_amebad_tx_codec.c tests/test_amebad_tx_fifo.c -o $@

build/test_amebad_rx_fifo: tests/test_amebad_rx_fifo.c firmware/realtek/gwir_amebad_rx_fifo.c firmware/realtek/gwir_amebad_rx_vendor.c firmware/realtek/gwir_amebad_rx_fifo.h $(CORE)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) -Ifirmware/realtek -Itests/stubs $(CORE) firmware/realtek/gwir_amebad_rx_fifo.c firmware/realtek/gwir_amebad_rx_vendor.c tests/test_amebad_rx_fifo.c -o $@

build/mock_tcp_server: tests/mock_tcp_server.c $(CORE) firmware/core/gwir_stream.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) $(CORE) tests/mock_tcp_server.c -o $@

build/test_stream: tests/test_stream.c $(CORE) firmware/core/gwir_stream.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) $(CORE) tests/test_stream.c -o $@

clean:
	rm -rf build

.PHONY: test-kotlin
test-kotlin:
	@command -v kotlinc >/dev/null || { echo 'Kotlin compiler unavailable here; use a remote runner (no install on user PC).'; exit 1; }
	@mkdir -p build
	kotlinc android/core/*.kt android/tests/*.kt -include-runtime -d build/gwir_v3_jvm_test.jar
	java -jar build/gwir_v3_jvm_test.jar

arm-smoke:
	./scripts/cross_arm_smoke.sh

.PHONY: test-sdk-integration
# Pure mock SDK project wiring test: does NOT build against Realtek proprietary libs.
test-sdk-integration:
	python3 tests/test_sdk_integration.py

build/test_amebad_rx_irq: tests/test_amebad_rx_irq.c firmware/realtek/gwir_amebad_rx_irq.c firmware/realtek/gwir_amebad_rx_vendor.c firmware/realtek/gwir_amebad_rx_fifo.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) -Ifirmware/realtek -Itests/stubs $(CORE) firmware/realtek/gwir_amebad_rx_irq.c firmware/realtek/gwir_amebad_rx_vendor.c firmware/realtek/gwir_amebad_rx_fifo.c tests/test_amebad_rx_irq.c -o $@

build/test_amebad_bus: tests/test_amebad_bus.c firmware/realtek/gwir_amebad_bus.c firmware/realtek/gwir_amebad_tx_fifo.c firmware/realtek/gwir_amebad_tx_codec.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) -Ifirmware/realtek firmware/realtek/gwir_amebad_bus.c firmware/realtek/gwir_amebad_tx_fifo.c firmware/realtek/gwir_amebad_tx_codec.c tests/test_amebad_bus.c -o $@

# OpenSSL is used by host tests ONLY. Target needs separately vetted HMAC backend.
build/test_auth: tests/test_auth.c firmware/core/gwir_auth.c firmware/core/gwir_protocol.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) firmware/core/gwir_protocol.c firmware/core/gwir_auth.c tests/test_auth.c -lcrypto -o $@

build/test_auth_dispatch: tests/test_auth_dispatch.c firmware/core/gwir_auth_dispatch.c firmware/core/gwir_auth.c $(CORE)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDE) $(CORE) tests/test_auth_dispatch.c -lcrypto -o $@

# Realtek-compatible Mbed TLS interface, HOST TEST shim uses OpenSSL only.
build/test_sdk_hmac: tests/test_sdk_hmac.c tests/mock_mbedtls_md.c firmware/sdk/gwir_sdk_hmac.c firmware/core/gwir_auth.c firmware/core/gwir_protocol.c
	@mkdir -p build
	$(CC) $(CFLAGS) -Ifirmware/sdk $(INCLUDE) -Itests/stubs firmware/sdk/gwir_sdk_hmac.c firmware/core/gwir_auth.c firmware/core/gwir_protocol.c tests/mock_mbedtls_md.c tests/test_sdk_hmac.c -lcrypto -o $@
