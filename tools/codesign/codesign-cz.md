# Digitální podepisování

Tento projekt podepisuje Windows release binárky pomocí Authenticode a certifikátu Certum Open Source Code Signing in the Cloud na SimplySign.

Podepisování je záměrně **ruční release krok**. Certum/SimplySign může chtít PIN pro každý podpis, takže podepisování z Visual Studio post-build eventů je ve výchozím stavu vypnuté. Post-build entry point `tools\codesign\sign_with_retry.cmd` skončí bez podpisu, pokud výslovně nenastavíte `CODESIGN_ALLOW_POSTBUILD=1`.

Až budete chtít podepisovat release artefakty, spusťte ručně `tools\codesign\codesign_certum.cmd`.

## Certifikát a nástroje

Na Windows release stroji nainstalujte a aktivujte:

1. mobilní aplikaci SimplySign,
2. SimplySign Desktop,
3. aktuální Windows SDK včetně nástroje `signtool.exe`.

Před podepisováním se přihlaste do SimplySign Desktop a ověřte, že je certifikát vidět v certificate store aktuálního uživatele. V detailu certifikátu zkopírujte thumbprint. Před uložením do `CODESIGN_CERT_SHA1` z něj odstraňte mezery.

## Povinné prostředí

```cmd
set CODESIGN_ENABLED=1
set CODESIGN_CERT_SHA1=THUMBPRINT_CERTUM_CERTIFIKATU_BEZ_MEZER
```

Volitelné proměnné:

| Proměnná | Výchozí hodnota | Popis |
| --- | --- | --- |
| `CODESIGN_SIGNTOOL` | `signtool.exe` | Plná cesta k `signtool.exe`, pokud není v `PATH`. |
| `CODESIGN_TIMESTAMP_URL` | `http://time.certum.pl` | RFC 3161 timestamp server. |
| `CODESIGN_DIGEST_ALGORITHM` | `sha256` | Digest algoritmus podepisovaného souboru. |
| `CODESIGN_TIMESTAMP_DIGEST_ALGORITHM` | `sha256` | Digest algoritmus timestampu. |
| `CODESIGN_RETRIES` | `3` | Počet pokusů o podpis. Hodí se kvůli dočasným výpadkům timestamp serverů. |
| `CODESIGN_RETRY_DELAY_SECONDS` | `10` | Pauza mezi pokusy. |
| `CODESIGN_DESCRIPTION` | prázdné | Volitelný popis `/d`, který může zobrazit Windows. |
| `CODESIGN_DESCRIPTION_URL` | prázdné | Volitelná URL `/du`, kterou může zobrazit Windows. |
| `CODESIGN_ALLOW_POSTBUILD` | prázdné | Nastavte na `1` jen tehdy, pokud opravdu chcete podepisování z Visual Studio post-build eventů. |

## Ruční test jednoho souboru

Tímto nejdřív ověřte, že funguje SimplySign, certifikát, SignTool i náš script:

```cmd
tools\codesign\codesign_certum.cmd --file "H:\_projects\salamander\output\salamander\Release_x64\salamand.exe"
```

Script podepisuje jen soubory `.exe`, `.dll`, `.spl` a `.slg`. Výsledek ověřuje příkazem:

```cmd
signtool verify /pa /all /v "cesta\k\souboru.exe"
```

## Ruční podepsání Inno x64 payloadu

Použijte po naplnění x64 payload adresáře a před sestavením nebo publikováním instalátoru:

```cmd
tools\codesign\codesign_certum.cmd --inno-x64 --payload-dir "H:\_projects\salamander\output\salamander\Release_x64"
```

Pokud `--payload-dir` vynecháte, script použije `%OPENSAL_BUILD_DIR%\salamander\Release_x64`.

Script čte `doc\runbook-setup\inno_setup_salamander_x64.iss` a podepisuje jen soubory, které jsou v tomto instalačním scriptu explicitně uvedené a mají jednu z těchto přípon:

- `.exe`
- `.dll`
- `.spl`

Odpovídající soubory se předají do jednoho volání `signtool sign`, takže PIN-based SimplySign karta by se měla pro celý payload batch zeptat jen jednou. Ověření podpisu pak stále probíhá pro každý podepsaný soubor.

Externí DLL se přeskakují. Exclusion list obsahuje:

- `7za.dll`, `7zwrapper.dll`, `unrar.dll`, `chmlib.dll`, `sqlite.dll`, `libeay32.dll`, `ssleay32.dll`
- `Newtonsoft.Json.dll`, `Markdig.dll`, `PrismSharp.dll`
- `WebView2*.dll`, `System.*.dll`, `Microsoft.Web.WebView2.*.dll`
- VC/UCRT/API-set runtime DLL jako `api-ms-win-*.dll`, `ucrtbase.dll`, `vcruntime140.dll`, `msvcp140.dll`, `concrt140.dll`
- `dbghelp.dll`

## Pořadí při release

1. Sestavit a naplnit release payload adresář.
2. Volitelně otestovat jeden soubor přes `--file`.
3. Podepsat Inno x64 payload přes `--inno-x64`.
4. Sestavit Inno installer.
5. Finální installer podepsat zvlášť přes `--file`.
6. Finální installer ověřit přes `signtool verify /pa /all /v`.

Podepsaný soubor už neupravujte. Jakákoliv změna po podepsání zneplatní Authenticode podpis.
