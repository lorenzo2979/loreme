import io
import os
import urllib.error
import unittest
from pathlib import Path
from unittest.mock import patch
from agent import friendly_api_error, model_reply

class V13Tests(unittest.TestCase):
    def test_429_hides_provider_body(self):
        err = urllib.error.HTTPError('https://example.com',429,'restricted',{},io.BytesIO(b'secret API error body'))
        result=friendly_api_error(err)
        self.assertIn('429',result)
        self.assertNotIn('secret',result)
    def test_401_clear_and_safe(self):
        err = urllib.error.HTTPError('https://example.com',401,'restricted',{},None)
        self.assertIn('Chiave API',friendly_api_error(err))
    def test_timeout_message(self):
        self.assertIn('Connessione',friendly_api_error(TimeoutError()))
    def test_gemini_http_error_is_user_friendly(self):
        err = urllib.error.HTTPError('https://example.com',429,'restricted',{},None)
        with patch.dict(os.environ,{'GEMINI_API_KEY':'test-secret'}):
            with patch('agent.urllib.request.urlopen',side_effect=err):
                with self.assertRaises(RuntimeError) as caught:
                    model_reply([{'role':'system','content':'a'},{'role':'user','content':'b'}],'gemini')
        self.assertNotIn('test-secret',str(caught.exception))
        self.assertIn('429',str(caught.exception))
    def test_groq_http_error_is_user_friendly(self):
        err = urllib.error.HTTPError('https://example.com',403,'restricted',{},None)
        with patch.dict(os.environ,{'GROQ_API_KEY':'test-secret'}):
            with patch('agent.urllib.request.urlopen',side_effect=err):
                with self.assertRaises(RuntimeError) as caught:
                    model_reply([{'role':'system','content':'a'},{'role':'user','content':'b'}],'groq')
        self.assertNotIn('test-secret',str(caught.exception))
        self.assertIn('403',str(caught.exception))

if __name__=='__main__': unittest.main()
