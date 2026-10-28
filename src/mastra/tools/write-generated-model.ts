import { createTool } from '@mastra/core/tools';
import { z } from 'zod';
import { mkdir, writeFile } from 'node:fs/promises';
import path from 'node:path';

const fileSchema = z.object({
  relativePath: z.string().min(1).refine((p) => !path.isAbsolute(p) && !p.split(/[\\/]/).includes('..'), 'Path traversal is forbidden'),
  content: z.string(),
});

export const writeGeneratedModelTool = createTool({
  id: 'write-generated-model',
  description: 'Write generated CoreModel source, build files, tests, and manifest into an isolated sensor directory.',
  inputSchema: z.object({
    sensorSlug: z.string().regex(/^[a-z0-9][a-z0-9-]*$/),
    files: z.array(fileSchema).min(1).max(30),
  }),
  outputSchema: z.object({ outputDirectory: z.string(), writtenFiles: z.array(z.string()) }),
  execute: async ({ sensorSlug, files }) => {
    const outputRoot = process.env.COREMODEL_OUTPUT_ROOT ?? path.resolve('sensor-models');
    const outputDirectory = path.resolve(outputRoot, sensorSlug);
    await mkdir(outputDirectory, { recursive: true });

    const writtenFiles: string[] = [];
    for (const file of files) {
      const target = path.resolve(outputDirectory, file.relativePath);
      if (!target.startsWith(`${outputDirectory}${path.sep}`)) throw new Error(`Unsafe output path: ${file.relativePath}`);
      await mkdir(path.dirname(target), { recursive: true });
      await writeFile(target, file.content, 'utf8');
      writtenFiles.push(file.relativePath);
    }
    return { outputDirectory, writtenFiles };
  },
});
