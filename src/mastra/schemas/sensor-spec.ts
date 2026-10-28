import { z } from 'zod';

export const integerLike = z.union([
  z.number().int().nonnegative(),
  z.string().regex(/^(0x[0-9a-fA-F]+|\d+)$/),
]);

export const busSchema = z.enum(['i2c', 'spi', 'mmio', 'can', 'gpio', 'ethernet']);

export const enumValueSchema = z.object({
  label: z.string().min(1),
  value: integerLike,
});

export const registerFieldSchema = z.object({
  name: z.string().min(1),
  lsb: z.number().int().min(0).max(63),
  width: z.number().int().min(1).max(64),
  access: z.enum(['ro', 'wo', 'rw', 'w1c', 'rc']).default('rw'),
  description: z.string().default(''),
  resetValue: integerLike.optional(),
  // An array of {label, value} pairs, not a dictionary: OpenAI's strict
  // Structured Outputs mode cannot represent dynamic-key objects
  // (z.record()) since every property must be statically enumerable.
  enumValues: z.array(enumValueSchema).default([]),
});

export const registerSchema = z.object({
  name: z.string().min(1),
  offset: integerLike,
  width: z.union([z.literal(8), z.literal(16), z.literal(32), z.literal(64)]),
  access: z.enum(['ro', 'wo', 'rw', 'w1c', 'rc']).default('rw'),
  resetValue: integerLike.default(0),
  description: z.string().default(''),
  fields: z.array(registerFieldSchema).default([]),
  readSideEffect: z.string().optional(),
  writeSideEffect: z.string().optional(),
});

export const interruptSchema = z.object({
  name: z.string(),
  source: z.string(),
  polarity: z.enum(['active-high', 'active-low']).default('active-high'),
  behavior: z.enum(['level', 'edge']).default('level'),
});

export const canFilterSchema = z.object({
  id: integerLike,
  mask: integerLike.default('0x7FF'),
  extended: z.boolean().default(false),
  destination: z.string().optional(),
});

export const canConfigSchema = z.object({
  protocol: z.enum(['can-2.0a', 'can-2.0b', 'can-fd']).default('can-2.0b'),
  nominalBitrate: z.number().int().positive().default(500000),
  dataBitrate: z.number().int().positive().optional(),
  samplePointPercent: z.number().min(50).max(99.9).optional(),
  loopbackSupported: z.boolean().default(false),
  listenOnlySupported: z.boolean().default(false),
  rxFifoDepth: z.number().int().positive().default(16),
  txMailboxCount: z.number().int().positive().default(3),
  filters: z.array(canFilterSchema).default([]),
}).superRefine((value, ctx) => {
  if (value.protocol === 'can-fd' && value.dataBitrate === undefined) {
    ctx.addIssue({ code: z.ZodIssueCode.custom, path: ['dataBitrate'], message: 'CAN FD requires dataBitrate.' });
  }
});

export const gpioPinSchema = z.object({
  pin: z.number().int().nonnegative(),
  name: z.string().min(1),
  direction: z.enum(['input', 'output', 'bidirectional']),
  resetLevel: z.enum(['low', 'high', 'high-z']).default('low'),
  activeLevel: z.enum(['active-high', 'active-low']).default('active-high'),
  pull: z.enum(['none', 'up', 'down']).default('none'),
  drive: z.enum(['push-pull', 'open-drain', 'open-source', 'high-z']).default('push-pull'),
  interrupt: z.enum(['none', 'rising', 'falling', 'both', 'level-high', 'level-low']).default('none'),
  description: z.string().default(''),
});

export const gpioConfigSchema = z.object({
  pins: z.array(gpioPinSchema).min(1),
  debounceMicroseconds: z.number().int().nonnegative().default(0),
});

export const ethernetConfigSchema = z.object({
  macAddress: z.string().regex(/^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/).optional(),
  phyInterface: z.enum(['mii', 'rmii', 'gmii', 'rgmii', 'sgmii', 'xgmii', 'virtual']).default('virtual'),
  supportedSpeedsMbps: z.array(z.union([z.literal(10), z.literal(100), z.literal(1000), z.literal(2500), z.literal(5000), z.literal(10000)])).min(1).default([100]),
  duplex: z.enum(['half', 'full', 'both']).default('full'),
  mtu: z.number().int().min(576).max(9216).default(1500),
  rxQueueCount: z.number().int().positive().default(1),
  txQueueCount: z.number().int().positive().default(1),
  checksumOffload: z.boolean().default(false),
  vlanSupported: z.boolean().default(false),
  promiscuousModeSupported: z.boolean().default(false),
  linkUpAtReset: z.boolean().default(true),
});

export const sensorSpecSchema = z.object({
  sensorName: z.string().min(1),
  modelClassName: z.string().regex(/^[A-Za-z_][A-Za-z0-9_]*$/).optional(),
  peripheralKind: z.enum(['sensor', 'actuator', 'mixed']).default('sensor'),
  bus: busSchema,
  address: integerLike.optional(),
  chipSelect: z.number().int().nonnegative().optional(),
  endianness: z.enum(['little', 'big']).default('little'),
  registerAddressWidth: z.union([z.literal(8), z.literal(16), z.literal(32)]).default(8),
  dataWidth: z.union([z.literal(8), z.literal(16), z.literal(32), z.literal(64)]).default(8),
  can: canConfigSchema.optional(),
  gpio: gpioConfigSchema.optional(),
  ethernet: ethernetConfigSchema.optional(),
  blockDiagramAnalysis: z.string().min(1),
  memoryMapAnalysis: z.string().min(1),
  registers: z.array(registerSchema).min(1),
  interrupts: z.array(interruptSchema).default([]),
  timingNotes: z.array(z.string()).default([]),
  behavioralNotes: z.array(z.string()).default([]),
  referenceModelHints: z.array(z.string()).default([]),
});

export type SensorSpec = z.infer<typeof sensorSpecSchema>;

export function parseInteger(value: string | number): number {
  if (typeof value === 'number') return value;
  return value.toLowerCase().startsWith('0x')
    ? Number.parseInt(value.slice(2), 16)
    : Number.parseInt(value, 10);
}
