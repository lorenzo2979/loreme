/*
 * GW006 IR Universal -- Arduino AmebaD toolchain compatibility probe.
 *
 * BUILD-ONLY: the official board selected by CI is AW-CU488 RTL8721DM,
 * NOT the GW006 WBRG1 RTL8721CSM. Compiling this sketch does NOT qualify
 * the image for flashing, boot, NOR writes, provisioning or physical IR.
 *
 * This intentionally tests the vendor C++ API at compile and linker stages.
 * No pin is configured; no Wi-Fi connection or flash operation is attempted
 * from setup()/loop().
 */
#include <Arduino.h>
#include <WiFi.h>
#include <IRDevice.h>
#include <FlashMemory.h>

static constexpr unsigned int kTVButtons = 36u;
static constexpr unsigned int kClimateButtons = 14u;
static constexpr unsigned int kDecoderButtons = 32u;
static constexpr unsigned int kSpeakerButtons = 14u;
static constexpr unsigned int kFramesPerButton = 5u;
static_assert(kTVButtons + kClimateButtons + kDecoderButtons + kSpeakerButtons == 96u,
              "GW006 exactly 96 buttons");
static_assert(kFramesPerButton == 5u, "GW006 exactly five IR frames per button");

/* Compile and LINK the underlying Wi-Fi, IR transmitter and FlashMemory
 * library methods. This function is NOT called at runtime, and is retained
 * only for static toolchain compatibility evidence. On actual GW006 use the
 * verified WBRG1 GPIO map rather than the AW-CU488 IR_TX pin.
 */
__attribute__((used, noinline))
void gw006_amebad_link_only_probe(void) {
    unsigned int sample[4] = {900u, 450u, 560u, 560u};
    uint8_t state = WiFi.status();
    IPAddress ip = WiFi.localIP();
    IR.begin((uint8_t)IR_TX, IR_MODE_TX, 38000u);
    IR.send(sample, 4u);
    unsigned int word = FlashMemory.readWord(0u);
    Serial.print((unsigned long)(state + ip[0] + word));
}

void setup() {
    Serial.begin(115200);
    Serial.println("GW006 AMEBAD ARDUINO TOOLCHAIN PROBE ONLY - NOT A GW006 IMAGE");
}
void loop() {
    delay(1000);
}
