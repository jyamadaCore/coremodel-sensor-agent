import { z } from 'zod';
import { busSchema, integerLike } from './sensor-spec.ts';

export const pageRangeSchema = z.string().regex(/^\d+(?:-\d+)?$/, 'Use a page number or range such as 1 or 75-99.');

export const sensorSourceSchema = z.object({
  sensorName: z.string().min(1).optional(),
  modelClassName: z.string().regex(/^[A-Za-z_][A-Za-z0-9_]*$/).optional(),
  busHint: busSchema.optional(),
  address: integerLike.optional(),
  chipSelect: z.number().int().nonnegative().optional(),
  datasheetPath: z.string().min(1).optional(),
  sourceText: z.string().min(1).optional(),
  pageRanges: z.array(pageRangeSchema).default([]),
  includeFullRegisterMap: z.boolean().default(true),
  additionalInstructions: z.string().default(''),
}).superRefine((value, ctx) => {
  if (!value.datasheetPath && !value.sourceText) {
    ctx.addIssue({
      code: z.ZodIssueCode.custom,
      path: ['datasheetPath'],
      message: 'Provide datasheetPath or sourceText.',
    });
  }
});

export type SensorSource = z.infer<typeof sensorSourceSchema>;
