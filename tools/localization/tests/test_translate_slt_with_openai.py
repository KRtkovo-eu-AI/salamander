import importlib.util, os, sys, tempfile, unittest, urllib.error
from pathlib import Path
P=Path(__file__).parents[1]/"translate_slt_with_openai.py"; spec=importlib.util.spec_from_file_location("slt",P); slt=importlib.util.module_from_spec(spec); sys.modules["slt"]=slt; spec.loader.exec_module(slt)
FIX=Path(__file__).parent/"fixtures/sample.slt"
class Tests(unittest.TestCase):
 def test_parser_uses_state_and_preserves_translated(self):
  lines=FIX.read_text(encoding="utf-8-sig").splitlines(keepends=True); self.assertEqual([i.key.split(':')[1] for i in slt.parse_items(lines)],["101","102"])
 def test_validate_rejects_changed_tokens_and_incomplete(self):
  items=slt.parse_items(FIX.read_text(encoding="utf-8-sig").splitlines(keepends=True))
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":items[0].key,"text":"Otevřít"}]})
 def test_translation_preserves_format_and_escaping(self):
  os.environ["OPENAI_API_KEY"]="test"
  def requester(payload,key,model): return {"translations":[{"id":x["id"],"text":x["text"].replace("Open","Otevřít").replace("Use","Použít")} for x in payload["items"]]}
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/"out.slt"; slt.translate(FIX,out,"czech","mock",40,False,False,requester); text=out.read_text(encoding="utf-8-sig"); self.assertIn('100,1,"Already translated"',text); self.assertIn('101,1,"Otevřít %s\\n"',text)
 def test_requires_key(self):
  os.environ.pop("OPENAI_API_KEY",None)
  with self.assertRaises(RuntimeError): slt.translate(FIX,Path("unused"),"czech","mock",40,True,False)
 def test_retry(self):
  calls=[]
  old=slt.urllib.request.urlopen
  def fake(*a,**k): calls.append(1); raise urllib.error.HTTPError("x",429,"rate",{},None)
  slt.urllib.request.urlopen=fake
  try:
   with self.assertRaises(urllib.error.HTTPError): slt.request_openai({},"x","m",attempts=2,sleep=lambda _:None)
   self.assertEqual(len(calls),2)
  finally: slt.urllib.request.urlopen=old
if __name__=="__main__": unittest.main()
