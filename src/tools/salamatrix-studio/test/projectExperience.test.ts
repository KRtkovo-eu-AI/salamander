/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-04 | Cover the Open Samandarin description and the UI Capabilities demo command. | The empty Studio workspace must offer the bundled dialog gallery. | https://github.com/KRtkovo-eu-AI/salamander/issues/776 | Grok 4.7 |
*/
import { readFileSync } from 'node:fs';
import { describe, expect, it } from 'vitest';

describe('empty workspace and project navigation contributions', () => {
  const packageJson = JSON.parse(readFileSync('package.json', 'utf8'));
  const englishStrings = JSON.parse(readFileSync('package.nls.json', 'utf8'));
  const czechStrings = JSON.parse(readFileSync('package.nls.cs.json', 'utf8'));

  it('activates in an empty workspace and offers project creation', () => {
    expect(packageJson.activationEvents).toContain('onView:salamatrixStudio.projectExplorer');
    expect(packageJson.contributes.commands.some((item: { command: string }) => item.command === 'salamatrixStudio.createExtension')).toBe(true);
    expect(packageJson.contributes.viewsWelcome[0].contents).toBe('%welcome.noProjects%');
    expect(englishStrings['welcome.noProjects']).toContain('Create New Extension');
    expect(englishStrings['welcome.noProjects']).toContain('command:salamatrixStudio.openUiCapabilitiesDemo');
    expect(czechStrings['welcome.noProjects']).toContain('Vytvořit novou extension');
    expect(czechStrings['welcome.noProjects']).toContain('Otevřít ukázku UI Capabilities');
    expect(englishStrings.description).toBe('Developer tools and visual designers for Open Samandarin extensions.');
    expect(czechStrings.description).toBe('Vývojářské nástroje a vizuální editory extensions pro Open Samandarin.');
    expect(packageJson.version).toBe('0.1.12');
    expect(packageJson.activationEvents).toContain('onCommand:salamatrixStudio.openUiCapabilitiesDemo');
  });

  it('keeps the UI Capabilities demo available from the project view', () => {
    const item = packageJson.contributes.menus['view/title'].find(
      (entry: { command: string }) => entry.command === 'salamatrixStudio.openUiCapabilitiesDemo',
    );
    expect(item.when).toBe('view == salamatrixStudio.projectExplorer');
    expect(item.when).not.toContain('hasProjects');
    expect(packageJson.contributes.commands.some(
      (entry: { command: string }) => entry.command === 'salamatrixStudio.openUiCapabilitiesDemo',
    )).toBe(true);
  });

  it('only exposes Add Dialog when a project exists', () => {
    const item = packageJson.contributes.menus['view/title'].find((entry: { command: string }) => entry.command === 'salamatrixStudio.addDialog');
    expect(item.when).toContain('salamatrixStudio.hasProjects');
    const paletteItem = packageJson.contributes.menus.commandPalette.find((entry: { command: string }) => entry.command === 'salamatrixStudio.addDialog');
    expect(paletteItem.when).toContain('salamatrixStudio.hasProjects');
  });
});
