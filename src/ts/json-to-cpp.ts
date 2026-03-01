import { readFile, writeFile } from 'node:fs/promises';
import * as ts from 'typescript';
import {
  quicktype,
  InputData,
  getOptionValues,
  JSONSchemaInput,
  FetchingJSONSchemaStore,
  type LanguageName,
  type Sourcelike,
  type Type,
} from 'quicktype-core';
import {
  CPlusPlusTargetLanguage,
  cPlusPlusOptions,
} from 'quicktype-core/dist/language/CPlusPlus/language';
import { CPlusPlusRenderer } from 'quicktype-core/dist/language/CPlusPlus/CPlusPlusRenderer';
import { minMaxValueForType } from 'quicktype-core/dist/attributes/Constraints';

type NumericEnumDefinition = {
  name: string;
  cases: NumericEnumCase[];
  cppUnderlyingType: string;
};

type NumericEnumCase = {
  value: number;
  cppName: string;
};

type NumericEnumFieldReference = {
  propertyName: string;
  enumName: string;
};

function integerCppTypeForBounds(t: Type): string | undefined {
  const minMax = minMaxValueForType(t);
  if (!minMax) {
    return undefined;
  }

  const [minimum, maximum] = minMax;
  if (
    typeof minimum !== 'number' ||
    typeof maximum !== 'number' ||
    !Number.isFinite(minimum) ||
    !Number.isFinite(maximum) ||
    !Number.isInteger(minimum) ||
    !Number.isInteger(maximum)
  ) {
    return undefined;
  }

  if (minimum >= 0) {
    if (maximum <= 0xff) return 'uint8_t';
    if (maximum <= 0xffff) return 'uint16_t';
    if (maximum <= 0xffffffff) return 'uint32_t';
    return 'uint64_t';
  }

  if (minimum >= -128 && maximum <= 127) return 'int8_t';
  if (minimum >= -32768 && maximum <= 32767) return 'int16_t';
  if (minimum >= -2147483648 && maximum <= 2147483647) return 'int32_t';
  return 'int64_t';
}

function replaceInt64Token(value: Sourcelike, replacement: string): Sourcelike {
  if (value === 'int64_t') {
    return replacement;
  }

  if (Array.isArray(value)) {
    return value.map((item) => replaceInt64Token(item, replacement));
  }

  return value;
}

