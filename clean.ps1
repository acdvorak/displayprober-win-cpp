# Windows PowerShell v5.1
#Requires -Version 5.1
#Requires -PSEdition Desktop

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue @(
  'build/',
  'out/',
  'src/cpp/build/',
  'src/cpp/out/'
)
