import { createStep, createWorkflow } from '@mastra/core/workflows';
import { z } from 'zod';
import { sensorSourceSchema } from '../schemas/sensor-source.ts';
import { sensorSpecSchema } from '../schemas/sensor-spec.ts';

const workflowOutputSchema = z.object({
  spec: sensorSpecSchema,
  json: z.string(),
});

const generateStep = createStep({
  id: 'generate-coremodel-sensor-json',
  inputSchema: sensorSourceSchema,
  outputSchema: workflowOutputSchema,
  execute: async ({ inputData, mastra }) => {
    const agent = mastra?.getAgent('sensorSpecAuthorAgent');
    if (!agent) throw new Error('sensorSpecAuthorAgent is not registered.');

    const prompt = [
      'Generate the normalized CoreModel sensor JSON from this source request.',
      '',
      JSON.stringify(inputData, null, 2),
      '',
      inputData.sourceText ? `SOURCE TEXT:\n${inputData.sourceText}` : '',
    ].filter(Boolean).join('\n');

    const result = await agent.generate([
      { role: 'user', content: prompt },
    ], {
      maxSteps: 20,
      structuredOutput: { schema: sensorSpecSchema },
    });

    const spec = sensorSpecSchema.parse(result.object);
    return { spec, json: JSON.stringify(spec, null, 2) };
  },
});

export const generateSensorSpecWorkflow = createWorkflow({
  id: 'generate-sensor-spec',
  inputSchema: sensorSourceSchema,
  outputSchema: workflowOutputSchema,
})
  .then(generateStep)
  .commit();
