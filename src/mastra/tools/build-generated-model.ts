import { createTool } from '@mastra/core/tools';
import { z } from 'zod';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import path from 'node:path';

const execFileAsync = promisify(execFile);

export const buildGeneratedModelTool = createTool({
  id: 'build-generated-model',
  description: 'Run the configured CoreModel build/test command in a generated model directory.',
  inputSchema: z.object({ outputDirectory: z.string(), timeoutMs: z.number().int().min(1000).max(600000).default(120000) }),
  outputSchema: z.object({ skipped: z.boolean(), success: z.boolean(), stdout: z.string(), stderr: z.string() }),
  execute: async ({ outputDirectory, timeoutMs }) => {
    const command = process.env.COREMODEL_BUILD_COMMAND?.trim();
    if (!command) return { skipped: true, success: false, stdout: '', stderr: 'COREMODEL_BUILD_COMMAND is not configured.' };

    const outputRoot = path.resolve(process.env.COREMODEL_OUTPUT_ROOT ?? 'sensor-models');
    const cwd = path.resolve(outputDirectory);
    if (cwd !== outputRoot && !cwd.startsWith(`${outputRoot}${path.sep}`)) throw new Error('Build directory is outside COREMODEL_OUTPUT_ROOT.');

    try {
      const { stdout, stderr } = await execFileAsync('/bin/sh', ['-lc', command], {
        cwd,
        timeout: timeoutMs,
        maxBuffer: 4 * 1024 * 1024,
      });
      return { skipped: false, success: true, stdout, stderr };
    } catch (error) {
      const e = error as Error & { stdout?: string; stderr?: string };
      return { skipped: false, success: false, stdout: e.stdout ?? '', stderr: e.stderr ?? e.message };
    }
  },
});
