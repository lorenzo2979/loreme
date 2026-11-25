# ALBATROS Multi-STT V2 - stato installazione

**NON INSTALLARE ANCORA SU DIETPI.**

## Verificato
- Repository GitHub, ramo dedicato, senza modifiche a `main`.
- Motore di consenso e unit test di logica, eseguiti localmente (6 casi nel ramo GitHub).
- Test automatici GitHub Actions configurati (verificare il risultato nella scheda Actions).

## Non verificato
- Il repository NON comprende ancora l'implementazione end-to-end Wyoming a quattro STT; il Dockerfile attuale è soltanto un segnaposto per il giudice.
- Build del singolo container con Speech-to-Phrase e Whisper effettivamente eseguiti.
- Addestramento Speech-to-Phrase SENZA leggere dispositivi o entità da Home Assistant.
- Trascrizione reale italiana per tutti e quattro i provider.
- Compatibilità della Google Gemini API con il modello e la quota gratuita del tuo account.
- Prestazioni reali su DietPi, tempi di risposta, consumo RAM.
- Sicurezza, regressione e stabilità di lungo periodo.

## Architettura richiesta
Un solo container, una sola porta Wyoming esposta a Home Assistant; motori in ordine: Speech-to-Phrase locale, Whisper locale, Groq, Google Gemini audio. Il router riceve AUDIO dal trasporto Assist ma non legge le entità, gli altri STT o la configurazione di Home Assistant. L'agente confronta i risultati e restituisce UNA SOLA trascrizione. Groq e Google elaborano audio nel cloud, con consenso esplicito e chiavi locali non pubblicate.

## Protezioni
Mai aggiungere token nel repository. Mantieni spento l'invio cloud fino alla configurazione esplicita. Nessuna modifica al firmware Alexa o alla pipeline Assist esistente prima del test indipendente.

## Documentazione di riferimento
https://github.com/OHF-Voice/speech-to-phrase
https://github.com/OHF-Voice/wyoming
https://github.com/OHF-Voice/wyoming-faster-whisper
