import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
import sys
sys.path.insert(0,str(Path(__file__).parents[1]))
from agent import arithmetic, connect, tool_execute, parse_action, ask

class AgentTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.db=connect(Path(self.tmp.name)/"agent.db")
    def tearDown(self):
        self.db.close();self.tmp.cleanup()
    def test_safe_arithmetic(self):
        self.assertEqual(arithmetic("(12+3)*2"),30)
    def test_disallow_code(self):
        for s in ['__import__("os").system("id")','2**100','[1,2]','1/0']:
            with self.assertRaises((ValueError,ZeroDivisionError,OverflowError)):
                arithmetic(s)
    def test_memory_roundtrip(self):
        tool_execute(self.db,"remember",{"key":"progetto","value":"ALBATROS"})
        self.assertEqual(tool_execute(self.db,"recall",{"key":"progetto"}),"ALBATROS")
        self.assertEqual(tool_execute(self.db,"list_memories",{}),["progetto"])
        tool_execute(self.db,"forget",{"key":"progetto"})
        self.assertEqual(tool_execute(self.db,"recall",{"key":"progetto"}),"Memoria non presente.")
    def test_sql_injection_safe(self):
        tool_execute(self.db,"remember",{"key":"x'); drop table memory;--","value":"ok"})
        self.assertEqual(len(tool_execute(self.db,"list_memories",{})),1)
    def test_unknown_tool(self):
        with self.assertRaises(ValueError):tool_execute(self.db,"shell",{})
    def test_parse(self):
        self.assertEqual(parse_action('```json\n{"answer":"ciao"}\n```')["answer"],"ciao")
    @patch("agent.model_reply",side_effect=['{"tool":"calculate","args":{"expression":"2+3"}}','{"answer":"Il risultato è 5"}'])
    def test_end_to_end_tool(self, reply):
        self.assertEqual(ask(self.db,"quanto fa 2+3?"),"Il risultato è 5")
        self.assertEqual(reply.call_count,2)
    @patch("agent.model_reply",return_value='{"tool":"remember","args":{"key":"x","value":"y"}}')
    def test_no_unapproved_memory(self, reply):
        self.assertIn("non autorizzata",ask(self.db,"ricorda",confirm_write=lambda *_:False))
        self.assertEqual(tool_execute(self.db,"list_memories",{}),[])

if __name__=="__main__": unittest.main()
