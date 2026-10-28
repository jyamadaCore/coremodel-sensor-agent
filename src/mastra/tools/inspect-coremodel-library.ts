import { createTool } from '@mastra/core/tools';
import { z } from 'zod';
import { readdir, readFile, stat } from 'node:fs/promises';
import path from 'node:path';

const allowedExtensions = new Set(['.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.md', '.txt', '.cmake']);

async function walk(root: string, maxFiles: number): Promise<string[]> {
  const found: string[] = [];
  async function visit(dir: string): Promise<void> {
    if (found.length >= maxFiles) return;
    for (const entry of await readdir(dir, { withFileTypes: true })) {
      if (found.length >= maxFiles) return;
      if (entry.name.startsWith('.') || ['build', 'node_modules', 'out'].includes(entry.name)) continue;
      const full = path.join(dir, entry.name);
      if (entry.isDirectory()) await visit(full);
      else if (allowedExtensions.has(path.extname(entry.name).toLowerCase())) found.push(full);
    }
  }
  await visit(root);
  return found;
}

export const inspectCoreModelLibraryTool = createTool({
  id: 'inspect-coremodel-library',
  description: 'Find relevant CoreModel peripheral implementations and return compact excerpts for API-grounded code generation.',
  inputSchema: z.object({
    bus: z.enum(['i2c', 'spi', 'mmio', 'can', 'gpio', 'ethernet']),
    sensorName: z.string(),
    hints: z.array(z.string()).default([]),
    maxFiles: z.number().int().min(1).max(20).default(8),
  }),
  outputSchema: z.object({
    root: z.string(),
    matches: z.array(z.object({ path: z.string(), excerpt: z.string() })),
  }),
  execute: async ({ bus, sensorName, hints, maxFiles }) => {
    const root = process.env.COREMODEL_LIBRARY_ROOT;
    if (!root) throw new Error('COREMODEL_LIBRARY_ROOT is not configured.');
    if (root.includes('/absolute/path/to/')) {
      throw new Error(`COREMODEL_LIBRARY_ROOT still contains the placeholder path: ${root}. Set it to the actual local CoreModel source directory and restart Mastra.`);
    }
    const rootStat = await stat(root);
    if (!rootStat.isDirectory()) throw new Error('COREMODEL_LIBRARY_ROOT is not a directory.');

    const terms = [bus, sensorName, ...hints, 'register', 'peripheral'].map((v) => v.toLowerCase());
    const files = await walk(root, 5000);
    const scored: Array<{ file: string; score: number; content: string }> = [];

    for (const file of files) {
      const content = await readFile(file, 'utf8').catch(() => '');
      const haystack = `${file}\n${content}`.toLowerCase();
      const score = terms.reduce((sum, term) => sum + (haystack.includes(term) ? 1 : 0), 0);
      if (score > 0) scored.push({ file, score, content });
    }

    scored.sort((a, b) => b.score - a.score || a.file.localeCompare(b.file));
    return {
      root,
      matches: scored.slice(0, maxFiles).map(({ file, content }) => ({
        path: path.relative(root, file),
        excerpt: content.slice(0, 12000),
      })),
    };
  },
});
