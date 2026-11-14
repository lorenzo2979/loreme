# GW006 IR Universal — scelta GPIO confermata dall'utente

Data: 14 novembre 2026

Hardware: GW006 con WBRG1 / RTL8721CSM, attualmente OpenBeken 1.18.321.

**Scelta dell'utente: usare i numeri OpenBeken 25 e 26 per il circuito IR.**

Mappatura dalla tabella dei pin OpenBeken RTL8720D:
- OpenBeken **25** → Realtek **PA25** → **IR TX** (trasmissione, conforme al progetto esistente che aveva TX=PA25).
- OpenBeken **26** → Realtek **PA26** → **IR RX** (ricezione, conforme al progetto esistente che aveva RX=PA26).

Fonte del mapping numerico: `openshwprojects/OpenBK7231T_App/src/hal/realtek/rtl8720d/hal_pins_rtl8720d.c`.

**Distinguere scelta dai test:** questa scelta è il pinout richiesto per il porting, ma non certifica l'elettronica fisica o il pinmux del GW006. In particolare PA25 è stato descritto anche come possibile collegamento al LED rosso; confermare elettricamente prima di attivare la trasmissione IR, non pilotare il pin a 5V. Verificare che PA26 sia effettivamente connesso al ricevitore IR. La scelta non autorizza il flashing.

Le implementazioni nuove devono usare **TX PA25 / RX PA26** anziché PB31 e non assumere i GPIO ESP32-C3 (TX5/RX21).
La memoria Flash del GW006, il boot e il ripristino restano soggetti a verifiche indipendenti.
