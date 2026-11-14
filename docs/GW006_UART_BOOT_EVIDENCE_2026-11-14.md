# GW006 / RTL8721CSM — riscontro UART effettivo (14/11/2026)

Questo documento registra solo i dettagli tecnici del boot ricevuti dal proprietario. Non contiene SSID, IP, MAC o password.

| Prova UART | Evidenza | Significato |
|---|---|---|
| Loader Realtek | `IMG1 ENTER ROMSUB:2`, `OTA1 USE` | Bootloader attivo, selezione OTA1 |
| KM0 | `KM0 BOOT_IMG2 BOOT REASON: 0` | Immagine KM0 avviata |
| KM4 | `KM4 BOOT REASON: 0` | Immagine KM4 avviata |
| Flash | `Flash ID:68, 40, 17` | JEDEC capacity 0x17 = 8.388.608 byte (8 MiB) |
| Lettura Flash | `FLASH CALIB[NEW OK]` | Calibrazione completata |
| Firmware | `OpenRTL8720D, version 1.18.321` | OpenBeken si avvia realmente |
| Configurazione | `Correct config has been loaded` | Impostazioni presenti e caricate |
| File system | `lfs is absent` | LittleFS non disponibile; non significa ROM guasta |
| Rete | `WIFI_STA_CONNECTED - 4` | Associazione Wi-Fi eseguita |
| Web | `TCP server listening` | Server HTTP avviato; non prova accessibilità dal telefono |

**Correzione:** OpenBeken è avviabile benché l'utente riferisca che non funziona per il suo scopo. Non dichiarare il dispositivo non avviabile.

**Importante:** le righe `IMG2 DATA`, `IMG2 ENTRY` includono indirizzi virtuali/di caricamento. Non trattarle come offset NOR grezzi. Questo log da solo **non** autorizza la zona 0x200000..0x6FFFFF per il journal IR.

Prossima verifica utile: immagine Flash della singola GW006 **solo in lettura**, o mappa dettagliata estratta dalle sue tabelle di boot con strumenti Realtek. Poi stabilire un layout capace di 96 tasti × 5 pacchetti (peggior caso 4.378.624 byte con sostituzione atomica), senza sovrapposizioni con KM0/KM4/boot.

La versione Arduino su ramo review produce tre immagini RTL8721DM di riferimento, **non ancora collaudate con WBRG1**; non sostituire ciecamente la ROM o cancellare settori per tentativi.
