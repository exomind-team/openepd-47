$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$allowlistPath = Join-Path $repositoryRoot "materials\import-allowlist.csv"

Describe "repository material integrity" {
    It "preserves every imported file byte-for-byte after Git checkout" {
        $allowlist = Import-Csv -LiteralPath $allowlistPath
        $mismatches = @()

        foreach ($entry in $allowlist) {
            $nativePath = $entry.TargetPath.Replace("/", "\")
            $targetPath = Join-Path $repositoryRoot $nativePath
            $actualHash = (Get-FileHash -LiteralPath $targetPath -Algorithm SHA256).Hash.ToLowerInvariant()

            if ($actualHash -ne $entry.TargetSha256) {
                $mismatches += $entry.TargetPath
            }
        }

        $mismatches.Count | Should Be 0
    }
}
