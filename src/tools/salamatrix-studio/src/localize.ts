import * as vscode from 'vscode';

const czech: Record<string, string> = {
  'Overview': 'Přehled',
  'Menu Builder': 'Editor menu',
  'Dialogs': 'Dialogy',
  'Source Files': 'Zdrojové soubory',
  'Open': 'Otevřít',
  'Select or open an extension.json file first.': 'Nejprve vyberte nebo otevřete soubor extension.json.',
  'Create Salamatrix Extension (1/5)': 'Vytvoření Salamatrix extension (1/5)',
  'Create Salamatrix Extension (2/5)': 'Vytvoření Salamatrix extension (2/5)',
  'Create Salamatrix Extension (3/5)': 'Vytvoření Salamatrix extension (3/5)',
  'Create Salamatrix Extension (4/5)': 'Vytvoření Salamatrix extension (4/5)',
  'Create Salamatrix Extension (5/5)': 'Vytvoření Salamatrix extension (5/5)',
  'Extension display name': 'Zobrazovaný název extension',
  'Extension name is required.': 'Název extension je povinný.',
  'Use a dotted identifier such as MyCompany.MyExtension.': 'Použijte tečkovaný identifikátor, například MyCompany.MyExtension.',
  'Unique dotted extension identifier': 'Jedinečný tečkovaný identifikátor extension',
  'Select the extension runtime': 'Vyberte runtime extension',
  'Description (optional)': 'Popis (volitelný)',
  'Select the workspace folder': 'Vyberte složku workspace',
  'Select the parent folder': 'Vyberte nadřazenou složku',
  'Choose the project location': 'Vyberte umístění projektu',
  'Use workspace folder': 'Použít složku workspace',
  'Create a new subfolder': 'Vytvořit novou podsložku',
  'Recommended when the workspace contains other projects': 'Doporučeno, pokud workspace obsahuje další projekty',
  'Extension folder name': 'Název složky extension',
  'Enter one safe folder name.': 'Zadejte jeden platný název složky.',
  'Add Existing Salamatrix Extension Folder': 'Přidat složku existující Salamatrix extension',
  'The selected folder does not contain extension.json.': 'Vybraná složka neobsahuje extension.json.',
  'Extension was not created because these files already exist: {0}': 'Extension nebyla vytvořena, protože již existují tyto soubory: {0}',
  'Created Salamatrix extension {0} for {1}.': 'Salamatrix extension {0} pro {1} byla vytvořena.',
  'Enable Generated Actions': 'Povolit generované akce',
  'Select Dark Command SVG': 'Vyberte tmavou SVG ikonu příkazu',
  'Select Light Command SVG': 'Vyberte světlou SVG ikonu příkazu',
  'SVG images': 'Obrázky SVG',
  'The selected command no longer exists.': 'Vybraný příkaz již neexistuje.',
  'Review the open diff. Enable generated menu actions? Studio will insert one marked dispatch block into {0} and will own {1}. Your existing code outside that block will not be changed.': 'Zkontrolujte otevřený diff. Povolit generované akce menu? Studio vloží jeden označený dispatch blok do {0} a bude spravovat {1}. Váš stávající kód mimo tento blok nezmění.',
  '{0} - Salamatrix Studio menu dispatch preview': '{0} - náhled menu dispatch bloku Salamatrix Studia',
  'Studio will not overwrite the existing unowned file {0}. Move or rename it before enabling generated actions.': 'Studio nepřepíše existující soubor {0}, který nevlastní. Před povolením generovaných akcí ho přesuňte nebo přejmenujte.',
  'Open a Salamatrix dialog design first.': 'Nejprve otevřete návrh Salamatrix dialogu.',
  'Native Preview': 'Nativní náhled',
  'Select the preview color mode': 'Vyberte barevný režim náhledu',
  'Dark': 'Tmavý',
  'Light': 'Světlý',
  'Preview with Salamander dark mode': 'Náhled v tmavém režimu Salamanderu',
  'Preview with modern Windows light mode': 'Náhled v moderním světlém režimu Windows',
  'Cannot start native preview: {0}': 'Nativní náhled nelze spustit: {0}',
};

export function t(text: string): string {
  return vscode.env.language.toLowerCase().startsWith('cs') ? czech[text] ?? text : text;
}

export function tf(text: string, ...values: string[]): string {
  return values.reduce((result, value, index) => result.replaceAll(`{${index}}`, value), t(text));
}
