import { createStep, createWorkflow } from '@mastra/core/workflows';
import { z } from 'zod';
import { sensorSourceSchema } from '../schemas/sensor-source.ts';
import { sensorSpecSchema } from '../schemas/sensor-spec.ts';

export const minimalSensorRequestSchema = z.object({
  sensorName: z.string().min(1),
  datasheetPath: z.string().min(1),
});

const outputSchema = z.object({
  spec: sensorSpecSchema,
  specJson: z.string(),
  report: z.string(),
});

const generateStep = createStep({
  id: 'generate-coremodel-sensor-json-from-minimal-request',
  inputSchema: minimalSensorRequestSchema,
  outputSchema: sensorSpecSchema,
  execute: async ({ inputData, mastra }) => {
    const agent = mastra?.getAgent('sensorSpecAuthorAgent');
    if (!agent) throw new Error('sensorSpecAuthorAgent is not registered.');

    const sourceRequest = sensorSourceSchema.parse({
      sensorName: inputData.sensorName,
      datasheetPath: inputData.datasheetPath,
    });

    const prompt = [
      'Generate the normalized CoreModel sensor JSON from this source request.',
      'No busHint, pageRanges, address, or chipSelect were supplied: infer the bus and every other detail yourself from the document, as instructed.',
      '',
      JSON.stringify(sourceRequest, null, 2),
    ].join('\n');

    const result = await agent.generate([{ role: 'user', content: prompt }], {
      maxSteps: 20,
      structuredOutput: { schema: sensorSpecSchema },
    });

    return sensorSpecSchema.parse(result.object);
  },
});

const buildStep = createStep({
  id: 'build-coremodel-sensor-from-generated-spec',
  inputSchema: sensorSpecSchema,
  outputSchema,
  execute: async ({ inputData, mastra }) => {
    const agent = mastra?.getAgent('coreModelSensorAgent');
    if (!agent) throw new Error('coreModelSensorAgent is not registered.');

    const result = await agent.generate([
      {
        role: 'user',
        content: `Build this CoreModel sensor model.\n\n${JSON.stringify(inputData, null, 2)}`,
      },
    ], { maxSteps: 30 });

    return {
      spec: inputData,
      specJson: JSON.stringify(inputData, null, 2),
      report: result.text,
    };
  },
});

export const generateAndBuildSensorModelWorkflow = createWorkflow({
  id: 'generate-and-build-sensor-model',
  inputSchema: minimalSensorRequestSchema,
  outputSchema,
})
  .then(generateStep)
  .then(buildStep)
  .commit();
