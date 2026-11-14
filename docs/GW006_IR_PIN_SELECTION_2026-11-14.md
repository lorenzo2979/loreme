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

## Aggiornamento utente 14/11/2026 — rimozione LED rosso

L'utente conferma che **il LED rosso associato a PA25 verrà rimosso** dalla propria scheda GW006. Questa decisione sostituisce, per il firmware IR Universal, il vecchio requisito di usare PA25 come indicatore luminoso APConfig. Il LED blu Wi-Fi (PB22) e il tasto APConfig (PB23) restano invariati.

**Assegnazione definitiva richiesta: PA25 = IR TX, PA26 = IR RX.**
La selezione PB31 TX del vecchio studio 11/11/2026 è superata da questa decisione successiva, salvo problemi elettrici verificati.

Lo stato è una **modifica hardware pianificata**, non una rimozione già verificata. I controlli elettrici prima dell'attivazione dei segnali devono confermare, a scheda disalimentata:
- LED rosso effettivamente rimosso e nessun altro carico/conflitto sulla net PA25; attenzione a eventuali resistenze e collegamenti residui.
- PA25 è un segnale logico 3,3 V per comandare uno stadio IR a transistor; **non** deve alimentare direttamente un LED trasmittente di potenza.
- PA26 porta il segnale del ricevitore IR nella corretta polarità/ampiezza, sempre a 3,3 V.
- Ogni precedente comportamento software LED/APConfig su PA25 deve essere eliminato o disabilitato prima dell'avvio della periferica IR.

La configurazione Arduino di riferimento AW-CU488 mappa `IR_TX = AMB_D6 = PA25` e `IR_RX = AMB_D5 = PA26`, come risulta dal file ufficiale Realtek `Arduino_package/hardware/variants/sparkfun_thingplus-awcu488/variant.h`. La corrispondenza di pin non equivale a un collaudo del PCB GW006.

Tali condizioni non alterano l'obbligo di verificare boot, Flash e ripristino prima di caricare firmware diverso da OpenBeken.
