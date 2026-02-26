import * as tsj from 'ts-json-schema-generator';
import * as fs from 'fs';

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

function normalizeUint32Types(node: unknown): void {
  if (node === null || typeof node !== 'object') {
    return;
  }

  if (Array.isArray(node)) {
    for (let i = 0; i < node.length; i++) {
      normalizeUint32Types(node[i]);
    }
    return;
  }

  const obj = node as Record<string, unknown>;
  const fullDescription = obj.fullDescription;
  if (
    typeof fullDescription === 'string' &&
    fullDescription.includes('@uint32')
  ) {
    const type = obj.type;
    if (type === 'number') {
      obj.type = 'integer';
    } else if (Array.isArray(type)) {
      for (let i = 0; i < type.length; i++) {
        if (type[i] === 'number') {
          type[i] = 'integer';
        }
      }
    }
  }

  const enumValues = obj.enum;
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
      const type = obj.type;
      if (type === 'number') {
        obj.type = 'integer';
      } else if (Array.isArray(type)) {
        for (let i = 0; i < type.length; i++) {
          if (type[i] === 'number') {
            type[i] = 'integer';
          }
        }
      }
    }
  }

  for (const key in obj) {
    normalizeUint32Types(obj[key]);
  }
}

normalizeUint32Types(schema);

const schemaString = JSON.stringify(schema, null, 2);
fs.writeFile(outputPath, schemaString, (err) => {
  if (err) throw err;
});
