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
 def test_accelerator_may_move_to_another_letter(self):
  items=slt.parse_items(FIX.read_text(encoding="utf-8-sig").splitlines(keepends=True))
  translated=[{"id":item.key,"text":item.text} for item in items]
  translated[1]["text"]="Použít výchozí &písmo"
  self.assertEqual(slt.validate(items,{"translations":translated})[items[1].key],"Použít výchozí &písmo")
 def test_translation_preserves_format_and_escaping(self):
  os.environ["OPENAI_API_KEY"]="test"
  def requester(payload,key,model): return {"translations":[{"id":x["id"],"text":x["text"].replace("Open","Otevřít").replace("Use","Použít")} for x in payload["items"]]}
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/"out.slt"; slt.translate(FIX,out,"czech","mock",40,False,False,requester); text=out.read_text(encoding="utf-8-sig"); self.assertIn('100,1,"Already translated"',text); self.assertIn('101,1,"Otevřít %s\\n"',text)

 def test_invalid_translation_is_skipped_without_aborting_batch(self):
  os.environ["OPENAI_API_KEY"]="test"
  def requester(payload,key,model):
   rows=[]
   for x in payload["items"]:
    text=x["text"].replace("Open","Otevřít").replace("Use","Použít").replace("&default font","výchozí &písmo")
    if x["resource_id"] == "101": text=text.replace("%s", "")
    rows.append({"id":x["id"],"text":text})
   return {"translations":rows}
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/"out.slt"; report=slt.translate(FIX,out,"czech","mock",40,False,False,requester); text=out.read_text(encoding="utf-8-sig")
   self.assertEqual(report["failed"],1); self.assertEqual(report["translated"],1)
   self.assertIn('101,0,"Open %s\\n"',text); self.assertIn('102,1,"Použít výchozí &písmo"',text)

 def test_single_item_retry_can_recover_rejected_translation(self):
  os.environ["OPENAI_API_KEY"]="test"
  calls=[]
  def requester(payload,key,model):
   calls.append(payload)
   rows=[]
   for x in payload["items"]:
    text=x["text"].replace("Open","Otevřít").replace("Use","Použít").replace("&default font","výchozí &písmo")
    if x["resource_id"] == "101" and not payload.get("retry_instructions"): text=text.replace("%s", "")
    rows.append({"id":x["id"],"text":text})
   return {"translations":rows}
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/"out.slt"; trace=Path(d)/"trace.jsonl"; report=slt.translate(FIX,out,"czech","mock",40,False,False,requester,trace_file=trace); text=out.read_text(encoding="utf-8-sig")
   self.assertEqual(report["failed"],0); self.assertEqual(report["translated"],2)
   self.assertTrue(any(call.get("retry_instructions") for call in calls))
   self.assertIn('101,1,"Otevřít %s\\n"',text); self.assertIn('102,1,"Použít výchozí &písmo"',text)
   self.assertIn('"event": "request"', trace.read_text(encoding="utf-8"))
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
