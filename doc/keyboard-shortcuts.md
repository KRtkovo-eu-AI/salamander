# Atalhos de Teclado

Este arquivo consolida os atalhos de teclado documentados no help do projeto para o Open Salamander e plugins incluídos no repositório.

Observacoes:

- A lista abaixo usa como fonte a documentacao de ajuda (`help/src/hh` e `src/plugins/**/help/hh`).
- Comandos de plugins que aceitam atalhos configuraveis pelo usuario nao aparecem com combinacoes fixas, a menos que a propria documentacao defina um atalho padrao.
- Os textos de acao foram mantidos proximos da documentacao original para evitar ambiguidade.

## Core

### Panels

| Atalho | O que faz |
| --- | --- |
| `F1` | Display help contents |
| `F2` | Rename the focused file or directory |
| `F3` | View the focused file |
| `F4` | Edit the focused file |
| `F5` | Copy the selected files and directories |
| `F6` | Move the selected files and directories |
| `F7` | Create the directory in the active panel |
| `F8` | Delete the selected files and directories |
| `F9` | Open the user menu |
| `F10` or `Alt` | Enter or leave Open Salamander's menu bar |
| `F11` | Connect the network drive |
| `F12` | Disconnect the network drive |
| `Ctrl+F1` | Display the Drive Information dialog |
| `Ctrl+F2` | Change attributes of the selected files and directories |
| `Ctrl+F3` | Sort files and directories in the active panel by name. Press again to reverse sort order |
| `Ctrl+F4` | Sort files and directories in the active panel by extension. Press again to reverse sort order |
| `Ctrl+F5` | Sort files and directories in the active panel by time. Press again to reverse sort order |
| `Ctrl+F6` | Sort files and directories in the active panel by size. Press again to reverse sort order |
| `Ctrl+F7` | Change case of the selected files and directories |
| `Ctrl+F9` | Refresh the list of files and directories in the active panel |
| `Ctrl+F10` | Compare directories from active and inactive panel |
| `Ctrl+F11` | Maximize/restore the active panel |
| `Ctrl+F12` | Choose filter for the active panel |
| `Alt+F1` | Change the drive in the left panel |
| `Alt+F2` | Change the drive in the right panel |
| `Alt+F3` | View the focused file in the alternate viewer |
| `Alt+F4` | Exit Open Salamander |
| `Alt+F5` | Pack selected files and directories |
| `Alt+F6` | Unpack selected archive |
| `Alt+F7` | Find files |
| `Alt+F9` | Unpack selected archive |
| `Alt+F10` | Calculate space occupied by the selected files and directories |
| `Alt+F11` | Open List of Opened Files |
| `Alt+F12` | Open List of Working Directories |
| `Shift+F1` | Switch to the What Is This? mode (context help) |
| `Shift+F3` | Open current directory in Windows Explorer |
| `Shift+F4` | Edit the new file |
| `Shift+F7` | Change the current directory |
| `Shift+F8` | Delete the selected files and directories, inverts Recycle Bin settings |
| `Shift+F9` | Open Hot Paths menu |
| `Shift+F10` | Open context menu for selected files and directories or item from the Change Drive menu |
| `Ctrl+Shift+F3` | View the focused file with chosen viewer |
| `Ctrl+Shift+F4` | Edit the focused file with chosen editor |
| `Ctrl+Shift+F5` | Save the selected names |
| `Ctrl+Shift+F6` | Load the selected names |
| `Ctrl+Shift+F9` | Display shared directories |
| `Ctrl+Shift+F10` | Calculates directory sizes in active panel |
| `Ctrl+Shift+F11` | Maximize/restore main window of Open Salamander |
| `Alt+Shift+F10` | Open the context menu for the current directory in active panel |
| `Insert` | Select/unselect the focused file or directory in the active panel |
| `Ctrl+Insert` | Copy the selected files and directories to the clipboard |
| `Alt+Insert` | Insert the full name of the focused file or directory to the clipboard |
| `Alt+Shift+Insert` | Insert the name of the focused file or directory to the clipboard |
| `Ctrl+Alt+Insert` | Insert the full path of the current directory in active panel |
| `Ctrl+Shift+Insert` | Insert the UNC full name of the focused file or directory to the clipboard |
| `Shift+Insert` | Paste files and directories from the clipboard to the current directory or change current directory in panel to path stored as text |
| `Delete` | Delete the selected files and directories |
| `Shift+Delete` | Delete the selected files and directories, inverts Recycle Bin settings |
| `Space` | Select/unselect the focused file or directory in the active panel and calculate directory size |
| `Alt+Space` | Open the window menu of Open Salamander's main window |
| `Ctrl+Space` | Insert the full name of the current directory in the active panel to the Command line |
| `Ctrl+Shift+Space` | Insert the DOS name of the current directory in the active panel to the Command line |
| `Backspace` | Go to the parent directory in the active panel |
| `Ctrl+Backspace` | Go to the root directory (same as `Ctrl+\`) |
| `Shift+Backspace` | Go to the root directory (same as `Ctrl+\`) |
| `Up` | Go to previous file or directory |
| `Down` | Go to next file or directory |
| `Left` | Go to file or directory on the left side or scroll panel to the left |
| `Right` | Go to file or directory on the right side or scroll panel to the right |
| `Shift+Up` | Select or unselect focused item and go to previous item |
| `Shift+Down` | Select or unselect focused item and go to next item |
| `Shift+Left` | Select or unselect items from focused item to the item on the left side |
| `Shift+Right` | Select or unselect items from focused item to the item on the right side |
| `Alt+Left` | Back in directories history |
| `Alt+Right` | Forward in directories history |
| `Alt+Up` | Go to previous selected file or directory |
| `Alt+Down` | Go to next selected file or directory |
| `Ctrl+Up` | Scroll panel up one item without changing focus |
| `Ctrl+Down` | Scroll panel down one item without changing focus |
| `Ctrl+Left` | Scroll panel to the left without changing focus |
| `Ctrl+Right` | Scroll panel to the right without changing focus |
| `Ctrl+Shift+Left/Right` | Open the focused directory or archive or locate the focused file in the other panel |
| `Home` | Go to the first file or directory |
| `End` | Go to the last file or directory |
| `Ctrl+Home` | Go to the first file |
| `Shift+Home` | Select or unselect all items from focused item to the first item |
| `Shift+End` | Select or unselect all items from focused item to the last item |
| `Alt+Home` | Go to the first selected file or directory |
| `Alt+End` | Go to the last selected file or directory |
| `Page Up` | Go up one page |
| `Page Down` | Go down one page |
| `Shift+Page Up` | Select or unselect all items from focused item to the item up one page |
| `Shift+Page Down` | Select or unselect all items from focused item to the item down one page |
| `Ctrl+Page Up` | Go to the parent directory |
| `Ctrl+Page Down` | Go to focused directory or archive |
| `Enter` | Execute focused file or go to focused directory or archive |
| `Ctrl+Enter` | Insert the name of the focused file or directory to the Command line |
| `Ctrl+Shift+Enter` | Insert the DOS name of the focused file or directory to the Command line |
| `Alt+Enter` | Show properties of the focused file or directory |
| `Tab` | Switch to the inactive panel |
| `Ctrl+Tab` | Switch to/from the Command line |
| `Shift+Escape` | Minimize the main window of Open Salamander |
| `Num +` | Select files (and optionally also directories) in the active panel |
| `Ctrl+= (+)` | Select files (and optionally also directories) in the active panel |
| `Ctrl+Num +` | Select all files and directories in the active panel |
| `Num -` | Unselect files (and optionally also directories) in the active panel |
| `Ctrl+-` | Unselect files (and optionally also directories) in the active panel |
| `Ctrl+Num -` | Unselect all files and directories in the active panel |
| `Num *` | Invert the selection on files (and optionally also on directories) in the active panel |
| `Ctrl+Num *` | Invert the selection on all files and directories in the active panel |
| `Num /` | Open a Command Shell window |
| `Ctrl+/` | Open a Command Shell window |
| `Ctrl+\` | Go to the root directory |
| `Shift+Num +` | Select files which have the same extension as focused file |
| `Shift+Num -` | Unselect files which have the same extension as focused file |
| `Ctrl+Shift+Num +` | Select files which have the same name part as focused file |
| `Ctrl+Shift+Num -` | Unselect files which have the same name part as focused file |
| `Ctrl+[` | Insert the full name of the current directory in the left panel to the Command line |
| `Ctrl+Shift+[` | Insert the DOS name of the current directory in the left panel to the Command line |
| `Ctrl+]` | Insert the full name of the current directory in the right panel to the Command line |
| `Ctrl+Shift+]` | Insert the DOS name of the current directory in the right panel to the Command line |
| `Alt+0..9` | Change current view mode in the active panel |
| `Ctrl+Shift+0..9` | Define Hot Path number `0..9` to the current directory |
| `Ctrl+0..9` or `Shift+0..9` | Change the directory in the active panel to Hot Path number `0..9` |
| `Ctrl+Alt+0..9` or `Shift+Alt+0..9` | Change the directory in the other panel to Hot Path number `0..9` |
| `Ctrl+Shift+= (+)` | Add the current directory to Hot Paths |
| `Shift+A..Z` | Change the drive in the active panel to drive `A:`..`Z:` |
| `A..Z` | Quick Search |
| `Alt+A..Z` | Open submenu from the Open Salamander menu using the menu bar hotkeys |
| `Ctrl+A` | Select all files and directories |
| `Ctrl+C` | Copy the selected files and directories to the clipboard |
| `Ctrl+D` | Unselect all files and directories |
| `Ctrl+E` | Send the selected files and directories as email attachment |
| `Ctrl+F` | Find files |
| `Ctrl+G` | Change the current directory |
| `Ctrl+H` | Turn on/off displaying of hidden and system files and directories |
| `Ctrl+I` | Repeat the last plugin command from Plugins menu |
| `Ctrl+K` | Convert contents of selected text files: change character coding and end of lines |
| `Ctrl+L` | Display the Drive Information dialog |
| `Ctrl+M` | Make File List dialog |
| `Ctrl+N` | Turn on/off the smart mode for the Name column width |
| `Ctrl+O` | Security / Owner |
| `Ctrl+P` | Security / Permissions |
| `Ctrl+Q` | Calculate space occupied by the selected files and directories |
| `Ctrl+R` | Refresh the active panel |
| `Ctrl+S` | Paste Shortcut |
| `Ctrl+T` | Go to the target of the focused shortcut (`.lnk` file) |
| `Ctrl+U` | Swap panels |
| `Ctrl+V` | Paste files and directories from the clipboard to the current directory or change current directory to path stored as text |
| `Ctrl+W` | Restore the selection used for the last previous operation |
| `Ctrl+X` | Cut the selected files and directories to the clipboard |
| `Ctrl+;` | Change directory in the active panel to the Documents folder |
| `Ctrl+.` or `Ctrl+Shift+\`` | Change directory in the active panel to the directory from the other panel |
| `Ctrl+,` or `Ctrl+\`` | Open the Network in the active panel or open the Network window for the active panel |

### Command Line

| Atalho | O que faz |
| --- | --- |
| `Escape` | Clear the Command line and switch back to panel |
| `Delete` | Delete selection or character after cursor |
| `Shift+Delete` | Delete selection or character ahead of cursor |
| `Backspace` | Delete selection or character ahead of cursor |
| `Ctrl+Backspace` | Delete whole word to the left |
| `Ctrl+Left` | Move the cursor one word back |
| `Ctrl+Right` | Move the cursor one word forward |
| `Ctrl+Up` | List up in the commands history |
| `Ctrl+Down` | List down in the commands history |
| `Alt+Down` | Drop-down list of commands history |
| `Home` | Go to first character |
| `End` | Go to last character |
| `Enter` | Execute command in the Command line |
| `Alt+Enter` | Execute command in the Command line with inverted Close shell window after command execution option |
| `Shift+Enter` | Same function as `Enter` in the panel |
| `Space`, `Num +`, `Num -`, `Num *`, `Num /` | Insert character `Space`, `+`, `-`, `*`, `/` |
| `A..Z`, `Shift+A..Z` | Insert character `A..Z` |
| `Other keys` | Same function as in panels |

### Internal Viewer

| Atalho | O que faz |
| --- | --- |
| `F2` | Turn on/off line wrapping in text mode |
| `F3` | Find next occurrence |
| `Ctrl+F3` | Find |
| `Shift+F3` | Find previous occurrence |
| `F4` | Switch to hexadecimal mode |
| `Alt+F4` | Close Viewer |
| `F5` | Switch to text mode |
| `F6` | Find next occurrence |
| `Shift+F6` | Find previous occurrence |
| `F7` | Find |
| `F8` | Change characters coding to the next one |
| `Shift+F8` | Change characters coding to the previous one |
| `F11` | Maximize/restore viewer window |
| `Ctrl+A` | Select all |
| `Ctrl+C` | Copy selection to clipboard |
| `Ctrl+F` | Find |
| `Ctrl+G` | Go to offset |
| `Ctrl+H` | Switch to hexadecimal mode |
| `Ctrl+L` | Find next occurrence |
| `Ctrl+N` | Find next occurrence |
| `Ctrl+O` | Open file |
| `Ctrl+P` | Find previous occurrence |
| `Ctrl+R` | Refresh and read the file again |
| `Ctrl+S` | Copy selected text or whole file to file |
| `Ctrl+T` | Switch to text mode |
| `Ctrl+W` | Turn on/off line wrapping in text mode |
| `Escape` | Close Viewer |
| `Up` | Scroll to previous line |
| `Down` | Scroll to next line |
| `Left` | Scroll to previous column |
| `Right` | Scroll to next column |
| `Ctrl+Left` | Scroll to the left by several columns |
| `Ctrl+Right` | Scroll to the right by several columns |
| `Home` | Scroll to file begin |
| `End` | Scroll to file end |
| `Page Up` | Scroll to previous page |
| `Page Down` | Scroll to next page |
| `Ctrl+Home` | Scroll to file begin |
| `Ctrl+End` | Scroll to file end |
| `Ctrl+Page Up` | Scroll to file begin |
| `Ctrl+Page Down` | Scroll to file end |
| `Shift+Up` | Move end of existing selection to previous line in text mode |
| `Shift+Down` | Move end of existing selection to next line in text mode |
| `Shift+Left` | Move end of existing selection to previous column in text mode |
| `Shift+Right` | Move end of existing selection to next column in text mode |
| `Shift+Home` | Move end of existing selection to line begin in text mode |
| `Shift+End` | Move end of existing selection to line end in text mode |
| `Shift+Ctrl+Home` | Move end of existing selection to file begin in both text and hex modes |
| `Shift+Ctrl+End` | Move end of existing selection to file end in both text and hex modes |
| `Ctrl+Insert` | Copy selection to clipboard |
| `Space` | Open the next file from the panel or the Find window |
| `Backspace` | Open the previous file from the panel or the Find window |
| `Ctrl+Space` | Open the next selected file from the panel or the Find window |
| `Ctrl+Backspace` | Open the previous selected file from the panel or the Find window |
| `Shift+Space` | Open the last file from the panel or the Find window |
| `Shift+Backspace` | Open the first file from the panel or the Find window |

## Plugins

### Plugin-specific shortcuts

| Plugin / Contexto | Atalho | O que faz |
| --- | --- | --- |
| `Automation` | `Ctrl+Shift+A` | Automation Open Script Menu |
| `Automation` | `Ctrl+Break` | Abort running script |
| `Checksum` | `Ctrl+Shift+V` | Verify checksum |
| `Database Viewer` | `Ctrl+C` | Copy to clipboard |
| `Database Viewer` | `Ctrl+F` | Find |
| `Database Viewer` | `Ctrl+N` | Find next |
| `Database Viewer` | `Ctrl+P` | Find previous |
| `Database Viewer` | `Ctrl+R` | Refresh |
| `Database Viewer` | `Ctrl+F1` | Properties |
| `Database Viewer` | `Ctrl+I` | Show fields |
| `Database Viewer` | `Ctrl+O` | Open file |
| `Database Viewer` | `F11` | Toggle fullscreen |
| `Demo Menu` | `Ctrl+Shift+M` | Run demo menu test command |
| `Demo Plug` | `Ctrl+Shift+Z` | Enter disk path |
| `Demo View` | `Ctrl+Shift+T` | View bitmap from clipboard |
| `DiskMap` | `Ctrl+Shift+D` | Show disk map |
| `FTP` | `Ctrl+Shift+F` | Connect |
| `FTP` | `Ctrl+F2` | Change attributes |
| `PictView` | `Insert` | Select |
| `PictView` | `Delete` or `Shift+Delete` | Delete |
| `PictView` | `F` | Focus image |
| `PictView` | `F2` | Rename |
| `PictView` | `F3` or `I` | Properties |
| `PictView` | `F11` | Toggle fullscreen |
| `PictView` | `Ctrl+A` | Select all |
| `PictView` | `Ctrl+C` or `Ctrl+Insert` | Copy |
| `PictView` | `Ctrl+D` | Deselect all |
| `PictView` | `Ctrl+H` | Histogram |
| `PictView` | `Ctrl+O` | Open file |
| `PictView` | `Ctrl+P` | Print |
| `PictView` | `Ctrl+R` | Revert / refresh |
| `PictView` | `Ctrl+S` | Save as / copy to file |
| `PictView` | `Ctrl+V` or `Shift+Insert` | Paste |
| `PictView` | `Ctrl+Shift+B` | View bitmap from clipboard |
| `PictView` | `Q` | Screen capture |
| `PictView` | `W` | Scan image |
| `PictView` | `C` | Crop |
| `PictView` | `E` | Show EXIF |
| `PictView` | `X` | Copy to |
| `Regedt` | `F7` | Create new key |
| `Regedt` | `Enter` | Edit value |
| `Renamer` | `Ctrl+Shift+R` | Batch rename |
| `Undelete` | `Ctrl+Shift+U` | Undelete |

### File Comparator

| Atalho | O que faz |
| --- | --- |
| `F2` | Turn on/off caret in text mode |
| `F4` | Turn on/off detailed differences view in text mode |
| `F11` | Toggle fullscreen |
| `M` | Toggle fullscreen |
| `O` | Compare files |
| `Ctrl+O` | Compare files |
| `R` | Recompare currently opened files |
| `Ctrl+R` | Recompare currently opened files |
| `C` | Copy selection to clipboard |
| `Ctrl+C` | Copy selection to clipboard |
| `D` | Turn on/off view mode in which only differences are displayed |
| `Ctrl+D` | Turn on/off view mode in which only differences are displayed |
| `W` | Show white space |
| `Ctrl+W` | Show white space |
| `Ctrl+H` | Split horizontally to top and bottom views |
| `,` | Maximize left (top) file view |
| `Ctrl+,` | Maximize left (top) file view |
| `.` | Maximize right (bottom) file view |
| `Ctrl+.` | Maximize right (bottom) file view |
| `E` | Size left and right (top and bottom) views equally |
| `Ctrl+E` | Size left and right (top and bottom) views equally |
| `H` | Works like Left Arrow |
| `J` | Works like Down Arrow |
| `K` | Works like Up Arrow |
| `L` | Works like Right Arrow |
| `B` | Works like Page Up |
| `F` | Works like Page Down |
| `G` | Scroll to file begin |
| `Shift+G` | Scroll to file end |
| `Alt+D` | Activates differences combo box |
| `Escape` | Close File Comparator or cancel current operation |
| `Page Up` | Scroll to previous page |
| `Page Down` | Scroll to next page |
| `Ctrl+Up` | Scroll to previous line |
| `Ctrl+Down` | Scroll to next line |
| `Ctrl+Home` | Scroll to file begin |
| `Ctrl+End` | Scroll to file end |
| `Ctrl+Page Up` | Scroll to file begin |
| `Ctrl+Page Down` | Scroll to file end |
| `Ctrl+Insert` | Copy selection to clipboard |
| `Alt+Up` | Focus previous difference |
| `Alt+Left` | Focus previous difference |
| `Alt+K` | Focus previous difference |
| `P` | Focus previous difference |
| `Alt+Down` | Focus next difference |
| `Alt+Right` | Focus next difference |
| `Alt+J` | Focus next difference |
| `N` | Focus next difference |
| `Alt+Home` | Focus first difference |
| `Alt+End` | Focus last difference |
| `Tab` | Toggle active file view for caret mode |
| `Ctrl+Tab` | Cycle between maximized left/top, right/bottom, and both views |
| `Ctrl+Shift+Tab` | Cycle between maximized right/bottom, left/top, and both views |
| `Up` | Scroll to previous line when caret is hidden |
| `Down` | Scroll to next line when caret is hidden |
| `Left` | Scroll to previous column when caret is hidden |
| `Right` | Scroll to next column when caret is hidden |
| `Ctrl+Left` | Fast scroll to previous column when caret is hidden |
| `Ctrl+Right` | Fast scroll to next column when caret is hidden |
| `Home` | Scroll to file begin when caret is hidden |
| `End` | Scroll to file end when caret is hidden |
| `Up` | Move caret to previous line when caret is shown |
| `Down` | Move caret to next line when caret is shown |
| `Left` | Move caret one character left when caret is shown |
| `Right` | Move caret one character right when caret is shown |
| `Ctrl+Left` | Move caret to previous word begin/end when caret is shown |
| `Ctrl+Right` | Move caret to next word begin/end when caret is shown |
| `Home` | Move caret to line begin when caret is shown |
| `End` | Move caret to line end when caret is shown |
| `Space` | Focus difference under caret |
| `Shift+Space` | Focus and select difference under caret |

## Notas finais

- O dialogo `Plugin Keyboard Shortcuts` permite atribuir atalhos a comandos de plugins quando o plugin expoe essa capacidade.
- Se precisar, da para transformar este inventario em uma pagina HTML ou em uma tabela dividida por menu/comando em vez de contexto.
