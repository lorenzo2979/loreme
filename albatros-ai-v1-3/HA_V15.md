# ALBATROS AI V1.5 — Home Assistant in sola lettura (fase 1)

Il modulo `ha_readonly.py` è **isolato** dall'agente e dalla chat web: non accede automaticamente a Home Assistant e non può attuare servizi. Usa esclusivamente GET sugli endpoint documentati `/api/states` e `/api/states/<entity_id>`.

## Primo collaudo — completamente offline

Nel Codespace, dal percorso `albatros-ai-v1-3`:

```bash
python -m unittest discover -s tests -v
python ha_readonly.py
python ha_readonly.py --entity sensor.albatros_demo_temperatura
```

Le due entità `albatros_demo_*` sono simulate. Nessun collegamento alla casa.

## Collegamento reale — NON ancora effettuato

Solo quando l'utente vorrà autorizzare una lettura reale, configurare **nel terminale privato** (mai in chat o in GitHub) le variabili `ALBATROS_HA_URL` e `ALBATROS_HA_TOKEN`; poi avviare `python ha_readonly.py --live --entity sensor.nome_reale`. Per la prima prova utilizzare una singola entità innocua. La connessione dal Codespace alla rete domestica non è garantita; una configurazione locale isolata su DietPi sarà valutata più avanti.

**Attenzione:** un token Home Assistant può consentire anche operazioni di scrittura ad altri client. Il codice di questo modulo invia solo GET, ma ciò non rende il token intrinsecamente di sola lettura. Conservarlo al sicuro e, quando disponibile, limitare i privilegi dell'utente HA usato.

La classe espone `list_states()` e `state(entity_id)`, normalizza le risposte e scarta altri attributi per ridurre l'esposizione di dati. Non sono stati toccati Home Assistant, Alexa, LUMO o Music Assistant. Fase 2 futura: integrare le letture nell'agente AI tramite tool allowlist con filtri sulle entità.
