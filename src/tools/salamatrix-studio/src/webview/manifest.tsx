import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';
import type { ExtensionCommand, ExtensionManifest } from '../manifestModel.js';

declare function acquireVsCodeApi(): { postMessage(message: unknown): void };
const vscode = acquireVsCodeApi();
const runtimes = ['PowerShell', 'Python.CPython', 'JavaScript.Node', 'PHP.CLI', 'Lua', 'Automation.JScript'];

function App(): React.JSX.Element {
  const [manifest, setManifest] = useState<ExtensionManifest>();
  const [error, setError] = useState('');
  useEffect(() => {
    const receive = (event: MessageEvent): void => {
      if (event.data.type === 'document') { setManifest(event.data.manifest as ExtensionManifest); setError(''); }
      if (event.data.type === 'error') setError(String(event.data.message));
    };
    window.addEventListener('message', receive);
    vscode.postMessage({ type: 'ready' });
    return () => window.removeEventListener('message', receive);
  }, []);

  const update = (next: ExtensionManifest): void => {
    setManifest(next);
    vscode.postMessage({ type: 'update', manifest: next });
  };
  if (!manifest) return <main>{error || 'Loading extension manifest…'}</main>;
  const runtime = typeof manifest.runtime === 'string' ? manifest.runtime : manifest.runtime.id ?? '';
  const setRuntime = (value: string): void => update({
    ...manifest,
    runtime: typeof manifest.runtime === 'string' ? value : { ...manifest.runtime, id: value },
  });
  const commands = manifest.commands ?? [];
  const setCommand = (index: number, command: ExtensionCommand): void => update({
    ...manifest,
    commands: commands.map((item, itemIndex) => itemIndex === index ? command : item),
  });
  const move = (index: number, offset: number): void => {
    const target = index + offset;
    if (target < 0 || target >= commands.length) return;
    const next = [...commands];
    [next[index], next[target]] = [next[target]!, next[index]!];
    update({ ...manifest, commands: next });
  };

  return <main>
    <header><div><h1>Extension Designer</h1><p>Visual editor for extension.json and its menus. Source JSON remains available through Open With.</p></div></header>
    {error && <div className="error">{error}</div>}
    <section className="manifest-grid">
      <Field label="Extension ID" value={manifest.id} onChange={(id) => update({ ...manifest, id })} />
      <Field label="Name" value={manifest.name} onChange={(name) => update({ ...manifest, name })} />
      <Field label="Version" value={manifest.version} onChange={(version) => update({ ...manifest, version })} />
      <label>Runtime<select value={runtime} onChange={(event) => setRuntime(event.target.value)}>{runtimes.map((item) => <option key={item}>{item}</option>)}</select></label>
      <Field label="Entry point" value={manifest.entryPoint} onChange={(entryPoint) => update({ ...manifest, entryPoint })} />
      <Field label="Description" value={manifest.description ?? ''} onChange={(description) => update({ ...manifest, description })} />
      <label className="wide">Capabilities (one per line)<textarea value={(manifest.capabilities ?? []).join('\n')} onChange={(event) => update({ ...manifest, capabilities: lines(event.target.value) })} /></label>
    </section>
    <section>
      <div className="section-title"><h2>Commands and menus</h2><button onClick={() => update({ ...manifest, commands: [...commands, newCommand(manifest.id, commands.length)] })}>Add command</button></div>
      <div className="commands">
        {commands.map((command, index) => <article key={`${command.id}-${index}`}>
          <div className="command-title"><strong>{command.title || command.id || `Command ${index + 1}`}</strong><span><button disabled={index === 0} onClick={() => move(index, -1)}>↑</button><button disabled={index === commands.length - 1} onClick={() => move(index, 1)}>↓</button><button className="danger" onClick={() => update({ ...manifest, commands: commands.filter((_, itemIndex) => itemIndex !== index) })}>Delete</button></span></div>
          <div className="command-grid">
            <Field label="Command ID" value={command.id} onChange={(id) => setCommand(index, { ...command, id })} />
            <Field label="Title" value={command.title} onChange={(title) => setCommand(index, { ...command, title })} />
            <Field label="Handler" value={command.handler} onChange={(handler) => setCommand(index, { ...command, handler })} />
            <label>Menu placement<select value={command.menu ?? 'plugin'} onChange={(event) => setCommand(index, { ...command, menu: event.target.value as ExtensionCommand['menu'] })}><option value="plugin">Plugin</option><option value="context">Context</option><option value="both">Both</option><option value="none">None</option></select></label>
            <label>Requires<select value={command.requires ?? ''} onChange={(event) => setCommand(index, { ...command, requires: event.target.value })}><option value="">Default</option><option value="any">Any</option><option value="disk">Disk</option><option value="focused">Focused item</option><option value="file">File</option><option value="selection">Selection</option></select></label>
            <Field label="Executable" value={command.requiresExecutable ?? ''} onChange={(requiresExecutable) => setCommand(index, { ...command, requiresExecutable })} />
            <Field label="Light icon SVG" value={command.icon ?? ''} onChange={(icon) => setCommand(index, { ...command, icon })} />
            <Field label="Dark icon SVG" value={command.iconDark ?? ''} onChange={(iconDark) => setCommand(index, { ...command, iconDark })} />
            <div className="checks"><Check label="Legacy context flag" checked={Boolean(command.contextMenu)} onChange={(contextMenu) => setCommand(index, { ...command, contextMenu })} /><Check label="Toolbar" checked={Boolean(command.toolbar)} onChange={(toolbar) => setCommand(index, { ...command, toolbar, toolbarMenu: toolbar ? command.toolbarMenu : false })} /><Check label="Toolbar menu" checked={Boolean(command.toolbarMenu)} onChange={(toolbarMenu) => setCommand(index, { ...command, toolbarMenu, toolbar: toolbarMenu ? true : command.toolbar })} /><Check label="Enabled" checked={command.enabled !== false} onChange={(enabled) => setCommand(index, { ...command, enabled })} /><Check label="Visible" checked={command.visible !== false} onChange={(visible) => setCommand(index, { ...command, visible })} /></div>
          </div>
        </article>)}
      </div>
    </section>
  </main>;
}

function Field(props: { label: string; value: string; onChange(value: string): void }): React.JSX.Element {
  return <label>{props.label}<input value={props.value} onChange={(event) => props.onChange(event.target.value)} /></label>;
}

function Check(props: { label: string; checked: boolean; onChange(value: boolean): void }): React.JSX.Element {
  return <label className="check"><input type="checkbox" checked={props.checked} onChange={(event) => props.onChange(event.target.checked)} />{props.label}</label>;
}

function newCommand(extensionId: string, index: number): ExtensionCommand {
  return { id: `${extensionId}.command${index + 1}`, title: `Command ${index + 1}`, handler: `command${index + 1}`, menu: 'plugin', contextMenu: false, toolbar: false };
}

function lines(value: string): string[] {
  return value.split(/\r?\n/).map((item) => item.trim()).filter(Boolean);
}

createRoot(document.getElementById('root')!).render(<App />);
