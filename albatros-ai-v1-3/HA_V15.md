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


## Fase 2 — integrazione dell'agente (codice predisposto; collaudo manuale rinviato)

Gli strumenti Gemini `ha_get_state` e `ha_list_states` sono ora registrati nella allowlist di `agent.py`, quindi possono essere usati dalla chat web V1.4 senza modificarne l'interfaccia. Per impostazione predefinita usano soltanto i sensori simulati.

L'agente riceve un campo `source`: `mock` (simulazione) o `live` (lettura vera); le istruzioni richiedono di distinguere chiaramente i due casi. Nessuna capacità di eseguire servizi Home Assistant è esposta all'AI.

Per le future prove con dispositivi reali serve impostare **esplicitamente** nel terminale privato: `ALBATROS_HA_MODE=live`, `ALBATROS_HA_URL`, `ALBATROS_HA_TOKEN` e `ALBATROS_HA_ENTITIES` (elenco di ID autorizzati, separati da virgole, massimo 30). L'elenco reale usa soltanto richieste GET puntuali per le entità consentite, non l'inventario completo. Mai committare credenziali o indirizzi domestici nel repository.

Il collaudo effettivo da Codespaces/chat web è **rinviato su richiesta dell'utente**: non segnare questa fase come verificata manualmente. Sono stati aggiunti test di regressione in `tests/test_ha_agent.py`, da includere nel collaudo finale.
