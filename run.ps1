#Requires -Version 5.1

& {
  Push-Location src/ts/
  try {
    npx tsx probe.ts
  }
  finally {
    Pop-Location
  }
}
