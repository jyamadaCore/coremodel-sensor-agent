import { execFile } from 'node:child_process';
import { access, mkdtemp, readFile, rm } from 'node:fs/promises';
import { constants } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { promisify } from 'node:util';
import { createTool } from '@mastra/core/tools';
import { z } from 'zod';
import { pageRangeSchema } from '../schemas/sensor-source.ts';

const execFileAsync = promisify(execFile);
const MAX_OUTPUT_CHARS = 220_000;

function parseRange(range: string): { first: number; last: number } {
  const [firstText, lastText] = range.split('-');
  const first = Number.parseInt(firstText, 10);
  const last = lastText ? Number.parseInt(lastText, 10) : first;
  if (last < first) throw new Error(`Invalid page range: ${range}`);
  return { first, last };
}

export const extractDatasheetTextTool = createTool({
  id: 'extract-datasheet-text',
  description: 'Extract text from a local PDF datasheet with layout preserved, either the whole document or selected pages. Use this before deriving a CoreModel sensor specification from a PDF.',
  inputSchema: z.object({
    pdfPath: z.string().min(1),
    pageRanges: z.array(pageRangeSchema).default([]),
  }),
  outputSchema: z.object({
    pdfPath: z.string(),
    extractedRanges: z.array(z.string()),
    text: z.string(),
    truncated: z.boolean(),
    characterCount: z.number().int().nonnegative(),
  }),
  execute: async (input) => {
    const { pdfPath, pageRanges } = input;
    await access(pdfPath, constants.R_OK);

    const workDir = await mkdtemp(join(tmpdir(), 'coremodel-datasheet-'));
    const chunks: string[] = [];

    try {
      if (pageRanges.length === 0) {
        const outputPath = join(workDir, 'all-pages.txt');
        await execFileAsync('pdftotext', ['-layout', pdfPath, outputPath], { maxBuffer: 20 * 1024 * 1024 });
        const text = await readFile(outputPath, 'utf8');
        chunks.push(`\n===== PDF ALL PAGES =====\n${text}`);
      } else {
        for (const [index, range] of pageRanges.entries()) {
          const { first, last } = parseRange(range);
          const outputPath = join(workDir, `pages-${index}.txt`);
          await execFileAsync('pdftotext', [
            '-layout',
            '-f', String(first),
            '-l', String(last),
            pdfPath,
            outputPath,
          ], { maxBuffer: 20 * 1024 * 1024 });
          const text = await readFile(outputPath, 'utf8');
          chunks.push(`\n===== PDF PAGES ${range} =====\n${text}`);
        }
      }
    } finally {
      await rm(workDir, { recursive: true, force: true });
    }

    const combined = chunks.join('\n');
    const truncated = combined.length > MAX_OUTPUT_CHARS;
    const text = truncated ? combined.slice(0, MAX_OUTPUT_CHARS) : combined;

    return {
      pdfPath,
      extractedRanges: pageRanges.length === 0 ? ['all'] : pageRanges,
      text,
      truncated,
      characterCount: combined.length,
    };
  },
});