function escapeRegExp(text: string): string {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function isPlainObject(value: unknown): value is Record<string, unknown> {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function isIntegerNumber(value: unknown): value is number {
  return (
    typeof value === 'number' &&
    Number.isFinite(value) &&
    Number.isInteger(value)
  );
}

function cppIntegerTypeForRange(minimum: number, maximum: number): string {
  if (minimum >= 0) {
    if (maximum <= 0xff) return 'uint8_t';
    if (maximum <= 0xffff) return 'uint16_t';
    if (maximum <= 0xffffffff) return 'uint32_t';
    return 'uint64_t';
  }

  if (minimum >= -128 && maximum <= 127) return 'int8_t';
  if (minimum >= -32768 && maximum <= 32767) return 'int16_t';
  if (minimum >= -2147483648 && maximum <= 2147483647) return 'int32_t';
  return 'int64_t';
}

function cppTypeForNumericEnum(values: number[]): string {
  const minimum = Math.min(...values);
  const maximum = Math.max(...values);
  return cppIntegerTypeForRange(minimum, maximum);
}

function enumCaseNameFromValue(value: number): string {
  if (value < 0) {
    return `NEG_${Math.abs(value)}`;
  }

  return `VALUE_${value}`;
}

function toUpperSnakeCase(value: string): string {
  const normalized = value
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .replace(/[^A-Za-z0-9]+/g, '_')
    .replace(/^_+|_+$/g, '')
    .replace(/_+/g, '_');

  if (normalized.length === 0) {
    return 'VALUE';
  }

  if (/^[0-9]/.test(normalized)) {
    return `VALUE_${normalized.toUpperCase()}`;
  }

  return normalized.toUpperCase();
}

function evaluateEnumMemberNumericValue(
  initializer: ts.Expression | undefined,
  previousValue: number | undefined,
): number | undefined {
  if (!initializer) {
    if (previousValue === undefined) {
      return 0;
    }

    return previousValue + 1;
  }

  if (ts.isNumericLiteral(initializer)) {
    return Number(initializer.text);
  }

  if (
    ts.isPrefixUnaryExpression(initializer) &&
    initializer.operator === ts.SyntaxKind.MinusToken &&
    ts.isNumericLiteral(initializer.operand)
  ) {
    return -Number(initializer.operand.text);
  }

  if (
    ts.isPrefixUnaryExpression(initializer) &&
    initializer.operator === ts.SyntaxKind.PlusToken &&
    ts.isNumericLiteral(initializer.operand)
  ) {
    return Number(initializer.operand.text);
  }

  return undefined;
}

function collectTsEnumValueNames(
  tsSource: string,
): Map<string, Map<number, string>> {
  const sourceFile = ts.createSourceFile(
    'types/displayprober-win-cpp.types.ts',
    tsSource,
    ts.ScriptTarget.Latest,
    true,
    ts.ScriptKind.TS,
  );

  const enumNameMap = new Map<string, Map<number, string>>();

  for (const statement of sourceFile.statements) {
    if (!ts.isEnumDeclaration(statement)) {
      continue;
    }

    const valueToName = new Map<number, string>();
    let previousValue: number | undefined = undefined;

    for (const member of statement.members) {
      const value = evaluateEnumMemberNumericValue(
        member.initializer,
        previousValue,
      );
      if (value === undefined || !Number.isInteger(value)) {
        previousValue = undefined;
        continue;
      }

      previousValue = value;

      const memberName = ts.isIdentifier(member.name)
        ? member.name.text
        : ts.isStringLiteral(member.name)
          ? member.name.text
          : undefined;

      if (!memberName) {
        continue;
      }

      if (!valueToName.has(value)) {
        valueToName.set(value, toUpperSnakeCase(memberName));
      }
    }

    if (valueToName.size > 0) {
      enumNameMap.set(statement.name.text, valueToName);
    }
  }

  return enumNameMap;
}

function findReferencedDefinitionName(node: unknown): string | undefined {
  if (!isPlainObject(node)) {
    return undefined;
  }

  const ref = node.$ref;
  if (typeof ref === 'string' && ref.startsWith('#/definitions/')) {
    return ref.slice('#/definitions/'.length);
  }

  const anyOf = node.anyOf;
  if (Array.isArray(anyOf)) {
    for (let i = 0; i < anyOf.length; i++) {
      const referenced = findReferencedDefinitionName(anyOf[i]);
      if (referenced) {
        return referenced;
      }
    }
  }

  const oneOf = node.oneOf;
  if (Array.isArray(oneOf)) {
    for (let i = 0; i < oneOf.length; i++) {
      const referenced = findReferencedDefinitionName(oneOf[i]);
      if (referenced) {
        return referenced;
      }
    }
  }

  return undefined;
}

function collectNumericEnumDefinitions(
  schema: Record<string, unknown>,
  tsEnumCaseNames: Map<string, Map<number, string>>,
): NumericEnumDefinition[] {
  const definitions = schema.definitions;
  if (!isPlainObject(definitions)) {
    return [];
  }

  const numericEnums: NumericEnumDefinition[] = [];

  for (const [definitionName, definitionSchema] of Object.entries(
    definitions,
  )) {
    if (!isPlainObject(definitionSchema)) {
      continue;
    }

    if (definitionSchema.type !== 'integer') {
      continue;
    }

    const enumValues = definitionSchema.enum;
    if (!Array.isArray(enumValues) || enumValues.length === 0) {
      continue;
    }

    const values: number[] = [];
    let allIntegerNumbers = true;
    for (let i = 0; i < enumValues.length; i++) {
      const value = enumValues[i];
      if (!isIntegerNumber(value)) {
        allIntegerNumbers = false;
        break;
      }

      values.push(value);
    }

    if (!allIntegerNumbers) {
      continue;
    }

    const tsCaseNames = tsEnumCaseNames.get(definitionName);
    const cases: NumericEnumCase[] = values.map((value) => ({
      value,
      cppName: tsCaseNames?.get(value) ?? enumCaseNameFromValue(value),
    }));

    numericEnums.push({
      name: definitionName,
      cases,
      cppUnderlyingType: cppTypeForNumericEnum(values),
    });
  }

  return numericEnums;
}

function collectNumericEnumFieldReferences(
  schema: Record<string, unknown>,
  numericEnumsByName: Map<string, NumericEnumDefinition>,
): NumericEnumFieldReference[] {
  const definitions = schema.definitions;
  if (!isPlainObject(definitions)) {
    return [];
  }

  const references = new Map<string, NumericEnumFieldReference>();

  for (const definitionSchema of Object.values(definitions)) {
    if (!isPlainObject(definitionSchema)) {
      continue;
    }

    const properties = definitionSchema.properties;
    if (!isPlainObject(properties)) {
      continue;
    }

    for (const [propertyName, propertySchema] of Object.entries(properties)) {
      const referencedDefinitionName =
        findReferencedDefinitionName(propertySchema);
      if (!referencedDefinitionName) {
        continue;
      }

      if (!numericEnumsByName.has(referencedDefinitionName)) {
        continue;
      }

      references.set(`${propertyName}|${referencedDefinitionName}`, {
        propertyName,
        enumName: referencedDefinitionName,
      });
    }
  }

  return Array.from(references.values());
}

function buildNumericEnumDeclaration(
  definition: NumericEnumDefinition,
): string {
  const entries = definition.cases
    .map((item) => `        ${item.cppName} = ${item.value}`)
    .join(',\n');

  return [
    `    enum class ${definition.name} : ${definition.cppUnderlyingType} {`,
    entries,
    '    };',
  ].join('\n');
}

function buildNumericEnumForwardDeclarations(
  definitions: NumericEnumDefinition[],
): string {
  return definitions
    .map((definition) =>
      [
        `    void from_json(const json & j, ${definition.name} & x);`,
        `    void to_json(json & j, const ${definition.name} & x);`,
      ].join('\n'),
    )
    .join('\n\n');
}

function buildNumericEnumFunctionDefinitions(
  definitions: NumericEnumDefinition[],
): string {
  return definitions
    .map((definition) => {
      const fromJsonCases = definition.cases
        .map(
          (item) =>
            `            case ${item.value}: x = ${definition.name}::${item.cppName}; break;`,
        )
        .join('\n');

      const toJsonCases = definition.cases
        .map(
          (item) =>
            `            case ${definition.name}::${item.cppName}: j = ${item.value}; break;`,
        )
        .join('\n');

      return [
        `    inline void from_json(const json & j, ${definition.name} & x) {`,
        '        const auto value = j.get<int64_t>();',
        '        switch (value) {',
        fromJsonCases,
        '            default: throw std::runtime_error("Input JSON does not conform to schema!");',
        '        }',
        '    }',
        '',
        `    inline void to_json(json & j, const ${definition.name} & x) {`,
        '        switch (x) {',
        toJsonCases,
        `            default: throw std::runtime_error("Unexpected value in enumeration \\\"${definition.name}\\\": " + std::to_string(static_cast<int64_t>(x)));`,
        '        }',
        '    }',
      ].join('\n');
    })
    .join('\n\n');
}

function patchCppWithNumericEnums(
  cpp: string,
  numericEnumDefinitions: NumericEnumDefinition[],
  fieldReferences: NumericEnumFieldReference[],
): string {
  if (numericEnumDefinitions.length === 0) {
    return cpp;
  }

  let patched = cpp;

  const enumDeclarationInsertionPoint =
    '\n    enum class WinActiveColorMode : int {';
  const enumDeclarationIndex = patched.indexOf(enumDeclarationInsertionPoint);
  if (enumDeclarationIndex !== -1) {
    const declarationBlock = numericEnumDefinitions
      .map((definition) => buildNumericEnumDeclaration(definition))
      .join('\n\n');

    patched =
      patched.slice(0, enumDeclarationIndex) +
      '\n' +
      declarationBlock +
      '\n\n' +
      patched.slice(enumDeclarationIndex + 1);
  }

  for (const fieldReference of fieldReferences) {
    const { propertyName, enumName } = fieldReference;
    const escapedPropertyName = escapeRegExp(propertyName);

    patched = patched.replace(
      new RegExp(`std::optional<[^>]+>\\s+${escapedPropertyName};`, 'g'),
      `std::optional<${enumName}> ${propertyName};`,
    );

    patched = patched.replace(
      new RegExp(
        `get_stack_optional<[^>]+>\\(j, \"${escapedPropertyName}\"\\)`,
        'g',
      ),
      `get_stack_optional<${enumName}>(j, "${propertyName}")`,
    );

    patched = patched.replace(
      new RegExp(`get_optional<[^>]+>\\(j, \"${escapedPropertyName}\"\\)`, 'g'),
      `get_optional<${enumName}>(j, "${propertyName}")`,
    );

    patched = patched.replace(
      new RegExp(
        `j\\.at\\(\"${escapedPropertyName}\"\\)\\.get<[^>]+>\\(\\)`,
        'g',
      ),
      `j.at("${propertyName}").get<${enumName}>()`,
    );
  }

  const forwardDeclarationAnchor =
    '\n    inline void from_json(const json & j, WinAdvancedColorInfo& x) {';
  const forwardDeclarationIndex = patched.indexOf(forwardDeclarationAnchor);
  if (forwardDeclarationIndex !== -1) {
    const forwardDeclarations = buildNumericEnumForwardDeclarations(
      numericEnumDefinitions,
    );

    patched =
      patched.slice(0, forwardDeclarationIndex) +
      '\n' +
      forwardDeclarations +
      '\n' +
      patched.slice(forwardDeclarationIndex);
  }

  const functionDefinitionAnchor =
    '\n    inline void from_json(const json & j, WinActiveColorMode & x) {';
  const functionDefinitionIndex = patched.indexOf(functionDefinitionAnchor);
  if (functionDefinitionIndex !== -1) {
    const functionDefinitions = buildNumericEnumFunctionDefinitions(
      numericEnumDefinitions,
    );

    patched =
      patched.slice(0, functionDefinitionIndex) +
      '\n' +
      functionDefinitions +
      '\n' +
      patched.slice(functionDefinitionIndex + 1);
  }

  return patched;
}

class FixedWidthIntegerCPlusPlusRenderer extends CPlusPlusRenderer {
  protected override cppType(
    t: Type,
    ctx: {
      inJsonNamespace: boolean;
      needsForwardIndirection: boolean;
      needsOptionalIndirection: boolean;
    },
    withIssues: boolean,
    forceNarrowString: boolean,
    isOptional: boolean,
  ): Sourcelike {
    const rendered = super.cppType(
      t,
      ctx,
      withIssues,
      forceNarrowString,
      isOptional,
    );

    if (t.kind !== 'integer') {
      return rendered;
    }

    const cppIntegerType = integerCppTypeForBounds(t);
    if (!cppIntegerType || cppIntegerType === 'int64_t') {
      return rendered;
    }

    return replaceInt64Token(rendered, cppIntegerType);
  }
}

class FixedWidthIntegerCPlusPlusTargetLanguage extends CPlusPlusTargetLanguage {
  protected override makeRenderer(
    renderContext: unknown,
    untypedOptionValues: unknown,
  ): CPlusPlusRenderer {
    return new FixedWidthIntegerCPlusPlusRenderer(
      this,
      renderContext as any,
      getOptionValues(cPlusPlusOptions, untypedOptionValues as any),
    );
  }
}

async function quicktypeJSONSchema(
  targetLanguage: LanguageName | CPlusPlusTargetLanguage,
  typeName: string,
  jsonSchemaString: string,
) {
  const schemaInput = new JSONSchemaInput(new FetchingJSONSchemaStore());

  // We could add multiple schemas for multiple types,
  // but here we're just making one type from JSON schema.
  await schemaInput.addSource({ name: typeName, schema: jsonSchemaString });

  const inputData = new InputData();
  inputData.addInput(schemaInput);

  return await quicktype({
    inputData,
    lang: targetLanguage,
    rendererOptions: {
      namespace: 'json',

      // Don't use Boost for `optional<T>`.
      // Instead, use C++17 `std::optional<T>`.
      boost: false,

      // Generate plain `struct` types and free-floating functions instead of
      // C++ classes.
      'code-format': 'with-struct',

      // Emit `import <nlohmann/json.hpp>`.
      'include-location': 'global-include',

      // Don't emit fields with `null` values.
      'hide-null-optional': true,
    },
  });
}

async function main() {
  const typesSource = await readFile(
    'types/displayprober-win-cpp.types.ts',
    'utf8',
  );
  const tsEnumCaseNames = collectTsEnumValueNames(typesSource);

  const schemaString = await readFile(
    'schemas/displayprober-win-cpp.schema.json',
    'utf8',
  );
  const schema = JSON.parse(schemaString) as Record<string, unknown>;
  const numericEnumDefinitions = collectNumericEnumDefinitions(
    schema,
    tsEnumCaseNames,
  );
  const numericEnumsByName = new Map(
    numericEnumDefinitions.map((definition) => [definition.name, definition]),
  );
  const numericEnumFieldReferences = collectNumericEnumFieldReferences(
    schema,
    numericEnumsByName,
  );

  const { lines } = await quicktypeJSONSchema(
    new FixedWidthIntegerCPlusPlusTargetLanguage(),
    'WinDisplayProberJson',
    schemaString,
  );
  const cpp = lines.join('\n');
  const cppWithNumericEnums = patchCppWithNumericEnums(
    cpp,
    numericEnumDefinitions,
    numericEnumFieldReferences,
  );
  await writeFile('../cpp/gencode/acd-json.hpp', cppWithNumericEnums, 'utf8');
}

main();
