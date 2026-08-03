import React, { useEffect, useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import type {
  ControlCatalog,
  ControlCatalogEntry,
  ControlKind,
  DialogControl,
  DialogDocument,
} from '../model.js';

declare function acquireVsCodeApi<T = unknown>(): {
  postMessage(message: unknown): void;
  getState(): T | undefined;
  setState(state: T): void;
};

const vscode = acquireVsCodeApi<{ dialog?: DialogDocument }>();
const scale = 1.5;

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
        vscode.setState({ dialog: event.data.dialog });
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
    vscode.setState({ dialog: next });
    vscode.postMessage({ type: 'update', dialog: next });
  };

  useEffect(() => {
    const move = (event: PointerEvent): void => {
      const current = drag.current;
      const currentDialog = dialogRef.current;
      if (!current || !currentDialog) return;
      const dx = Math.round((event.clientX - current.startX) / scale);
      const dy = Math.round((event.clientY - current.startY) / scale);
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
        vscode.setState({ dialog: dialogRef.current });
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
  }, []);

  if (error) return <main className="error-panel"><h2>Cannot open dialog</h2><p>{error}</p></main>;
  if (!dialog || !catalog) return <main className="loading">Loading Salamatrix Dialog Designer…</main>;

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
      text: defaultText(entry.kind, entry.title),
      bounds: { x, y, width: entry.defaultWidth, height: entry.defaultHeight },
      options: {},
    };
    commit({ ...dialog, controls: [...dialog.controls, control] });
    setSelectedId(id);
  };

  return (
    <main className="studio">
      <header className="toolbar">
        <label>Title <input value={dialog.title} onChange={(e) => updateDialog({ title: e.target.value })} /></label>
        <label>Width <NumberInput value={dialog.width} onCommit={(width) => updateDialog({ width })} /></label>
        <label>Height <NumberInput value={dialog.height} onCommit={(height) => updateDialog({ height })} /></label>
        <button onClick={() => vscode.postMessage({ type: 'generate' })}>Generate Code</button>
        <button onClick={() => vscode.postMessage({ type: 'generateForRuntime' })}>Generate For…</button>
        <button onClick={() => vscode.postMessage({ type: 'preview' })}>Native Preview</button>
      </header>
      <section className="workspace">
        <aside className="palette">
          <h2>Controls</h2>
          {catalog.controls.map((entry) => (
            <button
              key={entry.kind}
              draggable
              onDragStart={(event) => event.dataTransfer.setData('application/x-salamatrix-control', entry.kind)}
              onDoubleClick={() => addControl(entry)}
              title="Drag to the dialog or double-click"
            >
              {entry.title}
            </button>
          ))}
        </aside>
        <section className="designer-scroll">
          <div
            className="dialog-frame"
            style={{ width: dialog.width * scale, height: dialog.height * scale }}
            onClick={() => setSelectedId(undefined)}
            onDragOver={(event) => event.preventDefault()}
            onDrop={(event) => {
              event.preventDefault();
              const kind = event.dataTransfer.getData('application/x-salamatrix-control') as ControlKind;
              const entry = catalog.controls.find((item) => item.kind === kind);
              if (!entry) return;
              const rect = event.currentTarget.getBoundingClientRect();
              addControl(entry, Math.max(0, Math.round((event.clientX - rect.left) / scale)), Math.max(0, Math.round((event.clientY - rect.top) / scale)));
            }}
          >
            <div className="dialog-title">{dialog.title || 'Untitled dialog'}</div>
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
          <h2>Properties</h2>
          {selected ? (
            <>
              <Property label="Kind"><input value={selected.kind} readOnly /></Property>
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
              <Property label="Text"><input value={selected.text} onChange={(e) => updateControl({ text: e.target.value })} /></Property>
              {(['x', 'y', 'width', 'height'] as const).map((name) => (
                <Property key={name} label={name}>
                  <NumberInput value={selected.bounds[name]} onCommit={(value) => updateControl({ bounds: { ...selected.bounds, [name]: value } })} />
                </Property>
              ))}
              <Property label="Options">
                <JsonCommitInput value={selected.options ?? {}} onCommit={(options) => updateControl({ options })} />
              </Property>
              <Property label="Items">
                <TextCommitArea value={(selected.items ?? []).join('\n')} onCommit={(value) => updateControl({ items: textLines(value) })} />
              </Property>
              <Property label="Columns">
                <JsonCommitInput value={selected.columns ?? []} onCommit={(columns) => updateControl({ columns: columns as DialogControl['columns'] })} />
              </Property>
              <Property label="Selected">
                <NumberInput value={selected.selectedIndex ?? -1} onCommit={(selectedIndex) => updateControl({ selectedIndex })} />
              </Property>
              <Property label="Required">
                <input type="checkbox" checked={Boolean(selected.validation?.required)} onChange={(event) => updateControl({ validation: { ...selected.validation, required: event.target.checked } })} />
              </Property>
              <Property label="Message">
                <input value={selected.validation?.message ?? ''} onChange={(event) => updateControl({ validation: { ...selected.validation, message: event.target.value } })} />
              </Property>
              <button className="danger" onClick={() => {
                commit({ ...dialog, controls: dialog.controls.filter((control) => control.id !== selected.id) });
                setSelectedId(undefined);
              }}>Delete Control</button>
            </>
          ) : <p>Select a control to edit its properties.</p>}
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
    left: control.bounds.x * scale,
    top: control.bounds.y * scale,
    width: control.bounds.width * scale,
    height: control.bounds.height * scale,
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

function ControlPreview({ control }: { control: DialogControl }): React.JSX.Element {
  switch (control.kind) {
    case 'textbox': return <input tabIndex={-1} value={control.text} readOnly />;
    case 'checkbox': return <label><input tabIndex={-1} type="checkbox" checked={Boolean(control.options?.checked)} readOnly />{control.text}</label>;
    case 'radio': return <label><input tabIndex={-1} type="radio" checked={Boolean(control.options?.checked)} readOnly />{control.text}</label>;
    case 'button': case 'arrowbutton': case 'textarrowbutton': case 'colorarrowbutton': return <button tabIndex={-1}>{control.text || 'Button'}</button>;
    case 'combobox': return <select tabIndex={-1}><option>{control.items?.[0] ?? control.text}</option></select>;
    case 'groupbox': return <fieldset><legend>{control.text}</legend></fieldset>;
    case 'progressbar': return <progress value={Number(control.options?.progress ?? 50)} max={100} />;
    case 'listview': case 'treeview': case 'tabcontrol': return <div className="collection-preview">{control.text || control.kind}</div>;
    case 'hyperlink': return <a>{control.text || 'Hyperlink'}</a>;
    default: return <span>{control.text || control.kind}</span>;
  }
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
