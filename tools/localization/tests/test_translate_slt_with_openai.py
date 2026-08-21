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

 def test_spaced_ampersand_is_literal_conjunction(self):
  items=[slt.Item(0,"id","[STRINGTABLE 165]","Settings > Time & Language > Region","14222,","")]
  result={"translations":[{"id":"id","text":"Nastavení > Čas a jazyk > Region"}]}
  self.assertEqual(slt.validate(items,result)["id"],"Nastavení > Čas a jazyk > Region")

 def test_angle_bracketed_ui_text_can_be_translated(self):
  items=[slt.Item(0,"id","[STRINGTABLE 8]","<New name error: %s>","1107,","")]
  result={"translations":[{"id":"id","text":"<Chyba nového názvu: %s>"}]}
  self.assertEqual(slt.validate(items,result)["id"],"<Chyba nového názvu: %s>")
 def test_real_markup_tags_are_still_preserved(self):
  items=[slt.Item(0,"id","[STRINGTABLE 8]","<b>Error: %s</b>","1107,","")]
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"Chyba: %s"}]})

 def test_validate_rejects_replacement_glyphs_and_mojibake(self):
  items=[slt.Item(0,"id","[STRINGTABLE 8]","Open","1107,","")]
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"Otev\ufffdít"}]})
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"PÅ™enosnÃ½"}]})
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"??"}]})

 def test_payload_uses_language_metadata(self):
  os.environ["OPENAI_API_KEY"]="test"
  seen=[]
  def requester(payload,key,model):
   seen.append(payload)
   return {"translations":[{"id":x["id"],"text":"Открыть %s\n" if x["resource_id"] == "101" else "Использовать &шрифт по умолчанию"} for x in payload["items"]]}
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/"out.slt"; slt.translate(FIX,out,"russian","mock",40,False,False,requester)
  self.assertEqual(seen[0]["target_language"],"Russian")
  self.assertEqual(seen[0]["target_locale"],"ru-RU")
  self.assertEqual(seen[0]["target_langid"],1049)
  self.assertEqual(seen[0]["target_script"],"Cyrillic")

 def test_italian_language_metadata(self):
  info=slt.language_info("italian")
  self.assertEqual(info["name"],"Italian")
  self.assertEqual(info["locale"],"it-IT")
  self.assertEqual(info["langid"],1040)
  self.assertEqual(info["script"],"Italian Latin with accents")

 def test_translation_updates_langid_even_without_untranslated_items(self):
  os.environ["OPENAI_API_KEY"]="test"
  content = """[EXPORTINFO]
PROJECTNAME,\"x\"
TEXTVERSION,\"1\"
VERSION,\"1\"

[TRANSLATION]
LANGID,1033
AUTHOR,\"\"
WEB,\"\"
COMMENT,\"\"

[STRINGTABLE 0]
46,1,\"WebView2 渲染查看器 .NET\"
"""
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/"in.slt"; out=Path(d)/"out.slt"
   src.write_text(content,encoding="utf-8-sig")
   def requester(*_): raise AssertionError("no model call expected")
   report=slt.translate(src,out,"chinesesimplified","mock",40,False,False,requester)
   self.assertEqual(report["found"],0)
   self.assertIn("LANGID,2052",out.read_text(encoding="utf-8-sig"))
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

 def test_translation_payload_includes_existing_context(self):
  os.environ["OPENAI_API_KEY"]="test"
  current="""[STRINGTABLE 1]
1,1,"Panely se záložkami"
2,0,"Tabbed panels"
"""
  source="""[STRINGTABLE 1]
1,0,"Tabbed panels"
2,0,"Tabbed panels"
"""
  seen=[]
  def requester(payload,key,model):
   seen.append(payload)
   return {"translations":[{"id":payload["items"][0]["id"],"text":"Panely se záložkami"}]}
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/"source.slt"; inp=Path(d)/"in.slt"; out=Path(d)/"out.slt"
   src.write_text(source,encoding="utf-8-sig"); inp.write_text(current,encoding="utf-8-sig")
   slt.translate(inp,out,"czech","mock",40,False,False,requester,source_archive=src)
  self.assertEqual(seen[0]["existing_translations"],[{"source":"Tabbed panels","translation":"Panely se záložkami"}])
  self.assertEqual(seen[0]["items"][0]["source_text"],"Tabbed panels")

 def test_trim_translations_shortens_only_long_translated_items(self):
  os.environ["OPENAI_API_KEY"]="test"
  current="""[STRINGTABLE 1]
1,1,"Velmi dlouhý přeložený text"
2,0,"Untranslated"
"""
  source="""[STRINGTABLE 1]
1,0,"Short"
2,0,"Untranslated"
"""
  seen=[]
  def requester(payload,key,model):
   seen.append(payload)
   return {"translations":[{"id":payload["items"][0]["id"],"text":"Krát."}]}
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/"source.slt"; inp=Path(d)/"in.slt"; out=Path(d)/"out.slt"
   src.write_text(source,encoding="utf-8-sig"); inp.write_text(current,encoding="utf-8-sig")
   report=slt.translate(inp,out,"czech","mock",40,False,False,requester,source_archive=src,trim_translations=True)
   text=out.read_text(encoding="utf-8-sig")
  self.assertEqual(report["found"],1)
  self.assertEqual(seen[0]["mode"],"trim")
  self.assertEqual(seen[0]["items"][0]["max_length_chars"],5)
  self.assertIn('1,1,"Krát."',text)
  self.assertIn('2,0,"Untranslated"',text)

 def test_requires_key_when_not_dry_run(self):
  os.environ.pop("OPENAI_API_KEY",None)
  with self.assertRaises(RuntimeError): slt.translate(FIX,Path("unused"),"czech","mock",40,False,False)
 def test_dry_run_does_not_require_key_or_call_requester(self):
  os.environ.pop("OPENAI_API_KEY",None)
  def requester(payload,key,model): raise AssertionError("requester should not be called during direct dry-run")
  report=slt.translate(FIX,Path("unused"),"czech","mock",40,True,False,requester)
  self.assertEqual(report["found"],2); self.assertEqual(report["translated"],0)
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
