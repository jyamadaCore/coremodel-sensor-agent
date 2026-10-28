import { createStep, createWorkflow } from '@mastra/core/workflows';
import { z } from 'zod';
import { sensorSpecSchema } from '../schemas/sensor-spec.ts';

const outputSchema = z.object({
  report: z.string(),
});

const buildStep = createStep({
  id: 'build-coremodel-sensor',
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

    return { report: result.text };
  },
});

export const buildSensorModelWorkflow = createWorkflow({
  id: 'build-sensor-model',
  inputSchema: sensorSpecSchema,
  outputSchema,
})
  .then(buildStep)
  .commit();
