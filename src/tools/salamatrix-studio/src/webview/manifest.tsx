import React, { useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import type { ExtensionCommand, ExtensionManifest } from '../manifestModel.js';
import type { MenuAction, MenuActionKind, MenuDocument } from '../menuModel.js';

declare function acquireVsCodeApi(): { postMessage(message: unknown): void };
const vscode = acquireVsCodeApi();
const runtimes = ['PowerShell', 'Python.CPython', 'JavaScript.Node', 'PHP.CLI', 'Lua', 'Automation.JScript'];
type Section = 'overview' | 'menus';
type PreviewPlacement = 'plugin' | 'context' | 'toolbar';

const czech: Record<string, string> = {
  'Loading extension project…': 'Načítání projektu extension…',
  'Salamatrix extension project': 'Projekt Salamatrix extension',
  'Extension designer sections': 'Části editoru extension',
  'Overview': 'Přehled', 'Menu Builder': 'Editor menu',
  'Extension Overview': 'Přehled extension',
  'Identity, runtime, entry point, and capabilities.': 'Identita, runtime, vstupní bod a capabilities.',
  'Extension ID': 'ID extension', 'Name': 'Název', 'Version': 'Verze', 'Runtime': 'Runtime',
  'Entry point': 'Vstupní bod', 'Description': 'Popis',
  'Capabilities (one per line)': 'Capabilities (jedna na řádek)',
  'Commands, placement, handlers, and generated simple actions.': 'Příkazy, umístění, handlery a generované jednoduché akce.',
  'Add Command': 'Přidat příkaz',
  'Generated simple actions are disabled.': 'Generované jednoduché akce jsou vypnuté.',
  'Custom handlers remain non-invasive. Enable generation only if Program, Open, Command Line, or PowerShell actions are needed.': 'Custom handlery zůstávají neinvazivní. Generování povolte pouze pro akce Program, Otevřít, Příkazový řádek nebo PowerShell.',
  'Enable Generated Actions': 'Povolit generované akce',
  'No commands yet. Add a command to expose an extension action.': 'Zatím nejsou žádné příkazy. Přidáním příkazu zpřístupníte akci extension.',
  'Menu Preview': 'Náhled menu',
  'Preview of visible commands and configured SVG icons.': 'Náhled viditelných příkazů a nastavených SVG ikon.',
  'Dark icons': 'Tmavé ikony', 'Plugin': 'Plugin', 'Context': 'Kontext', 'Toolbar': 'Nástrojová lišta',
  'No commands in this placement': 'V tomto umístění nejsou žádné příkazy',
  'Delete': 'Odstranit', 'Command ID': 'ID příkazu', 'Title': 'Název', 'Handler': 'Handler',
  'Menu placement': 'Umístění v menu', 'Plugin and context': 'Plugin a kontext', 'None': 'Žádné',
  'Requires': 'Požadavky', 'Default': 'Výchozí', 'Any': 'Libovolné', 'Disk': 'Disk',
  'Focused item': 'Aktivní položka', 'File': 'Soubor', 'Selection': 'Výběr', 'Action': 'Akce',
  'Custom handler': 'Custom handler', 'Run program': 'Spustit program', 'Open file or URL': 'Otevřít soubor nebo URL',
  'Command line': 'Příkazový řádek', 'PowerShell script': 'PowerShell skript', 'Target': 'Cíl',
  'Arguments': 'Argumenty', 'Working directory': 'Pracovní složka', 'Required executable': 'Požadovaný spustitelný soubor',
  'Light icon SVG': 'Světlá SVG ikona', 'Dark icon SVG': 'Tmavá SVG ikona', 'Browse…': 'Procházet…',
  'Context flag': 'Příznak kontextového menu', 'Toolbar menu': 'Menu nástrojové lišty',
  'Enabled': 'Povoleno', 'Visible': 'Viditelné', 'Command': 'Příkaz',
};
function tr(text: string): string { return document.documentElement.lang === 'cs' ? czech[text] ?? text : text; }

interface DocumentMessage {
  type: 'document';
  manifest: ExtensionManifest;
  menu: MenuDocument;
  generatedEnabled: boolean;
  initialSection: Section;
  iconSources: { light?: string; dark?: string }[];
}

function App(): React.JSX.Element {
  const [manifest, setManifest] = useState<ExtensionManifest>();
  const [menu, setMenu] = useState<MenuDocument>();
  const [section, setSection] = useState<Section>('overview');
  const [generatedEnabled, setGeneratedEnabled] = useState(false);
  const [icons, setIcons] = useState<DocumentMessage['iconSources']>([]);
  const [error, setError] = useState('');
  const initialized = useRef(false);
  useEffect(() => {
    const receive = (event: MessageEvent): void => {
      if (event.data.type === 'document') {
        const message = event.data as DocumentMessage;
        setManifest(message.manifest); setMenu(message.menu); setGeneratedEnabled(message.generatedEnabled);
        setIcons(message.iconSources); setError('');
        if (!initialized.current) { setSection(message.initialSection); initialized.current = true; }
      }
      if (event.data.type === 'revealSection') setSection(event.data.section as Section);
      if (event.data.type === 'error') setError(String(event.data.message));
    };
    window.addEventListener('message', receive);
    vscode.postMessage({ type: 'ready' });
    return () => window.removeEventListener('message', receive);
  }, []);

  const update = (nextManifest: ExtensionManifest, nextMenu = menu): void => {
    if (!nextMenu) return;
    setManifest(nextManifest); setMenu(nextMenu);
    vscode.postMessage({ type: 'update', manifest: nextManifest, menu: nextMenu });
  };
  if (!manifest || !menu) return <main>{error || tr('Loading extension project…')}</main>;
  const runtime = typeof manifest.runtime === 'string' ? manifest.runtime : manifest.runtime.id ?? '';
  const setRuntime = (value: string): void => update({
    ...manifest, runtime: typeof manifest.runtime === 'string' ? value : { ...manifest.runtime, id: value },
  });

  return <main>
    <header><div><h1>{manifest.name}</h1><p>{tr('Salamatrix extension project')}</p></div></header>
    <nav className="tabs" aria-label={tr('Extension designer sections')}>
      <button className={section === 'overview' ? 'active' : ''} onClick={() => setSection('overview')}>{tr('Overview')}</button>
      <button className={section === 'menus' ? 'active' : ''} onClick={() => setSection('menus')}>{tr('Menu Builder')}</button>
    </nav>
    {error && <div className="error">{error}</div>}
    {section === 'overview'
      ? <Overview manifest={manifest} runtime={runtime} update={update} setRuntime={setRuntime} />
      : <MenuBuilder manifest={manifest} menu={menu} icons={icons} generatedEnabled={generatedEnabled} update={update} />}
  </main>;
}

function Overview(props: {
  manifest: ExtensionManifest; runtime: string;
  update(manifest: ExtensionManifest, menu?: MenuDocument): void; setRuntime(value: string): void;
}): React.JSX.Element {
  const { manifest, update } = props;
  return <section>
    <div className="section-title"><div><h2>{tr('Extension Overview')}</h2><p>{tr('Identity, runtime, entry point, and capabilities.')}</p></div></div>
    <div className="manifest-grid">
      <Field label={tr('Extension ID')} value={manifest.id} onChange={(id) => update({ ...manifest, id })} />
      <Field label={tr('Name')} value={manifest.name} onChange={(name) => update({ ...manifest, name })} />
      <Field label={tr('Version')} value={manifest.version} onChange={(version) => update({ ...manifest, version })} />
      <label>{tr('Runtime')}<select value={props.runtime} onChange={(event) => props.setRuntime(event.target.value)}>{runtimes.map((item) => <option key={item}>{item}</option>)}</select></label>
      <Field label={tr('Entry point')} value={manifest.entryPoint} onChange={(entryPoint) => update({ ...manifest, entryPoint })} />
      <Field label={tr('Description')} value={manifest.description ?? ''} onChange={(description) => update({ ...manifest, description })} />
      <label className="wide">{tr('Capabilities (one per line)')}<textarea value={(manifest.capabilities ?? []).join('\n')} onChange={(event) => update({ ...manifest, capabilities: lines(event.target.value) })} /></label>
    </div>
  </section>;
}

function MenuBuilder(props: {
  manifest: ExtensionManifest; menu: MenuDocument; icons: DocumentMessage['iconSources']; generatedEnabled: boolean;
  update(manifest: ExtensionManifest, menu: MenuDocument): void;
}): React.JSX.Element {
  const { manifest, menu, update } = props;
  const commands = manifest.commands ?? [];
  const [placement, setPlacement] = useState<PreviewPlacement>('plugin');
  const [dark, setDark] = useState(false);
  const changeCommand = (index: number, next: ExtensionCommand): void => {
    const oldHandler = commands[index]?.handler;
    const actions = menu.commands.map((action, actionIndex) => actionIndex === index || action.handler === oldHandler
      ? { ...action, handler: next.handler } : action);
    update({ ...manifest, commands: commands.map((item, itemIndex) => itemIndex === index ? next : item) }, { ...menu, commands: actions });
  };
  const changeAction = (index: number, next: MenuAction): void => update(manifest, {
    ...menu, commands: menu.commands.map((item, itemIndex) => itemIndex === index ? next : item),
  });
  const add = (): void => {
    const command = newCommand(manifest.id, commands.length);
    update({ ...manifest, commands: [...commands, command] }, {
      ...menu, commands: [...menu.commands, { handler: command.handler, action: 'custom' }],
    });
  };
  const remove = (index: number): void => update({ ...manifest, commands: commands.filter((_, itemIndex) => itemIndex !== index) }, {
    ...menu, commands: menu.commands.filter((_, itemIndex) => itemIndex !== index),
  });
  const move = (index: number, offset: number): void => {
    const target = index + offset; if (target < 0 || target >= commands.length) return;
    const nextCommands = [...commands], nextActions = [...menu.commands];
    [nextCommands[index], nextCommands[target]] = [nextCommands[target]!, nextCommands[index]!];
    [nextActions[index], nextActions[target]] = [nextActions[target]!, nextActions[index]!];
    update({ ...manifest, commands: nextCommands }, { ...menu, commands: nextActions });
  };
  return <>
    <section>
      <div className="section-title"><div><h2>{tr('Menu Builder')}</h2><p>{tr('Commands, placement, handlers, and generated simple actions.')}</p></div><button onClick={add}>{tr('Add Command')}</button></div>
      {!props.generatedEnabled && <div className="notice"><div><strong>{tr('Generated simple actions are disabled.')}</strong><p>{tr('Custom handlers remain non-invasive. Enable generation only if Program, Open, Command Line, or PowerShell actions are needed.')}</p></div><button onClick={() => vscode.postMessage({ type: 'enableGeneratedActions' })}>{tr('Enable Generated Actions')}</button></div>}
      <div className="commands">
        {commands.map((command, index) => <CommandCard key={`${command.id}-${index}`} command={command}
          action={menu.commands[index] ?? { handler: command.handler, action: 'custom' }} index={index}
          first={index === 0} last={index === commands.length - 1} onCommand={(next) => changeCommand(index, next)}
          onAction={(next) => changeAction(index, next)} onMove={(offset) => move(index, offset)} onDelete={() => remove(index)} />)}
        {commands.length === 0 && <div className="empty">{tr('No commands yet. Add a command to expose an extension action.')}</div>}
      </div>
    </section>
    <section>
      <div className="section-title"><div><h2>{tr('Menu Preview')}</h2><p>{tr('Preview of visible commands and configured SVG icons.')}</p></div><label className="check"><input type="checkbox" checked={dark} onChange={(event) => setDark(event.target.checked)} />{tr('Dark icons')}</label></div>
      <div className="preview-tabs">{(['plugin', 'context', 'toolbar'] as PreviewPlacement[]).map((item) => <button key={item} className={placement === item ? 'active' : ''} onClick={() => setPlacement(item)}>{tr(titleCase(item))}</button>)}</div>
      <div className={`menu-preview ${dark ? 'dark' : ''}`}>
        {commands.map((command, index) => ({ command, index })).filter(({ command }) => appears(command, placement)).map(({ command, index }) => <div className={`preview-item ${command.enabled === false ? 'disabled' : ''}`} key={command.id}>
          <span className="preview-icon">{(dark ? props.icons[index]?.dark : props.icons[index]?.light) ? <img src={dark ? props.icons[index]?.dark : props.icons[index]?.light} /> : <span>◇</span>}</span>
          <span>{command.title}</span>
        </div>)}
        {!commands.some((command) => appears(command, placement)) && <div className="preview-empty">{tr('No commands in this placement')}</div>}
      </div>
    </section>
  </>;
}

function CommandCard(props: {
  command: ExtensionCommand; action: MenuAction; index: number; first: boolean; last: boolean;
  onCommand(value: ExtensionCommand): void; onAction(value: MenuAction): void; onMove(offset: number): void; onDelete(): void;
}): React.JSX.Element {
  const { command, action } = props;
  return <article>
    <div className="command-title"><strong>{command.title || command.id || `${tr('Command')} ${props.index + 1}`}</strong><span><button disabled={props.first} onClick={() => props.onMove(-1)}>↑</button><button disabled={props.last} onClick={() => props.onMove(1)}>↓</button><button className="danger" onClick={props.onDelete}>{tr('Delete')}</button></span></div>
    <div className="command-grid">
      <Field label={tr('Command ID')} value={command.id} onChange={(id) => props.onCommand({ ...command, id })} />
      <Field label={tr('Title')} value={command.title} onChange={(title) => props.onCommand({ ...command, title })} />
      <Field label={tr('Handler')} value={command.handler} onChange={(handler) => props.onCommand({ ...command, handler })} />
      <label>{tr('Menu placement')}<select value={command.menu ?? 'plugin'} onChange={(event) => props.onCommand({ ...command, menu: event.target.value as ExtensionCommand['menu'] })}><option value="plugin">{tr('Plugin')}</option><option value="context">{tr('Context')}</option><option value="both">{tr('Plugin and context')}</option><option value="none">{tr('None')}</option></select></label>
      <label>{tr('Requires')}<select value={command.requires ?? ''} onChange={(event) => props.onCommand({ ...command, requires: event.target.value })}><option value="">{tr('Default')}</option><option value="any">{tr('Any')}</option><option value="disk">{tr('Disk')}</option><option value="focused">{tr('Focused item')}</option><option value="file">{tr('File')}</option><option value="selection">{tr('Selection')}</option></select></label>
      <label>{tr('Action')}<select value={action.action} onChange={(event) => props.onAction({ ...action, action: event.target.value as MenuActionKind })}><option value="custom">{tr('Custom handler')}</option><option value="program">{tr('Run program')}</option><option value="open">{tr('Open file or URL')}</option><option value="command">{tr('Command line')}</option><option value="powershell">{tr('PowerShell script')}</option></select></label>
      {action.action !== 'custom' && <>
        <Field label={tr('Target')} value={action.target ?? ''} onChange={(target) => props.onAction({ ...action, target })} />
        <Field label={tr('Arguments')} value={action.arguments ?? ''} onChange={(argumentsValue) => props.onAction({ ...action, arguments: argumentsValue })} />
        <Field label={tr('Working directory')} value={action.workingDirectory ?? ''} onChange={(workingDirectory) => props.onAction({ ...action, workingDirectory })} />
      </>}
      <Field label={tr('Required executable')} value={command.requiresExecutable ?? ''} onChange={(requiresExecutable) => props.onCommand({ ...command, requiresExecutable })} />
      <div className="icon-field"><Field label={tr('Light icon SVG')} value={command.icon ?? ''} onChange={(icon) => props.onCommand({ ...command, icon })} /><button onClick={() => vscode.postMessage({ type: 'pickIcon', commandIndex: props.index, dark: false })}>{tr('Browse…')}</button></div>
      <div className="icon-field"><Field label={tr('Dark icon SVG')} value={command.iconDark ?? ''} onChange={(iconDark) => props.onCommand({ ...command, iconDark })} /><button onClick={() => vscode.postMessage({ type: 'pickIcon', commandIndex: props.index, dark: true })}>{tr('Browse…')}</button></div>
      <div className="checks"><Check label={tr('Context flag')} checked={Boolean(command.contextMenu)} onChange={(contextMenu) => props.onCommand({ ...command, contextMenu })} /><Check label={tr('Toolbar')} checked={Boolean(command.toolbar)} onChange={(toolbar) => props.onCommand({ ...command, toolbar, toolbarMenu: toolbar ? command.toolbarMenu : false })} /><Check label={tr('Toolbar menu')} checked={Boolean(command.toolbarMenu)} onChange={(toolbarMenu) => props.onCommand({ ...command, toolbarMenu, toolbar: toolbarMenu ? true : command.toolbar })} /><Check label={tr('Enabled')} checked={command.enabled !== false} onChange={(enabled) => props.onCommand({ ...command, enabled })} /><Check label={tr('Visible')} checked={command.visible !== false} onChange={(visible) => props.onCommand({ ...command, visible })} /></div>
    </div>
  </article>;
}

function Field(props: { label: string; value: string; onChange(value: string): void }): React.JSX.Element {
  return <label>{props.label}<input value={props.value} onChange={(event) => props.onChange(event.target.value)} /></label>;
}
function Check(props: { label: string; checked: boolean; onChange(value: boolean): void }): React.JSX.Element {
  return <label className="check"><input type="checkbox" checked={props.checked} onChange={(event) => props.onChange(event.target.checked)} />{props.label}</label>;
}
function newCommand(extensionId: string, index: number): ExtensionCommand {
  return { id: `${extensionId}.command${index + 1}`, title: `${tr('Command')} ${index + 1}`, handler: `command${index + 1}`, menu: 'plugin', contextMenu: false, toolbar: false, enabled: true, visible: true };
}
function appears(command: ExtensionCommand, placement: PreviewPlacement): boolean {
  if (command.visible === false) return false;
  if (placement === 'toolbar') return Boolean(command.toolbar);
  return command.menu === 'both' || command.menu === placement || (placement === 'context' && Boolean(command.contextMenu));
}
function lines(value: string): string[] { return value.split(/\r?\n/).map((item) => item.trim()).filter(Boolean); }
function titleCase(value: string): string { return value[0]!.toUpperCase() + value.slice(1); }

createRoot(document.getElementById('root')!).render(<App />);
