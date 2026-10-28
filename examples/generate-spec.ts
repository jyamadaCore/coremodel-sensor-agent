import { readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { mastra } from '../src/mastra/index.ts';
import { sensorSourceSchema } from '../src/mastra/schemas/sensor-source.ts';

const sourceFile = process.argv[2];
if (!sourceFile) {
  console.error('Usage: npm run run:generate-spec -- <source-request.json> [output.json]');
  process.exit(2);
}

const outputFile = process.argv[3] ?? 'generated-sensor-spec.json';
const request = sensorSourceSchema.parse(JSON.parse(await readFile(resolve(sourceFile), 'utf8')));
const workflow = mastra.getWorkflow('generateSensorSpecWorkflow');
const run = await workflow.createRun();
const result = await run.start({ inputData: request });

if (result.status !== 'success') {
  console.error(result);
  process.exitCode = 1;
} else {
  await writeFile(resolve(outputFile), `${result.result.json}\n`, 'utf8');
  console.log(`Wrote ${resolve(outputFile)}`);
}
