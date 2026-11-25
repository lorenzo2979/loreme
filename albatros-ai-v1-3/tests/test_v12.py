"""Real request serialization is tested with HTTP mocks; no network or API keys used."""
import io
import json
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
import sys
sys.path.insert(0, str(Path(__file__).parents[1]))
from agent import ask, connect, model_reply, tool_execute

class MockResponse(io.BytesIO):
    def __enter__(self): return self
    def __exit__(self, *args): self.close()

class V12Tests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.db = connect(Path(self.temp.name)/'test.sqlite3')
    def tearDown(self):
        self.db.close(); self.temp.cleanup()
    @patch('agent.model_reply', side_effect=['{"answer":"Ciao!"}', '{"answer":"Ti ho già salutato."}'])
    def test_context_across_turns(self, fake):
        history=[]
        self.assertEqual(ask(self.db,'Saluta', history=history),'Ciao!')
        self.assertEqual(ask(self.db,'Che hai detto prima?', history=history),'Ti ho già salutato.')
        second=fake.call_args_list[-1].args[0]
        self.assertEqual(second[1:4],[{'role':'user','content':'Saluta'},
                                     {'role':'assistant','content':'Ciao!'},
                                     {'role':'user','content':'Che hai detto prima?'}])
    @patch('agent.model_reply', side_effect=['{"answer":"ok"}']*7)
    def test_history_bounded(self, fake):
        history=[]
        for i in range(7): ask(self.db, f'Richiesta {i}',history=history)
        self.assertEqual(len(history),8)
        self.assertEqual(history[0]['content'],'Richiesta 3')
    @patch('agent.model_reply', return_value='{"tool":"forget","args":{"key":"a"}}')
    def test_rejected_mutation_not_written_to_history(self, fake):
        tool_execute(self.db,'remember',{'key':'a','value':'b'})
        history=[]
        result=ask(self.db,'Dimentica a',confirm_write=lambda *_:False,history=history)
        self.assertIn('non autorizzata',result)
        self.assertEqual(history,[])
        self.assertEqual(tool_execute(self.db,'recall',{'key':'a'}),'b')
    def test_gemini_uses_structured_roles_and_system(self):
        seen=[]
        def respond(request, timeout):
            seen.append(json.loads(request.data))
            return MockResponse(json.dumps({'candidates':[{'content':{'parts':[{'text':'{"answer":"ok"}'}]}}]}).encode())
        messages=[{'role':'system','content':'ISTRUZIONI'},
                  {'role':'user','content':'Ciao'},
                  {'role':'assistant','content':'Salve'},
                  {'role':'user','content':'Poi?'}]
        with patch.dict('os.environ', {'GEMINI_API_KEY':'testing-only'}):
            with patch('agent.urllib.request.urlopen',side_effect=respond):
                self.assertEqual(model_reply(messages,'gemini'),'{"answer":"ok"}')
        body=seen[0]
        self.assertEqual(body['systemInstruction']['parts'][0]['text'],'ISTRUZIONI')
        self.assertEqual([m['role'] for m in body['contents']],['user','model','user'])
        self.assertEqual(body['generationConfig']['responseMimeType'],'application/json')
    def test_groq_request_structure(self):
        seen=[]
        def respond(request, timeout):
            seen.append(json.loads(request.data))
            return MockResponse(json.dumps({'choices':[{'message':{'content':'{"answer":"ok"}'}}]}).encode())
        messages=[{'role':'system','content':'ISTRUZIONI'},{'role':'user','content':'Ciao'}]
        with patch.dict('os.environ', {'GROQ_API_KEY':'testing-only'}):
            with patch('agent.urllib.request.urlopen',side_effect=respond):
                self.assertEqual(model_reply(messages,'groq'),'{"answer":"ok"}')
        self.assertEqual(seen[0]['messages'],messages)
    @patch('agent.model_reply',side_effect=['{"tool":"list_projects","args":{}}','{"answer":"Non ci sono progetti"}'])
    def test_read_tool_no_confirmation(self, fake):
        self.assertEqual(ask(self.db,'Elenca i progetti'),'Non ci sono progetti')
        self.assertEqual(fake.call_count,2)

if __name__=='__main__': unittest.main()
