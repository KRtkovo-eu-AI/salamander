import importlib.util, io, json, os, shutil, sys, tempfile, unittest, urllib.error
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

 def test_literal_quotes_are_valid_slt_text(self):
  items=[slt.Item(0,"id","[STRINGTABLE 8]",'Show "%s"',"1107,","")]
  result={"translations":[{"id":"id", "text":'Zobraz "%s"'}]}
  self.assertEqual(slt.validate(items,result)["id"],'Zobraz "%s"')

 def test_ftp_confirmation_dialog_quotes_are_valid_slt_text(self):
  lines=[
   "[DIALOG 535]\n",
   '339,218,1,"Conferme"\n',
   '536,5,5,317,12,0,"&Show message "You are leaving server. Do you wish to disconnect or to keep connection?""\n',
   '537,5,18,164,12,0,"S&how message "Do you want to disconnect?""\n',
   '538,5,31,234,12,0,"Sh&ow message "Connection is closed. Do you want to reconnect?""\n',
   '539,5,44,325,12,0,"Sho&w message "Target name is already used. Do you want to overwrite it?" in Quick Rename"\n',
   '540,5,57,202,12,0,"Show &message "Connection to FTP server has been lost""\n',
   '541,5,70,321,12,0,"Show m&essage "If you always want to list hidden files and directories on this FTP server, ...""\n',
  ]
  items=slt.parse_items(lines,force=True)
  self.assertEqual(len(items),7)
  result={"translations":[{"id":item.key,"text":item.text} for item in items]}
  self.assertEqual(len(slt.validate(items,result)),7)

 def test_validate_rejects_replacement_glyphs_and_mojibake(self):
  items=[slt.Item(0,"id","[STRINGTABLE 8]","Open","1107,","")]
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"Otev\ufffdít"}]})
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"PÅ™enosnÃ½"}]})
  with self.assertRaises(ValueError): slt.validate(items,{"translations":[{"id":"id","text":"??"}]})

 def test_payload_uses_language_metadata(self):
  os.environ["CURSOR_API_KEY"]="test"
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
  os.environ["CURSOR_API_KEY"]="test"
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
  os.environ["CURSOR_API_KEY"]="test"
  def requester(payload,key,model): return {"translations":[{"id":x["id"],"text":x["text"].replace("Open","Otevřít").replace("Use","Použít")} for x in payload["items"]]}
  with tempfile.TemporaryDirectory() as d:
   out=Path(d)/"out.slt"; slt.translate(FIX,out,"czech","mock",40,False,False,requester); text=out.read_text(encoding="utf-8-sig"); self.assertIn('100,1,"Already translated"',text); self.assertIn('101,1,"Otevřít %s\\n"',text)

 def test_invalid_translation_is_skipped_without_aborting_batch(self):
  os.environ["CURSOR_API_KEY"]="test"
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
  os.environ["CURSOR_API_KEY"]="test"
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
  os.environ["CURSOR_API_KEY"]="test"
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
  os.environ["CURSOR_API_KEY"]="test"
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

 def test_windows_os_blocking_compat_shims_missing_functions(self):
  slt._patch_windows_os_blocking()
  self.assertTrue(callable(os.get_blocking))
  self.assertTrue(callable(os.set_blocking))
 def test_cursor_session_falls_back_to_cli_after_get_blocking(self):
  class BrokenAgent:
   def send(self, prompt): raise AttributeError("module 'os' has no attribute 'get_blocking'")
  calls=[]
  def fake_run(*a,**k):
   calls.append(a[0]); return type("R",(),{"returncode":0,"stdout":json.dumps({"translations":[]}),"stderr":""})()
  session=slt.CursorSession.__new__(slt.CursorSession)
  session.api_key="k"; session.model="grok-4.5"; session._workdir=tempfile.mkdtemp(); session._agent=BrokenAgent(); session._created=None; session._cli=None
  old_find, old_run = slt.find_cursor_cli, slt.subprocess.run
  slt.find_cursor_cli=lambda: "agent.exe"; slt.subprocess.run=fake_run
  try:
   text=session._complete("hello")
   self.assertEqual(json.loads(text),{"translations":[]}); self.assertIsNone(session._agent); self.assertTrue(calls)
  finally:
   slt.find_cursor_cli=old_find; slt.subprocess.run=old_run; shutil.rmtree(session._workdir, ignore_errors=True)
  os.environ.pop("CURSOR_API_KEY",None)
  os.environ.pop("OPENAI_API_KEY",None)
  with self.assertRaises(RuntimeError): slt.translate(FIX,Path("unused"),"czech","mock",40,False,False)
 def test_dry_run_does_not_require_key_or_call_requester(self):
  os.environ.pop("CURSOR_API_KEY",None)
  os.environ.pop("OPENAI_API_KEY",None)
  def requester(payload,key,model): raise AssertionError("requester should not be called during direct dry-run")
  report=slt.translate(FIX,Path("unused"),"czech","mock",40,True,False,requester)
  self.assertEqual(report["found"],2); self.assertEqual(report["translated"],0)
 def test_parse_json_object_accepts_fences_and_cli_wrappers(self):
  payload={"translations":[{"id":"a","text":"Ahoj"}]}
  self.assertEqual(slt.parse_json_object("```json\n"+json.dumps(payload)+"\n```"),payload)
  self.assertEqual(slt.parse_json_object(json.dumps([{"id":"a","text":"Ahoj"}])),payload)
  self.assertEqual(slt.parse_json_object(json.dumps({"type":"result","result":json.dumps(payload)})),payload)
 def test_default_cursor_model(self):
  old_cursor=os.environ.pop("CURSOR_MODEL",None); old_openai=os.environ.pop("OPENAI_MODEL",None); old_openrouter=os.environ.pop("OPENROUTER_MODEL",None)
  try:
   self.assertEqual(slt.DEFAULT_CURSOR_MODEL,"grok-4.5")
   self.assertEqual(slt.default_model("cursor"),"grok-4.5")
   self.assertEqual(slt.DEFAULT_OPENROUTER_MODEL,"openai/gpt-5.4-nano")
   self.assertEqual(slt.default_model("openrouter"),"openai/gpt-5.4-nano")
  finally:
   if old_cursor is not None: os.environ["CURSOR_MODEL"]=old_cursor
   if old_openai is not None: os.environ["OPENAI_MODEL"]=old_openai
   if old_openrouter is not None: os.environ["OPENROUTER_MODEL"]=old_openrouter

 def test_openrouter_request_uses_chat_completions_and_schema(self):
  payload={"target_language":"Czech","items":[]}
  response_body=json.dumps({"choices":[{"message":{"content":json.dumps({"translations":[]})}}]}).encode()
  seen=[]
  class Response(io.BytesIO):
   def __enter__(self): return self
   def __exit__(self,*_): pass
  def fake(request, timeout):
   seen.append((request, timeout))
   return Response(response_body)
  old=slt.urllib.request.urlopen; slt.urllib.request.urlopen=fake
  try:
   result=slt.request_openrouter(payload,"secret","openai/gpt-5.4-nano",attempts=1)
  finally:
   slt.urllib.request.urlopen=old
  self.assertEqual(result,{"translations":[]})
  request,timeout=seen[0]
  self.assertEqual(request.full_url,"https://openrouter.ai/api/v1/chat/completions")
  self.assertEqual(request.headers["Authorization"],"Bearer secret")
  body=json.loads(request.data.decode("utf-8"))
  self.assertEqual(body["model"],"openai/gpt-5.4-nano")
  self.assertEqual(body["response_format"]["type"],"json_schema")
  self.assertEqual(body["response_format"]["json_schema"]["name"],"translations")
  self.assertTrue(body["provider"]["require_parameters"])
  self.assertEqual(timeout,300)
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
