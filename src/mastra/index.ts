import { Mastra } from '@mastra/core/mastra';
import { coreModelSensorAgent } from './agents/coremodel-sensor-agent.ts';
import { sensorSpecAuthorAgent } from './agents/sensor-spec-author-agent.ts';
import { buildSensorModelWorkflow } from './workflows/build-sensor-model.ts';
import { generateSensorSpecWorkflow } from './workflows/generate-sensor-spec.ts';
import { generateAndBuildSensorModelWorkflow } from './workflows/generate-and-build-sensor-model.ts';

export const mastra = new Mastra({
  agents: { coreModelSensorAgent, sensorSpecAuthorAgent },
  workflows: { buildSensorModelWorkflow, generateSensorSpecWorkflow, generateAndBuildSensorModelWorkflow },
});
