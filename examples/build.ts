import { resolve } from 'node:path';
import { mastra } from '../src/mastra/index.ts';

const datasheetPath = process.argv[2];
const sensorName = process.argv[3];

if (!datasheetPath || !sensorName) {
  console.error('Usage: npm run build -- <path-to-datasheet.pdf> "<sensor or actuator name>"');
  process.exit(2);
}

const workflow = mastra.getWorkflow('generateAndBuildSensorModelWorkflow');
const run = await workflow.createRun();
const result = await run.start({
  inputData: { sensorName, datasheetPath: resolve(datasheetPath) },
});

if (result.status !== 'success') {
  console.error(result);
  process.exitCode = 1;
} else {
  console.log(`Generated spec:\n${result.result.specJson}\n`);
  console.log(result.result.report);
}
