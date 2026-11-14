# Checkpoint CP40 — GW006 browser authentication bridge (14/11/2026)

## Stato effettivo
- Repo: `lorenzo2979/loreme`.
- Branch: `review/gw006-realtek-flash-hal`; **main non modificato**, OpenBeken 1.18.321 non toccato.
- Riferimento del lavoro: `docs/CHECKPOINT_CP39_GW006_2026-11-14.md`.
- Importante: **non esiste ancora un firmware approvato per flashing GW006**.

## Progressione tecnica verificata
Dalla lettura diretta del source bundle WBRG1 firmato SHA256 `1a97c5f571c85224ea5df923bbf114addb3acea8bf1b65cbf35f3d653c168044` è emerso:
- `gwir_sdk_bootstrap.c` richiede un provider board-specific per key/nonce, IR e NOR.
- `gwir_sdk_sta_dashboard.c` è **disabilitato per default**; la variante di prova HTTP serve soltanto asset statici, non autentica né controlla i 96 tasti.
- Il server GWIR già presente accetta pacchetti HMAC sul socket TCP Realtek, con STA gate e owner unico.
- Il linker della build prodotto continua a scartare moduli non raggiungibili; **non aggirare l'audit** abilitando task falsi o forzando `--undefined`.
- Il dump pubblico non certifica la partizione Flash dell'esemplare; `GWIR_WBRG1_STORAGE_APPROVED=0` rimane obbligatorio.

### Codice creato in CP40
1. `firmware/core/gwir_web_rpc.h`
2. `firmware/core/gwir_web_rpc.c`
3. `tests/test_gwir_web_rpc.c`
4. `.github/workflows/gw006-cp40-web-rpc.yml`
5. `scripts/gw006_cp40_runtime_audit.py` e workflow di ispezione `gw006-cp40-runtime-audit.yml`

`gwir_web_rpc_process` è un **ponte di trasporto** tra il futuro endpoint HTTP POST binario e il **già esistente** `gwir_runtime_owner`. Esige un solo record GWAS v1 di lunghezza esatta, verso dispositivo, e un owner già inizializzato con chiave e nonce reali. Non inserisce credenziali, non resetta contatori quando HTTP termina e non espone comandi GWIR non firmati. Raccoglie solo una risposta nel formato GWAS firmato (la firma deve essere verificata dal browser). Scarta richieste/resoconti malformati senza attestare un'operazione riuscita.

**Prova CP40 positiva:** https://github.com/lorenzo2979/loreme/actions/runs/43590870645 — build C con header reali dell'archivio pinned e test host `SUCCESS`.

## Attenzione: non ancora risolto
- Il nuovo ponte **non è ancora connesso al listener HTTP/STA** dell'SDK; il codice è compilabile e testato come modulo isolato. Non dichiarare dashboard funzionante.
- La build prodotto AmebaD completa `43583624928` aveva KM0/KM4 compilati, ma audit ELF finale fallito per 11 moduli non allocati. Tale fallimento non è stato corretto/ignorato da CP40.
- Mancano binding board-specific delle chiavi, nonce persistenti, rete, APConfig, IR pinmux e flash partizione certificata; il motore IR di riferimento GW018 resta non installabile.
- L'interfaccia browser deve includere codice WebCrypto che costruisca e verifichi i record autenticati, e HTTP deve controllare dimensioni, origine, endpoint, isolamento AP/STA e lifecycle; nessun token DuckDNS in AP HTTP.

## Prossimo intervento sulla **vera** integrazione
1. Aggiungere un endpoint HTTP POST binario limitato e solo STA che chiami `gwir_web_rpc_process`, con owner unico (mai nella rete APConfig aperta).
2. Collegarlo all'avvio **dopo** il completamento di `gwir_sdk_secure_start` e soltanto quando esiste un provider per dispositivo, creando un test della catena reale boot → browser → HMAC → IR senza aggirare il linker.
3. Abilitare la dashboard con UI WebCrypto e RPC solo dopo test di autenticazione, anti-replay e richiesta senza credenziali.
4. Convalidare hardware GW006, backup, partizione IR di 4.378.624 byte, pin PA25/PA26 e recovery prima di installare.

**Regola di sicurezza:** nessun merge su main, nessun flash OpenBeken; tutto in review fino ai gate completi.
