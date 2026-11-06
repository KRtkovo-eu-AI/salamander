# S12-preflight — WIC thumbnail callback output

## S12-preflight — IMPLEMENTED / lokální brána PASS (2026-10-22)

Samostatná oprava existujícího WIC backendu před obnovením S12. Původní
blob ef72188c53a6be5a713841475f654c96b102885d je shodný v targetu, release
base i donoru 47661e92. AddExtendedPrefixIfNeeded používal SAL_MAX_PATH
místo Win32 MAX_PATH; 415-unit JPEG tak skončil jako Shell bitmapa.
sPVSaveImage navíc odmítal NULL filename a neuměl PVSF_USERDEFINED_OUTPUT.
Oprava používá původní SaveFrame encoder přes seekable memory IStream,
pak kontrolované 64KiB callback writes. File output zachovává původní encoder.
Zrušení funguje před encodingem a při předávání výstupu; nepřerušuje vnitřní
WIC encoding. Celý encoded output je v paměti, limit legacy callbacku DWORD.

Host+PictView+EXIF Debug/Release x64 /warnaserror PASS, EXIF stále v9.
39 native checks/config skutečného prefix helperu a callback adaptéru PASS;
encoder dependency v tomto malém harnessu je stub. Doplňující skutečná
Release pictview.spl přes přesné PDB symboly: 17 checks PASS, dlouhá Unicode
cesta rozpoznána jako JPEG, platný 32x24 callback JPEG nezávisle dekódován,
short-write/cancel/invalid descriptor a běžné JPEG/PNG/TIFF file saves PASS.
Native decoder D/R build a runtime PASS, decoder source contract PASS,
pytest 49 PASS. Původních 52 runner groups zachováno, přidána jediná skupina;
celý runner v tomto předkroku znovu neběžel.

Skutečné GUI Update Thumbnail v obou konfiguracích nad vlastní krátkou
ASCII cestou PASS: 714 -> 1761 bytes, 1005-byte JPEG thumbnail v EXIF IFD1,
orientation 6 zachována, všechny non-APP/COM JPEG segmenty a compressed scan
bajtově shodné. Skutečně načtené plugin/EXIF DLL hashově evidovány, normální
ukončení bez reportu, konfigurace obnoveny, 94 předchozích input files a vlastní
sentinel nezměněny. GUI dlouhých cest patří obnovené S12, není zde tvrzeno.

Evidence evidence/S12-preflight; source-manifest.json, gui-final-validation.json,
actual-release-backend-v2.json, *-loaded-modules.json a bounded command logs.
Source parent 45d4a3e7; runtime patch SHA256
437e6119b04185901f98efcee245431e89c5715018a78dbdcf0c3d7bcbcf8d3c.
S12 donor candidate e22210d1 je odložen mimo Git; jeho starší výsledky nejsou
validační branou tohoto předkroku. S12 musí být obnovena a znovu ověřena.
Předkrok nepushed; full-solution CI/CodeQL pro něj NEPROVEDENY.
