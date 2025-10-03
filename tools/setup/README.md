# setup.inf helper scripts

Instalátor z projektu `setup.sln` čte svůj obsah pouze ze souboru
`setup.inf`.  Proto je potřeba každou novou sadu pluginových souborů
ručně přidat do sekcí `[CopyFiles]` a `[CreateDirs]`.  Správa rozsáhlých
managed pluginů (TextViewer, WebView2RenderViewer) je v ruce snadno
chybová, protože obsahují desítky až stovky podpůrných souborů.

Skript `update_setup_inf.py` tento krok automatizuje – projde staging v
`%OPENSAL_BUILD_DIR%\salamander\Release_{x86,x64}\plugins` a vytvoří
položky pro pluginy `jsonviewer`, `textviewer`, `webview2renderviewer`
a `samandarin`.

## Použití

1. Sestavte Salamandera (ideálně obě architektury) tak, aby v
   `%OPENSAL_BUILD_DIR%\salamander\Release_x64\plugins` a
   `%OPENSAL_BUILD_DIR%\salamander\Release_x86\plugins` ležely
   zabalitelné soubory pluginů.
2. Připravte `setup.inf` – můžete začít z oficiálního INF a upravit
   sekce `[Private]`, `[CopyFiles]` atd. podle potřeby.
3. Spusťte skript:

   ```powershell
   python tools\setup\update_setup_inf.py \` \
       --build-root "$env:OPENSAL_BUILD_DIR" \` \
       --setup-inf "$env:OPENSAL_BUILD_DIR\setup\Release_x64\setup.inf"
   ```

   Pokud držíte separátní `setup.inf` i pro x86 instalátor, spusťte
   skript znovu s odpovídající cestou (a případně jiným `--build-root`).

Skript:

- odstraní staré řádky týkající se výše uvedených pluginů,
- doplní nové páry zdroj → cíl do `[CopyFiles]` včetně komentáře
  s architekturou,
- v `[CreateDirs]` založí všechny potřebné podadresáře,
- při prvním zápisu vytvoří zálohu `setup.inf.bak` (pokud nepoužijete
  `--no-backup`).

Výsledný `setup.inf` stačí zkopírovat vedle `setup.exe`/`remove.exe`
a instalátor bude obsahovat nové pluginy včetně jejich managed
závislostí.
