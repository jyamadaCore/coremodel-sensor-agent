import { access } from 'node:fs/promises';

const required = [
  'src/mastra/index.ts',
  'src/mastra/agents/coremodel-sensor-agent.ts',
  'src/mastra/agents/sensor-spec-author-agent.ts',
  'src/mastra/schemas/sensor-spec.ts',
  'src/mastra/schemas/sensor-source.ts',
  'src/mastra/tools/validate-sensor-spec.ts',
  'src/mastra/tools/inspect-coremodel-library.ts',
  'src/mastra/tools/write-generated-model.ts',
  'src/mastra/tools/build-generated-model.ts',
  'src/mastra/tools/extract-datasheet-text.ts',
  'src/mastra/workflows/build-sensor-model.ts',
  'src/mastra/workflows/generate-sensor-spec.ts',
  'src/mastra/workflows/generate-and-build-sensor-model.ts',
  'examples/run.ts',
  'examples/generate-spec.ts',
  'examples/build.ts',
  'examples/ad74416h-source.json',
  'package.json',
  'tsconfig.json',
];

const missing = [];
for (const file of required) {
  try {
    await access(file);
  } catch {
    missing.push(file);
  }
}

if (missing.length) {
  console.error('Incomplete CoreModel sensor-agent project. Missing files:');
  for (const file of missing) console.error(`  - ${file}`);
  process.exit(1);
}

console.log(`Project layout is complete (${required.length} required files found).`);
