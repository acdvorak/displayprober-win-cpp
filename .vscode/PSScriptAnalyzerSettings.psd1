@{
  Rules = @{
    PSUseCompatibleSyntax = @{
      Enable = $true
      TargetVersions = @(
        '5.1'
      )
    }

    PSUseCompatibleCommands = @{
      Enable = $true
      TargetProfiles = @(
        # Windows 10 and newer.
        # See https://learn.microsoft.com/en-us/powershell/utility-modules/psscriptanalyzer/rules/usecompatiblecommands?view=ps-modules
        'win-48_x64_10.0.17763.0_5.1.17763.316_x64_4.0.30319.42000_framework'
      )
    }

    PSUseCompatibleTypes = @{
      Enable = $true
      TargetProfiles = @(
        # Windows 10 and newer.
        # See https://learn.microsoft.com/en-us/powershell/utility-modules/psscriptanalyzer/rules/usecompatibletypes?view=ps-modules
        'win-48_x64_10.0.17763.0_5.1.17763.316_x64_4.0.30319.42000_framework'
      )
    }
  }

  ExcludeRules = @(
    'PSAvoidUsingWriteHost'
  )
}
