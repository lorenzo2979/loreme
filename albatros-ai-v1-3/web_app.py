"""ALBATROS AI V1.4 — private mobile-friendly browser interface.

Uses the existing V1.3 agent, database and confirmation callback.
Local-only HTTP service; Codespaces ports MUST remain private.
"""
import argparse
from http import HTTPStatus
from http.cookies import SimpleCookie
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import getpass
import hmac
import json
import os
from pathlib import Path
import secrets
import threading
import urllib.parse

from agent import DEFAULT_DB, ask, connect

ROOT = Path(__file__).resolve().parent
MAX_BODY = 8192
UI = (ROOT / "web" / "index.html").read_bytes()


class WebState:
    def __init__(self, password, db_path, provider):
        self.password = password
        self.cookie = secrets.token_urlsafe(32)
        self.db_path = db_path
        self.provider = provider
        self.lock = threading.Lock()
        self.history = []
        self.status = "idle"
        self.answer = ""
        self.pending = None
        self.approved = None
        self.event = threading.Event()
        self.request_id = 0

    def snapshot(self):
        with self.lock:
            return {"status": self.status, "answer": self.answer,
                    "pending": self.pending, "request_id": self.request_id}

    def send(self, question):
        if not isinstance(question, str) or not question.strip() or len(question) > 3000:
            raise ValueError("Messaggio vuoto o troppo lungo.")
        with self.lock:
            if self.status in ("working", "confirmation"):
                raise ValueError("Attendi la risposta o conferma l'operazione precedente.")
            self.request_id += 1
            current = self.request_id
            self.status, self.answer, self.pending = "working", "", None
            self.approved = None
            self.event.clear()
        threading.Thread(target=self._work, args=(question, current), daemon=True).start()
        return current

    def _work(self, question, request_id):
        db = None
        try:
            db = connect(self.db_path)
            def confirm(name, args):
                with self.lock:
                    if self.request_id != request_id:
                        return False
                    self.pending = {"tool": name, "args": args}
                    self.status = "confirmation"
                    self.approved = None
                    self.event.clear()
                if not self.event.wait(180):
                    accepted = False
                else:
                    with self.lock:
                        accepted = self.approved is True
                with self.lock:
                    self.pending = None
                    self.status = "working"
                return accepted
            answer = ask(db, question, provider=self.provider,
                         confirm_write=confirm, history=self.history)
        except Exception as exc:
            # Avoid leaking provider request bodies, tokens or local paths.
            if isinstance(exc, (ValueError, RuntimeError)):
                answer = str(exc)[:250]
            else:
                answer = "Errore durante la richiesta. Controlla il terminale."
        finally:
            if db is not None:
                db.close()
        with self.lock:
            if self.request_id == request_id:
                self.answer, self.status, self.pending = answer, "done", None

    def confirm(self, request_id, approved):
        with self.lock:
            if (self.status != "confirmation" or self.pending is None or
                self.request_id != request_id or self.event.is_set()):
                raise ValueError("Nessuna operazione in attesa di conferma.")
            self.approved = approved
            self.event.set()


