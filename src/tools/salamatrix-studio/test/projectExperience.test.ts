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
    expect(czechStrings['welcome.noProjects']).toContain('Vytvořit novou extension');
  });

  it('only exposes Add Dialog when a project exists', () => {
    const item = packageJson.contributes.menus['view/title'].find((entry: { command: string }) => entry.command === 'salamatrixStudio.addDialog');
    expect(item.when).toContain('salamatrixStudio.hasProjects');
    const paletteItem = packageJson.contributes.menus.commandPalette.find((entry: { command: string }) => entry.command === 'salamatrixStudio.addDialog');
    expect(paletteItem.when).toContain('salamatrixStudio.hasProjects');
  });
});
