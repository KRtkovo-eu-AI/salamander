# Digitální podepisování

Tento projekt podepisuje Windows release binárky pomocí Authenticode. Release projekty ve Visual Studiu volají z post-build eventů `tools\codesign\sign_with_retry.cmd`. Tento soubor je záměrně jen malý spouštěcí skript; vlastní implementace pro Certum/SimplySign je vedle něj v `tools\codesign\codesign_certum.cmd`.

## Certifikát

Očekávaný certifikát je **Certum Open Source Code Signing in the Cloud** na SimplySign. Na release stroji nainstalujte a aktivujte:

1. mobilní aplikaci SimplySign,
2. SimplySign Desktop,
3. aktuální Windows SDK včetně nástroje `signtool.exe`.

Před podepsaným release buildem se přihlaste do SimplySign Desktop a ověřte, že `signtool.exe` vidí certifikát v certificate store aktuálního uživatele.

## Povinné prostředí

Podepisování je ve výchozím stavu vypnuté, aby lokální vývojářské release buildy neselhaly bez certifikátu. Na release stroji ho zapněte takto:

```cmd
set CODESIGN_ENABLED=1
```

Volitelné proměnné:

| Proměnná | Výchozí hodnota | Popis |
| --- | --- | --- |
| `CODESIGN_SIGNTOOL` | `signtool.exe` | Plná cesta k `signtool.exe`, pokud není v `PATH`. |
| `CODESIGN_CERT_SUBJECT` | prázdné | Subject certifikátu předaný do `signtool /n`. Hodí se, když je dostupných více certifikátů. |
| `CODESIGN_CERT_SHA1` | prázdné | SHA-1 thumbprint certifikátu předaný do `signtool /sha1`. Má přednost před `CODESIGN_CERT_SUBJECT`. |
| `CODESIGN_TIMESTAMP_URL` | `http://timestamp.digicert.com` | RFC 3161 timestamp server. |
| `CODESIGN_DIGEST_ALGORITHM` | `SHA256` | Digest algoritmus podepisovaného souboru. |
| `CODESIGN_TIMESTAMP_DIGEST_ALGORITHM` | `SHA256` | Digest algoritmus timestampu. |
| `CODESIGN_RETRIES` | `3` | Počet pokusů o podpis. Hodí se kvůli dočasným výpadkům timestamp serverů. |
| `CODESIGN_RETRY_DELAY_SECONDS` | `10` | Pauza mezi pokusy. |
| `CODESIGN_DESCRIPTION` | prázdné | Volitelný popis `/d`, který může zobrazit Windows. |
| `CODESIGN_DESCRIPTION_URL` | prázdné | Volitelná URL `/du`, kterou může zobrazit Windows. |

Pokud není nastavené ani `CODESIGN_CERT_SUBJECT`, ani `CODESIGN_CERT_SHA1`, skript použije `signtool /a` a nechá SignTool vybrat nejlepší dostupný code-signing certifikát.

## Příklad nastavení

```cmd
set CODESIGN_ENABLED=1
set CODESIGN_SIGNTOOL=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe
set CODESIGN_CERT_SUBJECT=Open Source Developer
set CODESIGN_DESCRIPTION=Open Salamander Samandarin
set CODESIGN_DESCRIPTION_URL=https://github.com/KRtkovo-eu-AI/salamander
```

Potom normálně spusťte release build. Každý projekt, který importuje release property sheet s post-build signing eventem, zavolá:

```cmd
tools\codesign\sign_with_retry.cmd "path\to\binary.exe"
```

## Ruční test

Až poběží SimplySign a budou nastavené proměnné prostředí, otestujte podpis jedné binárky ručně:

```cmd
tools\codesign\sign_with_retry.cmd build\x64\Release\salamand.exe
```

Podepsaný soubor ověřte:

```cmd
signtool verify /pa /v build\x64\Release\salamand.exe
```

## Pořadí při release

1. Sestavit release binárky.
2. Podepsat všechny vyprodukované PE binárky (`.exe`, `.dll`, `.spl` a pomocné nástroje).
3. Zabalit installer nebo release archiv.
4. Podepsat finální installer, pokud je to spustitelný installer.
5. Ověřit podpisy finálních release artefaktů.

Podepsaný soubor už neupravujte. Jakákoliv změna po podepsání zneplatní Authenticode podpis.
