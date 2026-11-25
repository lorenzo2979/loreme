# ALBATROS AI V1.1 — checkpoint tecnico, 25 novembre 2026

**Prototipo personale distinto dal router Multi-STT; nessuna modifica ad Alexa, Home Assistant o Music Assistant.**

## Verificato localmente
- 14 test Python superati (8 test base + 6 nuovi su progetti e sicurezza).
- Eseguita console offline con creazione progetto, attività, elenco, completamento.
- Memoria SQLite persistente; controllo accessi agli strumenti, conferma per scritture richieste al modello AI.
- Controllo di JSON ambiguo (non può essere contemporaneamente risposta e comando).
- Creato ZIP completo verificato: `ALBATROS_AI_V1_1_TESTATO.zip`, SHA256 `adb263ff19c596ad8fcbd9fb7a7eaa3b34f1d9bb43e76a70a06728f34c22464b`.

## Limiti
- Chiamate effettive a Groq/Gemini NON collaudate: nessuna chiave API usata.
- Nessun collegamento all'impianto Home Assistant, a Music Assistant o al satellite Alexa.
- Nessuna build Docker: programma Python standard, console offline oppure chat con provider remoto.
- **Il codice completo V1.1 è stato consegnato come ZIP in chat. Questo checkpoint GitHub non equivale alla pubblicazione dei sorgenti V1.1 nel repository.**

## Azione successiva
Verificare una chat reale con provider API su un ambiente privato (chiave solo in variabile ambiente, mai nel repository) e, in seguito, predisporre interfaccia web locale. Mantenere l'operatività di Alexa attuale invariata.

## Esecuzione prova locale
```bash
python -m unittest discover -s tests -v
python agent.py --offline
```
