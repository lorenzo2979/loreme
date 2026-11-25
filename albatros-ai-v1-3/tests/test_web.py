import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch
from web_app import WebState

class WebStateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.state = WebState("private", Path(self.temp.name) / "db.sqlite3", "gemini")

    def tearDown(self):
        self.temp.cleanup()

    def wait(self, wanted):
        until = time.monotonic() + 3
        while time.monotonic() < until:
            x = self.state.snapshot()
            if x["status"] == wanted:
                return x
            time.sleep(.01)
        self.fail("Web did not reach " + wanted)

    def test_chat(self):
        with patch("web_app.ask", return_value="Ciao"):
            self.state.send("ciao")
            self.assertEqual(self.wait("done")["answer"], "Ciao")

    def test_requires_confirm_and_denial(self):
        def fake(db, question, provider, confirm_write, history):
            accepted = confirm_write("add_task", {"project": "ALBATROS AI", "description": "Prova"})
            return "Accettata" if accepted else "Rifiutata"
        with patch("web_app.ask", side_effect=fake):
            req = self.state.send("aggiungi attività")
            pending = self.wait("confirmation")
            self.assertEqual(pending["pending"]["tool"], "add_task")
            with self.assertRaises(ValueError):
                self.state.send("seconda richiesta")
            self.state.confirm(req, False)
            self.assertEqual(self.wait("done")["answer"], "Rifiutata")

    def test_approval(self):
        def fake(db, question, provider, confirm_write, history):
            return "Accettata" if confirm_write("create_project", {"name": "prova"}) else "Rifiutata"
        with patch("web_app.ask", side_effect=fake):
            req = self.state.send("crea progetto")
            self.wait("confirmation")
            self.state.confirm(req, True)
            self.assertEqual(self.wait("done")["answer"], "Accettata")

    def test_invalid(self):
        with self.assertRaises(ValueError):
            self.state.send("")
        with self.assertRaises(ValueError):
            self.state.confirm(1, True)

if __name__ == "__main__":
    unittest.main()
