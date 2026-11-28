# Kroko italiano su DietPi per ALBATROS

Questo pacchetto prepara il modello Kroko italiano Community 64-L e riproduce una clip di 5,76 secondi alla velocità del microfono. Stampa il testo, il tempo di risposta rispetto all'ultimo campione inviato e il picco di RAM del processo Kroko.

## Preparazione e prova

Requisiti. DietPi Debian x86-64, shell Bash, Python 3, curl, unzip, ip e unshare. Servono circa 250 MB di download e 300 MB liberi durante la preparazione. Il server e le dipendenze sono scaricati nelle versioni già verificate nel collaudo ALBATROS e controllati con SHA-256.

Copia `ALBATROS_Kroko_DietPi.zip` sul mini PC ed esegui dalla cartella in cui si trova lo ZIP.

```bash
unzip -n ALBATROS_Kroko_DietPi.zip
cd ALBATROS_Kroko_DietPi
bash prepara.sh
bash prova.sh
```

La prova usa un namespace di rete temporaneo che contiene soltanto il loopback. Avvia Kroko, invia la clip e arresta il processo all'uscita. Da utente non root usa un namespace utente. Se il kernel risponde `Operation not permitted`, la prova richiede una shell di amministrazione sul sistema host. `runtime/server.log` contiene l'output del server. Per ripetere una prova già preparata basta `bash prova.sh`, anche in assenza di Internet.

Il riferimento inizia con «La religione predominante in Moldavia». Il modello usato durante il collaudo ha trascritto «cristiana-ortodossa» al posto di «cristiano-ortodossa». Il checkpoint di questo script è l'esecuzione riuscita con testo e misure sul processore fisico. La misura di latenza è riferita all'ultimo campione, che può contenere silenzio.

Kroko Community gira in locale attraverso il server WebSocket. L'integrazione successiva con Assist richiederà un adattatore fra questo protocollo e Wyoming e una pipeline di prova in Home Assistant.

## Fonti

- [Documentazione Kroko on-premise](https://docs.kroko.ai/on-premise/)
- [Kroko italiano Community 64-L](https://huggingface.co/Banafo/Kroko-ASR/blob/main/Kroko-IT-Community-64-L-Streaming-001.data)
- [ONNX Runtime 1.17.1](https://central.sonatype.com/artifact/com.microsoft.onnxruntime/onnxruntime/1.17.1)
- [Wyoming in Home Assistant](https://www.home-assistant.io/integrations/wyoming/)
- Il campione deriva da [Google FLEURS it_it, validazione](https://huggingface.co/datasets/google/fleurs/tree/70bb2e84b976b7e960aa89f1c648e09c59f894dd/data/it_it), file `5749465013419864787.wav`. Audio e riferimento con licenza [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). La clip originale è stata convertita da float32 a PCM16.
