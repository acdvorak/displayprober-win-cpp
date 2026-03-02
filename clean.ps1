#Requires -Version 5.1

Remove-Item -Recurse -Force -ErrorAction SilentlyContinue @(
  'build/',
  'out/',
  'src/cpp/build/',
  'src/cpp/out/'
)
