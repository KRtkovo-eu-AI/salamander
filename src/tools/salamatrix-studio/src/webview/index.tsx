import React, { useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import type {
  ControlCatalog,
  ControlCatalogEntry,
  ControlKind,
  DialogControl,
  DialogDocument,
} from '../model.js';
import {
  designerTitleBarHeight,
  dialogFramePixels,
  dialogUnitScaleX,
  dialogUnitScaleY,
} from '../dialogGeometry.js';

declare function acquireVsCodeApi<T = unknown>(): {
  postMessage(message: unknown): void;
  getState(): T | undefined;
  setState(state: T): void;
};

type PreviewTheme = 'light' | 'dark';
interface WebviewState { dialog?: DialogDocument; previewTheme?: PreviewTheme }
const vscode = acquireVsCodeApi<WebviewState>();

const czech: Record<string, string> = {
  'Cannot open dialog': 'Dialog nelze otevřít',
  'Loading Salamatrix Dialog Designer…': 'Načítání editoru Salamatrix dialogu…',
  'Title': 'Název', 'Width': 'Šířka', 'Height': 'Výška',
  'Generate Code': 'Generovat kód', 'Generate For…': 'Generovat pro…',
  'Preview': 'Náhled', 'Light': 'Světlý', 'Dark': 'Tmavý', 'Native Preview': 'Nativní náhled',
  'Controls': 'Prvky', 'Properties': 'Vlastnosti',
  'Drag to the dialog or double-click': 'Přetáhněte do dialogu nebo dvakrát klikněte',
  'Untitled dialog': 'Dialog bez názvu', 'Kind': 'Typ', 'Text': 'Text',
  'Options': 'Možnosti', 'Items': 'Položky', 'Columns': 'Sloupce',
  'Selected': 'Vybraná položka', 'Required': 'Povinné', 'Message': 'Zpráva',
  'Delete Control': 'Odstranit prvek', 'Select a control to edit its properties.': 'Vyberte prvek a upravte jeho vlastnosti.',
  'Button': 'Tlačítko', 'Hyperlink': 'Odkaz', 'Progress Bar': 'Ukazatel průběhu',
  'Arrow Button': 'Tlačítko se šipkou', 'Text Arrow Button': 'Textové tlačítko se šipkou',
  'Color Arrow Button': 'Barevné tlačítko se šipkou', 'Toolbar Header': 'Záhlaví panelu nástrojů',
};

function tr(text: string): string {
  return document.documentElement.lang.toLowerCase().startsWith('cs') ? czech[text] ?? text : text;
}

type HostMessage =
  | { type: 'document'; dialog: DialogDocument; catalog: ControlCatalog }
  | { type: 'error'; message: string };

interface DragState {
  mode: 'move' | 'resize';
  id: string;
  startX: number;
  startY: number;
  original: DialogControl;
}

function App(): React.JSX.Element {
  const restored = vscode.getState()?.dialog;
  const [previewTheme, setPreviewTheme] = useState<PreviewTheme>(vscode.getState()?.previewTheme ?? 'dark');
  const [dialog, setDialog] = useState<DialogDocument | undefined>(restored);
  const [catalog, setCatalog] = useState<ControlCatalog | undefined>();
  const [selectedId, setSelectedId] = useState<string | undefined>(undefined);
  const [error, setError] = useState<string | undefined>(undefined);
  const drag = useRef<DragState | undefined>(undefined);
  const dialogRef = useRef<DialogDocument | undefined>(restored);

  useEffect(() => {
    const listener = (event: MessageEvent<HostMessage>): void => {
      if (event.data.type === 'document') {
        dialogRef.current = event.data.dialog;
        setDialog(event.data.dialog);
        setCatalog(event.data.catalog);
        setError(undefined);
        vscode.setState({ dialog: event.data.dialog, previewTheme });
      } else {
        setError(event.data.message);
      }
    };
    window.addEventListener('message', listener);
    vscode.postMessage({ type: 'ready' });
    return () => window.removeEventListener('message', listener);
  }, []);

  const commit = (next: DialogDocument): void => {
    dialogRef.current = next;
    setDialog(next);
    vscode.setState({ dialog: next, previewTheme });
    vscode.postMessage({ type: 'update', dialog: next });
  };

  useEffect(() => {
    const move = (event: PointerEvent): void => {
      const current = drag.current;
      const currentDialog = dialogRef.current;
      if (!current || !currentDialog) return;
      const dx = Math.round((event.clientX - current.startX) / dialogUnitScaleX);
      const dy = Math.round((event.clientY - current.startY) / dialogUnitScaleY);
      const next = {
        ...currentDialog,
        controls: currentDialog.controls.map((control) => {
          if (control.id !== current.id) return control;
          const bounds = current.mode === 'move'
            ? { ...current.original.bounds, x: Math.max(0, current.original.bounds.x + dx), y: Math.max(0, current.original.bounds.y + dy) }
            : { ...current.original.bounds, width: Math.max(4, current.original.bounds.width + dx), height: Math.max(4, current.original.bounds.height + dy) };
          return { ...control, bounds };
        }),
      };
      dialogRef.current = next;
      setDialog(next);
    };
    const up = (): void => {
      if (drag.current && dialogRef.current) {
        vscode.setState({ dialog: dialogRef.current, previewTheme });
        vscode.postMessage({ type: 'update', dialog: dialogRef.current });
      }
      drag.current = undefined;
    };
    window.addEventListener('pointermove', move);
    window.addEventListener('pointerup', up);
    return () => {
      window.removeEventListener('pointermove', move);
      window.removeEventListener('pointerup', up);
    };
  }, [previewTheme]);

  if (error) return <main className="error-panel"><h2>{tr('Cannot open dialog')}</h2><p>{error}</p></main>;
  if (!dialog || !catalog) return <main className="loading">{tr('Loading Salamatrix Dialog Designer…')}</main>;

  const selected = dialog.controls.find((control) => control.id === selectedId);
  const updateDialog = (patch: Partial<DialogDocument>): void => commit({ ...dialog, ...patch });
  const updateControl = (patch: Partial<DialogControl>): void => {
    if (!selected) return;
    commit({
      ...dialog,
      controls: dialog.controls.map((control) => control.id === selected.id ? { ...control, ...patch } : control),
    });
  };

  const addControl = (entry: ControlCatalogEntry, x = 12, y = 12): void => {
    const base = entry.kind.replace(/[^A-Za-z0-9]/g, '') || 'control';
    let index = 1;
    let id = base;
    while (dialog.controls.some((control) => control.id === id)) id = `${base}${++index}`;
    const control: DialogControl = {
      kind: entry.kind,
      id,
      text: defaultText(entry.kind, tr(entry.title)),
      bounds: { x, y, width: entry.defaultWidth, height: entry.defaultHeight },
      options: {},
    };
    commit({ ...dialog, controls: [...dialog.controls, control] });
    setSelectedId(id);
  };

  return (
    <main className={`studio preview-${previewTheme}`}>
      <header className="toolbar">
        <label>{tr('Title')} <input value={dialog.title} onChange={(e) => updateDialog({ title: e.target.value })} /></label>
        <label>{tr('Width')} <NumberInput value={dialog.width} onCommit={(width) => updateDialog({ width })} /></label>
        <label>{tr('Height')} <NumberInput value={dialog.height} onCommit={(height) => updateDialog({ height })} /></label>
        <button onClick={() => vscode.postMessage({ type: 'generate' })}>{tr('Generate Code')}</button>
        <button onClick={() => vscode.postMessage({ type: 'generateForRuntime' })}>{tr('Generate For…')}</button>
        <label>{tr('Preview')}
          <select value={previewTheme} onChange={(event) => {
            const theme = event.target.value as PreviewTheme;
            setPreviewTheme(theme);
            vscode.setState({ dialog: dialogRef.current, previewTheme: theme });
          }}>
            <option value="light">{tr('Light')}</option>
            <option value="dark">{tr('Dark')}</option>
          </select>
        </label>
        <button onClick={() => vscode.postMessage({ type: 'preview', theme: previewTheme })}>{tr('Native Preview')}</button>
      </header>
      <section className="workspace">
        <aside className="palette">
          <h2>{tr('Controls')}</h2>
          {catalog.controls.map((entry) => (
            <button
              key={entry.kind}
              draggable
              onDragStart={(event) => event.dataTransfer.setData('application/x-salamatrix-control', entry.kind)}
              onDoubleClick={() => addControl(entry)}
              title={tr('Drag to the dialog or double-click')}
            >
              {tr(entry.title)}
            </button>
          ))}
        </aside>
        <section className="designer-scroll">
          <div
            className="dialog-frame"
            style={dialogFramePixels(dialog.width, dialog.height)}
            onClick={() => setSelectedId(undefined)}
            onDragOver={(event) => event.preventDefault()}
            onDrop={(event) => {
              event.preventDefault();
              const kind = event.dataTransfer.getData('application/x-salamatrix-control') as ControlKind;
              const entry = catalog.controls.find((item) => item.kind === kind);
              if (!entry) return;
              const rect = event.currentTarget.getBoundingClientRect();
              addControl(
                entry,
                Math.max(0, Math.round((event.clientX - rect.left) / dialogUnitScaleX)),
                Math.max(0, Math.round((event.clientY - rect.top - designerTitleBarHeight) / dialogUnitScaleY)),
              );
            }}
          >
            <div className="dialog-title">{dialog.title || tr('Untitled dialog')}</div>
            <div className="dialog-client">
              {dialog.controls.map((control) => (
                <DesignControl
                  key={control.id}
                  control={control}
                  selected={control.id === selectedId}
                  onSelect={() => setSelectedId(control.id)}
                  onPointerDown={(event, mode) => {
                    event.stopPropagation();
                    setSelectedId(control.id);
                    drag.current = {
                      mode,
                      id: control.id,
                      startX: event.clientX,
                      startY: event.clientY,
                      original: structuredClone(control),
                    };
                  }}
                />
              ))}
            </div>
          </div>
        </section>
        <aside className="properties">
          <h2>{tr('Properties')}</h2>
          {selected ? (
            <>
              <Property label={tr('Kind')}><input value={selected.kind} readOnly /></Property>
              <Property label="ID">
                <TextCommitInput value={selected.id} onCommit={(id) => {
                  if (!/^[A-Za-z][A-Za-z0-9_-]*$/.test(id) || dialog.controls.some((control) => control.id === id && control.id !== selected.id)) return;
                  commit({
                    ...dialog,
                    controls: dialog.controls.map((control) => control.id === selected.id ? { ...control, id } : control),
                  });
                  setSelectedId(id);
                }} />
              </Property>
              <Property label={tr('Text')}><input value={selected.text} onChange={(e) => updateControl({ text: e.target.value })} /></Property>
              {(['x', 'y', 'width', 'height'] as const).map((name) => (
                <Property key={name} label={name}>
                  <NumberInput value={selected.bounds[name]} onCommit={(value) => updateControl({ bounds: { ...selected.bounds, [name]: value } })} />
                </Property>
              ))}
              <Property label={tr('Options')}>
                <JsonCommitInput value={selected.options ?? {}} onCommit={(options) => updateControl({ options })} />
              </Property>
              <Property label={tr('Items')}>
                <TextCommitArea value={(selected.items ?? []).join('\n')} onCommit={(value) => updateControl({ items: textLines(value) })} />
              </Property>
              <Property label={tr('Columns')}>
                <JsonCommitInput value={selected.columns ?? []} onCommit={(columns) => updateControl({ columns: columns as DialogControl['columns'] })} />
              </Property>
              <Property label={tr('Selected')}>
                <NumberInput value={selected.selectedIndex ?? -1} onCommit={(selectedIndex) => updateControl({ selectedIndex })} />
              </Property>
              <Property label={tr('Required')}>
                <input type="checkbox" checked={Boolean(selected.validation?.required)} onChange={(event) => updateControl({ validation: { ...selected.validation, required: event.target.checked } })} />
              </Property>
              <Property label={tr('Message')}>
                <input value={selected.validation?.message ?? ''} onChange={(event) => updateControl({ validation: { ...selected.validation, message: event.target.value } })} />
              </Property>
              <button className="danger" onClick={() => {
                commit({ ...dialog, controls: dialog.controls.filter((control) => control.id !== selected.id) });
                setSelectedId(undefined);
              }}>{tr('Delete Control')}</button>
            </>
          ) : <p>{tr('Select a control to edit its properties.')}</p>}
        </aside>
      </section>
    </main>
  );
}

function DesignControl(props: {
  control: DialogControl;
  selected: boolean;
  onSelect(): void;
  onPointerDown(event: React.PointerEvent, mode: 'move' | 'resize'): void;
}): React.JSX.Element {
  const { control } = props;
  const style: React.CSSProperties = {
    left: control.bounds.x * dialogUnitScaleX,
    top: control.bounds.y * dialogUnitScaleY,
    width: control.bounds.width * dialogUnitScaleX,
    height: control.bounds.height * dialogUnitScaleY,
  };
  return (
    <div
      className={`design-control kind-${control.kind}${props.selected ? ' selected' : ''}`}
      style={style}
      onClick={(event) => { event.stopPropagation(); props.onSelect(); }}
      onPointerDown={(event) => props.onPointerDown(event, 'move')}
    >
      <ControlPreview control={control} />
      {props.selected && <span className="resize-handle" onPointerDown={(event) => props.onPointerDown(event, 'resize')} />}
    </div>
  );
}

const staticTextBold = 0x2;
const staticTextUnderline = 0x4;
const staticTextEndEllipsis = 0x20;
const staticTextPathEllipsis = 0x40;

function ControlPreview({ control }: { control: DialogControl }): React.JSX.Element {
  if (control.kind === 'statictext') {
    const flags = typeof control.options?.styleFlags === 'number' ? control.options.styleFlags : 0;
    const classes = [
      'static-text-preview',
      (flags & staticTextBold) !== 0 ? 'static-text-bold' : '',
      (flags & staticTextUnderline) !== 0 ? 'static-text-underline' : '',
      (flags & staticTextEndEllipsis) !== 0 ? 'static-text-end-ellipsis' : '',
      (flags & staticTextPathEllipsis) !== 0 ? 'static-text-path-ellipsis' : '',
    ].filter(Boolean).join(' ');
    const pathEllipsis = (flags & staticTextPathEllipsis) !== 0;
    return pathEllipsis
      ? <PathEllipsisText text={control.text} className={classes} />
      : <span className={classes}>{control.text}</span>;
  }
  switch (control.kind) {
    case 'textbox': return <input tabIndex={-1} value={control.text} readOnly />;
    case 'checkbox': return <label><input tabIndex={-1} type="checkbox" checked={Boolean(control.options?.checked)} readOnly />{control.text}</label>;
    case 'radio': return <label><input tabIndex={-1} type="radio" checked={Boolean(control.options?.checked)} readOnly />{control.text}</label>;
    case 'button': case 'arrowbutton': case 'textarrowbutton': case 'colorarrowbutton': return <button tabIndex={-1}>{control.text || tr('Button')}</button>;
    case 'combobox': return <select tabIndex={-1}><option>{control.items?.[0] ?? control.text}</option></select>;
    case 'groupbox': return <fieldset><legend>{control.text}</legend></fieldset>;
    case 'progressbar': return <progress value={Number(control.options?.progress ?? 50)} max={100} />;
    case 'listview': case 'treeview': case 'tabcontrol': return <div className="collection-preview">{control.text || control.kind}</div>;
    case 'hyperlink': return <a>{control.text || tr('Hyperlink')}</a>;
    default: return <span>{control.text || control.kind}</span>;
  }
}

function PathEllipsisText({ text, className }: { text: string; className: string }): React.JSX.Element {
  const container = useRef<HTMLSpanElement>(null);
  const [displayText, setDisplayText] = useState(text);

  useLayoutEffect(() => {
    const element = container.current;
    if (!element) return;
    const update = (): void => {
      const width = element.clientWidth;
      if (width <= 0) return;
      const canvas = document.createElement('canvas');
      const context = canvas.getContext('2d');
      if (!context) return;
      context.font = getComputedStyle(element).font;
      if (context.measureText(text).width <= width) {
        setDisplayText(text);
        return;
      }
      const separator = Math.max(text.lastIndexOf('\\'), text.lastIndexOf('/') );
      const suffix = separator >= 0 ? text.slice(separator) : '';
      const prefix = separator >= 0 ? text.slice(0, separator) : text;
      const ellipsis = '...';
      let low = 0;
      let high = prefix.length;
      while (low < high) {
        const middle = Math.ceil((low + high) / 2);
        if (context.measureText(prefix.slice(0, middle) + ellipsis + suffix).width <= width) low = middle;
        else high = middle - 1;
      }
      setDisplayText(prefix.slice(0, low) + ellipsis + suffix);
    };
    update();
    const observer = new ResizeObserver(update);
    observer.observe(element);
    return () => observer.disconnect();
  }, [text]);

  return <span ref={container} className={className}>{displayText}</span>;
}
function Property({ label, children }: { label: string; children: React.ReactNode }): React.JSX.Element {
  return <label className="property"><span>{label}</span>{children}</label>;
}

function NumberInput({ value, onCommit }: { value: number; onCommit(value: number): void }): React.JSX.Element {
  return <input type="number" value={value} onChange={(event) => onCommit(Number(event.target.value))} />;
}

function TextCommitInput({ value, onCommit }: { value: string; onCommit(value: string): void }): React.JSX.Element {
  const [draft, setDraft] = useState(value);
  useEffect(() => setDraft(value), [value]);
  return <input
    value={draft}
    onChange={(event) => setDraft(event.target.value)}
    onBlur={() => onCommit(draft)}
    onKeyDown={(event) => {
      if (event.key === 'Enter') onCommit(draft);
      if (event.key === 'Escape') setDraft(value);
    }}
  />;
}

function TextCommitArea({ value, onCommit }: { value: string; onCommit(value: string): void }): React.JSX.Element {
  const [draft, setDraft] = useState(value);
  useEffect(() => setDraft(value), [value]);
  return <textarea value={draft} onChange={(event) => setDraft(event.target.value)} onBlur={() => onCommit(draft)} />;
}

function JsonCommitInput<T>({ value, onCommit }: { value: T; onCommit(value: T): void }): React.JSX.Element {
  const source = JSON.stringify(value, null, 2);
  const [draft, setDraft] = useState(source);
  const [invalid, setInvalid] = useState(false);
  useEffect(() => { setDraft(source); setInvalid(false); }, [source]);
  const commitJson = (): void => {
    try { onCommit(JSON.parse(draft) as T); setInvalid(false); } catch { setInvalid(true); }
  };
  return <textarea className={invalid ? 'invalid' : ''} value={draft} onChange={(event) => setDraft(event.target.value)} onBlur={commitJson} />;
}

function textLines(value: string): string[] {
  return value.split(/\r?\n/).map((item) => item.trim()).filter(Boolean);
}

function defaultText(kind: ControlKind, title: string): string {
  if (['textbox', 'progressbar', 'listview', 'treeview', 'tabcontrol', 'folderpicker', 'filepicker', 'toolbarheader'].includes(kind)) return '';
  return title;
}

createRoot(document.getElementById('root')!).render(<App />);
