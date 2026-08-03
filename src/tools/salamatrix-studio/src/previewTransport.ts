import { DialogDocument, OptionValue } from './model.js';

export function encodePreview(dialog: DialogDocument): string {
  const rows = ['SMXPREVIEW1', hex(dialog.title), `${dialog.width}\t${dialog.height}`];
  for (const control of dialog.controls) {
    const style = numberOption(control.options?.styleFlags);
    const checked = control.options?.checked === true ? 1 : 0;
    rows.push([hex(control.kind), hex(control.id), hex(control.text), control.bounds.x, control.bounds.y, control.bounds.width, control.bounds.height, style, checked, 0].join('\t'));
    for (const [key, value] of Object.entries(control.options ?? {})) {
      rows.push(['O', hex(key), typeof value, hex(String(value))].join('\t'));
    }
    for (const item of control.items ?? []) rows.push(['I', hex(item)].join('\t'));
    for (const column of control.columns ?? []) rows.push(['C', hex(column.title), column.width].join('\t'));
    if (control.selectedIndex !== undefined) rows.push(['S', control.selectedIndex].join('\t'));
    if (control.validation) rows.push(['V', control.validation.required ? 1 : 0, hex(control.validation.message ?? '')].join('\t'));
  }
  return `${rows.join('\n')}\n`;
}

function hex(value: string): string { return Buffer.from(value, 'utf8').toString('hex'); }
function numberOption(value: OptionValue | undefined): number { return typeof value === 'number' ? value : 0; }
