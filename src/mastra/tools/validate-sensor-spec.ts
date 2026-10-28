import { createTool } from '@mastra/core/tools';
import { z } from 'zod';
import { parseInteger, sensorSpecSchema } from '../schemas/sensor-spec.ts';

const resultSchema = z.object({
  valid: z.boolean(),
  errors: z.array(z.string()),
  warnings: z.array(z.string()),
  normalizedSpec: sensorSpecSchema.optional(),
});

export const validateSensorSpecTool = createTool({
  id: 'validate-sensor-spec',
  description: 'Validate a normalized I2C, SPI, MMIO, CAN, GPIO, or Ethernet peripheral specification before generating CoreModel code.',
  inputSchema: sensorSpecSchema,
  outputSchema: resultSchema,
  execute: async (input) => {
    const errors: string[] = [];
    const warnings: string[] = [];
    const offsets = new Map<number, string>();

    if (input.bus === 'i2c' && input.address === undefined) errors.push('I2C models require an address.');
    if (input.bus === 'spi' && input.chipSelect === undefined) warnings.push('SPI chipSelect is omitted; platform integration must provide it.');
    if (input.bus === 'can' && !input.can) errors.push('CAN models require a can configuration object.');
    if (input.bus === 'gpio' && !input.gpio) errors.push('GPIO models require a gpio configuration object.');
    if (input.bus === 'ethernet' && !input.ethernet) errors.push('Ethernet models require an ethernet configuration object.');

    if (input.can) {
      const maxStandardId = 0x7ff;
      const maxExtendedId = 0x1fffffff;
      input.can.filters.forEach((filter, index) => {
        const id = parseInteger(filter.id);
        const mask = parseInteger(filter.mask);
        const max = filter.extended ? maxExtendedId : maxStandardId;
        if (id > max) errors.push(`CAN filter ${index}: ID 0x${id.toString(16)} exceeds ${filter.extended ? '29-bit' : '11-bit'} range.`);
        if (mask > max) errors.push(`CAN filter ${index}: mask 0x${mask.toString(16)} exceeds ${filter.extended ? '29-bit' : '11-bit'} range.`);
      });
      if (input.can.protocol !== 'can-fd' && input.can.dataBitrate !== undefined) {
        warnings.push('CAN dataBitrate is ignored unless protocol is can-fd.');
      }
    }

    if (input.gpio) {
      const pins = new Map<number, string>();
      for (const pin of input.gpio.pins) {
        const previous = pins.get(pin.pin);
        if (previous) errors.push(`GPIO pin collision at ${pin.pin}: ${previous} and ${pin.name}.`);
        pins.set(pin.pin, pin.name);
        if (pin.direction === 'input' && ['push-pull', 'open-drain', 'open-source'].includes(pin.drive)) {
          warnings.push(`GPIO ${pin.name}: drive mode is normally irrelevant for an input pin.`);
        }
        if (pin.direction === 'output' && pin.interrupt !== 'none') {
          warnings.push(`GPIO ${pin.name}: interrupt behavior on an output pin requires explicit loopback or external-line semantics.`);
        }
      }
    }

    if (input.ethernet) {
      const uniqueSpeeds = new Set(input.ethernet.supportedSpeedsMbps);
      if (uniqueSpeeds.size !== input.ethernet.supportedSpeedsMbps.length) errors.push('Ethernet supportedSpeedsMbps contains duplicate values.');
      if (input.ethernet.phyInterface === 'rmii' && [...uniqueSpeeds].some((speed) => speed > 100)) {
        errors.push('RMII supports 10/100 Mbps only; remove gigabit and faster speeds or select another PHY interface.');
      }
      if (input.ethernet.mtu > 1500) warnings.push('Ethernet jumbo-frame MTU requested; generated tests must cover oversized frame handling.');
    }

    for (const register of input.registers) {
      const offset = parseInteger(register.offset);
      const previous = offsets.get(offset);
      if (previous) errors.push(`Register offset collision at 0x${offset.toString(16)}: ${previous} and ${register.name}.`);
      offsets.set(offset, register.name);

      const registerBits = register.width;
      const occupied = new Set<number>();
      const reset = parseInteger(register.resetValue);
      const maxReset = registerBits === 64 ? Number.MAX_SAFE_INTEGER : 2 ** registerBits - 1;
      if (reset > maxReset) errors.push(`${register.name}: reset value exceeds ${registerBits} bits.`);

      for (const field of register.fields) {
        if (field.lsb + field.width > registerBits) errors.push(`${register.name}.${field.name}: field exceeds register width.`);
        for (let bit = field.lsb; bit < field.lsb + field.width; bit++) {
          if (occupied.has(bit)) errors.push(`${register.name}.${field.name}: overlaps another field at bit ${bit}.`);
          occupied.add(bit);
        }
      }
    }

    if (!input.registers.some((r) => /who.?am.?i|dev(?:ice)?_?id|chip_?id|devid|version|revision/i.test(r.name))) {
      warnings.push('No obvious identity/version register was found; confirm the peripheral truly lacks one.');
    }

    return { valid: errors.length === 0, errors, warnings, normalizedSpec: errors.length === 0 ? input : undefined };
  },
});