class Handler(BaseHTTPRequestHandler):
    server_version = "ALBATROS-Web/1.4"

    @property
    def state(self):
        return self.server.state

    def log_message(self, format, *args):
        # Do not log request payloads, cookies or access credentials.
        return

    def _reply(self, code, data, content_type="application/json; charset=utf-8", cookie=None):
        if isinstance(data, dict):
            data = json.dumps(data, ensure_ascii=False).encode()
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Content-Security-Policy",
                         "default-src 'self'; script-src 'self'; style-src 'self'; "
                         "base-uri 'none'; form-action 'self'; frame-ancestors 'none'")
        if cookie:
            self.send_header("Set-Cookie", cookie)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _authenticated(self):
        cookies = SimpleCookie()
        try:
            cookies.load(self.headers.get("Cookie", ""))
        except Exception:
            return False
        item = cookies.get("albatros_session")
        return item is not None and hmac.compare_digest(item.value, self.state.cookie)

    def _origin_ok(self):
        origin = self.headers.get("Origin", "")
        if not origin:
            return False
        host = self.headers.get("Host", "")
        return origin in ("https://" + host, "http://" + host)

    def _read(self, json_body=True):
        try:
            size = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            raise ValueError("Dimensione non valida")
        if size <= 0 or size > MAX_BODY:
            raise ValueError("Dimensione richiesta non valida")
        data = self.rfile.read(size)
        if json_body:
            if self.headers.get("Content-Type", "").split(";")[0] != "application/json":
                raise ValueError("Formato richiesta non valido")
            return json.loads(data)
        return urllib.parse.parse_qs(data.decode())

    def do_GET(self):
        if self.path == "/":
            if not self._authenticated():
                login = ("""<!doctype html><html lang="it"><meta name="viewport" """
                         """content="width=device-width,initial-scale=1"><meta charset="utf-8">"""
                         """<title>ALBATROS AI — Accesso</title>"""
                         """<style>body{background:#0b1220;color:#e8eef8;font:16px system-ui;"""
                         """display:grid;place-items:center;min-height:85vh}main{max-width:360px;"""
                         """padding:28px;background:#162238;border-radius:18px}"""
                         """input,button{padding:13px;border-radius:9px;font:inherit;width:100%;"""
                         """box-sizing:border-box;margin-top:14px}button{background:#5b9dff;color:#081326;"""
                         """border:0;font-weight:700}</style><main><h1>ALBATROS AI</h1>"""
                         """<p>Inserisci il codice mostrato nel terminale Codespaces.</p>"""
                         """<form action="/login" method="post"><input type="password" """
                         """name="password" autocomplete="off" required placeholder="Codice di accesso">"""
                         """<button>Accedi</button></form></main></html>""").encode()
                return self._reply(200, login, "text/html; charset=utf-8")
            return self._reply(200, UI, "text/html; charset=utf-8")
        if self.path == "/api/state":
            if not self._authenticated():
                return self._reply(401, {"error": "Accesso necessario"})
            return self._reply(200, self.state.snapshot())
        return self._reply(404, {"error": "Pagina non trovata"})

    def do_POST(self):
        if not self._origin_ok():
            return self._reply(403, {"error": "Origine non autorizzata"})
        if self.path == "/login":
            try:
                supplied = self._read(False).get("password", [""])[0]
            except (ValueError, UnicodeError):
                return self._reply(400, {"error": "Richiesta non valida"})
            if not hmac.compare_digest(supplied, self.state.password):
                return self._reply(403, {"error": "Codice errato"})
            self.send_response(303)
            self.send_header("Location", "/")
            self.send_header("Set-Cookie", "albatros_session=" + self.state.cookie +
                             "; HttpOnly; SameSite=Strict; Path=/")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", "0")
            return self.end_headers()
        if not self._authenticated():
            return self._reply(401, {"error": "Accesso necessario"})
        try:
            data = self._read()
            if not isinstance(data, dict):
                raise ValueError("Formato richiesta non valido")
            if self.path == "/api/send":
                result = {"request_id": self.state.send(data.get("question"))}
            elif self.path == "/api/confirm":
                request_id, approved = data.get("request_id"), data.get("approved")
                if type(request_id) is not int or type(approved) is not bool:
                    raise ValueError("Conferma non valida")
                self.state.confirm(request_id, approved)
                result = {"ok": True}
            else:
                return self._reply(404, {"error": "Endpoint sconosciuto"})
            return self._reply(200, result)
        except (ValueError, json.JSONDecodeError) as exc:
            return self._reply(400, {"error": str(exc)[:200]})


def main():
    parser = argparse.ArgumentParser(description="ALBATROS AI web V1.4")
    parser.add_argument("--provider", choices=("gemini", "groq"), default="gemini")
    parser.add_argument("--db", default=str(DEFAULT_DB))
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--api-key-prompt", action="store_true")
    args = parser.parse_args()
    if not (1024 <= args.port <= 65535):
        parser.error("Porta non consentita")
    key_name = "GEMINI_API_KEY" if args.provider == "gemini" else "GROQ_API_KEY"
    if args.api_key_prompt and not os.environ.get(key_name):
        key = getpass.getpass(f"Inserisci {key_name} (nascosta): ").strip()
        if not key:
            parser.error("Chiave API vuota")
        os.environ[key_name] = key
    if not os.environ.get(key_name):
        parser.error("Usa --api-key-prompt per inserire la chiave privata")
    db = connect(args.db)
    db.close()
    password = secrets.token_urlsafe(12)
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    server.state = WebState(password, args.db, args.provider)
    print("\nALBATROS AI V1.4 — interfaccia web privata")
    print(f"Apri la porta {args.port} da Codespaces (visibilità: PRIVATE).")
    print("Codice di accesso temporaneo:", password)
    print("Non condividere il codice né rendere pubblica la porta.\n", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
