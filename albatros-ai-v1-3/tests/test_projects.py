import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
import sys
sys.path.insert(0,str(Path(__file__).parents[1]))
from agent import connect,tool_execute,ask,parse_action

class ProjectTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.path=Path(self.temp.name)/'agent.db'
        self.db=connect(self.path)
    def tearDown(self):
        self.db.close()
        self.temp.cleanup()
    def test_project_task_lifecycle(self):
        tool_execute(self.db,'create_project',{'name':'ALBATROS AI','description':'Agente intelligente'})
        self.assertEqual(tool_execute(self.db,'list_projects',{})[0]['name'],'ALBATROS AI')
        task=tool_execute(self.db,'add_task',{'project':'ALBATROS AI','description':'Testare Gemini'})
        self.assertEqual(task['id'],1)
        self.assertFalse(tool_execute(self.db,'list_tasks',{'project':'ALBATROS AI'})[0]['done'])
        tool_execute(self.db,'complete_task',{'id':1})
        self.assertTrue(tool_execute(self.db,'list_tasks',{'project':'ALBATROS AI'})[0]['done'])
        self.db.close()
        self.db=connect(self.path)
        self.assertTrue(tool_execute(self.db,'list_tasks',{'project':'ALBATROS AI'})[0]['done'])
    def test_missing_project_fails(self):
        with self.assertRaises(Exception):
            tool_execute(self.db,'add_task',{'project':'non esiste','description':'x'})
    def test_reject_bad_task_ids(self):
        for task_id in (-1,0,'1',True):
            with self.assertRaises(ValueError):
                tool_execute(self.db,'complete_task',{'id':task_id})
    def test_reject_invalid_names(self):
        with self.assertRaises(ValueError):
            tool_execute(self.db,'create_project',{'name':'\n','description':'ok'})
    @patch('agent.model_reply',return_value='{"tool":"create_project","args":{"name":"X","description":"Y"}}')
    def test_project_writes_need_confirmation(self,reply):
        self.assertIn('non autorizzata',ask(self.db,'Crea un progetto',confirm_write=lambda *_:False))
        self.assertEqual(tool_execute(self.db,'list_projects',{}),[])
    def test_reject_ambiguous_model_actions(self):
        for value in ('{"answer":"ciao","tool":"forget","args":{"key":"x"}}',
                      '{"tool":"calculate","args":"1+1"}',
                      '{"answer":18}'):
            with self.assertRaises(ValueError):
                parse_action(value)

if __name__=='__main__':unittest.main()
