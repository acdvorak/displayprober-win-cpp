import * as tsj from 'ts-json-schema-generator';
import * as fs from 'fs';

import type { Def } from './json-schema-types';

const config: tsj.Config = {
  schemaId: 'displayprober-win-cpp',
  path: 'types/displayprober-win-cpp.types.ts',
  tsconfig: 'tsconfig.json',
  type: 'WinDisplayProberJson',
  jsDoc: 'extended',
  markdownDescription: true,
  fullDescription: true,
  strictTuples: true,
};

const outputPath = 'schemas/displayprober-win-cpp.schema.json';

const schema = tsj.createGenerator(config).createSchema(config.type);

function setIntegerBoundaries(node: Def): void {
  const fullDescription = node.fullDescription;
  if (!fullDescription) {
    return;
  }

  const type = node.type;

  let bounds:
    | { minimum: number | bigint; maximum: number | bigint }
    | undefined = undefined;
  if (fullDescription.includes('@uint64')) {
    bounds = { minimum: 0, maximum: Number.MAX_SAFE_INTEGER };
  } else if (fullDescription.includes('@uint32')) {
    bounds = { minimum: 0, maximum: 0xffffffff };
  } else if (fullDescription.includes('@uint16')) {
    bounds = { minimum: 0, maximum: 0xffff };
  } else if (fullDescription.includes('@uint8')) {
    bounds = { minimum: 0, maximum: 0xff };
  } else if (fullDescription.includes('@int64')) {
    bounds = { minimum: -4_294_967_295, maximum: 4_294_967_294 };
  } else if (fullDescription.includes('@int32')) {
    bounds = { minimum: -2_147_483_648, maximum: 2_147_483_647 };
  } else if (fullDescription.includes('@int16')) {
    bounds = { minimum: -32_768, maximum: 32_767 };
  } else if (fullDescription.includes('@int8')) {
    bounds = { minimum: -128, maximum: 127 };
  }

  if (type === 'number') {
    node.type = 'integer';
  } else if (Array.isArray(type) && type.some((t) => t === 'number')) {
    for (let i = 0; i < type.length; i++) {
      if (type[i] === 'number') {
        type[i] = 'integer';
      }
    }
  }

  if (bounds) {
    Object.assign(node, bounds);
  }
}

function normalizeIntegerTypes(node: Def | null | undefined): void {
  if (!node || typeof node !== 'object') {
    return;
  }

  if (Array.isArray(node)) {
    for (let i = 0; i < node.length; i++) {
      normalizeIntegerTypes(node[i] as Def);
    }
    return;
  }

  const fullDescription = node.fullDescription;

  if (typeof fullDescription === 'string') {
    if (fullDescription.includes('@int') || fullDescription.includes('@uint')) {
      setIntegerBoundaries(node);
    }
  }

  const enumValues = node.enum;
  if (Array.isArray(enumValues)) {
    let allIntegers = true;
    for (let i = 0; i < enumValues.length; i++) {
      const value = enumValues[i];
      if (typeof value !== 'number' || !Number.isInteger(value)) {
        allIntegers = false;
        break;
      }
    }

    if (allIntegers) {
      const type = node.type;
      if (type === 'number') {
        node.type = 'integer';
      } else if (Array.isArray(type)) {
        for (let i = 0; i < type.length; i++) {
          if (type[i] === 'number') {
            type[i] = 'integer';
          }
        }
      }
    }
  }

  for (const key in node) {
    normalizeIntegerTypes((node as Record<string, Def>)[key]);
  }
}

normalizeIntegerTypes(schema as Def);

const schemaString = JSON.stringify(schema, null, 2);
fs.writeFile(outputPath, schemaString, (err) => {
  if (err) throw err;
});
