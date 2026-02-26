import { readFile, writeFile } from 'node:fs/promises';
import {
  quicktype,
  InputData,
  jsonInputForTargetLanguage,
  JSONSchemaInput,
  FetchingJSONSchemaStore,
  type LanguageName,
} from 'quicktype-core';

async function quicktypeJSONSchema(
  targetLanguage: LanguageName,
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
  const { lines } = await quicktypeJSONSchema(
    'cpp',
    'WinDisplayProberJson',
    await readFile('schemas/displayprober-win-cpp.schema.json', 'utf8'),
  );
  const cpp = lines.join('\n');
  await writeFile('../cpp/gencode/acd-json.hpp', cpp, 'utf8');
}

main();
