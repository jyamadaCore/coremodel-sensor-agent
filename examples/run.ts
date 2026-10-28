import { readFile } from 'node:fs/promises';
import { mastra } from '../src/mastra/index.ts';
import { sensorSpecSchema } from '../src/mastra/schemas/sensor-spec.ts';

const file = process.argv[2] ?? new URL('./adxl345.json', import.meta.url).pathname;
const spec = sensorSpecSchema.parse(JSON.parse(await readFile(file, 'utf8')));
const workflow = mastra.getWorkflow('buildSensorModelWorkflow');
const run = await workflow.createRun();
const result = await run.start({ inputData: spec });

if (result.status !== 'success') {
  console.error(result);
  process.exitCode = 1;
} else {
  console.log(result.result.report);
}
