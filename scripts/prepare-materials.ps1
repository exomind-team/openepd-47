[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$Destination
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-NormalizedRelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$FullName
    )

    $relativePath = $FullName.Substring($Root.Length).TrimStart("\")
    return $relativePath.Replace("\", "/")
}

function Get-ImportDecision {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $vendorTargets = @{
        "E0470A01-AF-CF(A) 260713-1.dwg" = "materials/vendor/mechanical/E0470A01-AF-CF-A-260713-1.dwg"
        "E0470A01-AF-CF(A) 260713-1.pdf" = "materials/vendor/mechanical/E0470A01-AF-CF-A-260713-1.pdf"
        "ESP32S3_TPS65185.pdf"           = "materials/vendor/power/ESP32S3-TPS65185.pdf"
        "英瑞达  E0470A01-AF-S A版规格书.pdf" = "materials/vendor/display/E0470A01-AF-S-specification.pdf"
    }

    if ($vendorTargets.ContainsKey($RelativePath)) {
        return @{
            Decision = "Import"
            Reason = "Vendor-authorized hardware document"
            TargetPath = $vendorTargets[$RelativePath]
        }
    }

    if ($RelativePath -eq "FT5446U-DataSheet.pdf") {
        return @{
            Decision = "Exclude"
            Reason = "Wrong touch controller and confidential marking"
            TargetPath = ""
        }
    }

    if ($RelativePath.StartsWith("epdiy-upstream/", [System.StringComparison]::OrdinalIgnoreCase)) {
        return @{
            Decision = "Exclude"
            Reason = "Upstream repository is referenced by URL and commit instead of duplicated"
            TargetPath = ""
        }
    }

    $firmwarePrefix = "4.7-inch(684x1216)_ebook/"
    if ($RelativePath.StartsWith($firmwarePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        $firmwarePath = $RelativePath.Substring($firmwarePrefix.Length)
        $excludedPath = (
            $firmwarePath.StartsWith(".cache/", [System.StringComparison]::OrdinalIgnoreCase) -or
            $firmwarePath.StartsWith(".vscode/", [System.StringComparison]::OrdinalIgnoreCase) -or
            $firmwarePath.StartsWith("build/", [System.StringComparison]::OrdinalIgnoreCase) -or
            $firmwarePath.StartsWith(".git/", [System.StringComparison]::OrdinalIgnoreCase) -or
            $firmwarePath -eq ".clangd" -or
            $firmwarePath -eq "sdkconfig" -or
            $firmwarePath -eq "sdkconfig.old" -or
            $firmwarePath -eq "pictures/downloaded-image.png" -or
            $firmwarePath.EndsWith(".bak", [System.StringComparison]::OrdinalIgnoreCase) -or
            $firmwarePath.Contains("backup")
        )

        if ($excludedPath) {
            return @{
                Decision = "Exclude"
                Reason = "Generated local unknown-origin or backup file"
                TargetPath = ""
            }
        }

        $targetPath = "materials/reference-firmware/ebook/" + $firmwarePath
        if ($firmwarePath -eq "main/common/wifi_config.h") {
            return @{
                Decision = "Sanitize"
                Reason = "Replace demo credentials with public placeholders"
                TargetPath = $targetPath
            }
        }

        return @{
            Decision = "Import"
            Reason = "Vendor-authorized reference firmware"
            TargetPath = $targetPath
        }
    }

    return @{
        Decision = "Exclude"
        Reason = "Duplicate archive or file outside the curated import set"
        TargetPath = ""
    }
}

function Write-SanitizedWifiConfig {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $content = @'
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

/*
 * Public demo configuration.
 * Replace these placeholders locally before connecting to Wi-Fi.
 */
#define DEMO_WIFI_SSID     "your_wifi_ssid"
#define DEMO_WIFI_PASSWORD "your_wifi_password"

#endif /* WIFI_CONFIG_H */
'@

    $utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $content, $utf8WithoutBom)
}

$sourceRoot = (Resolve-Path -LiteralPath $Source).Path.TrimEnd("\")
$destinationRoot = [System.IO.Path]::GetFullPath($Destination).TrimEnd("\")
$materialsRoot = Join-Path $destinationRoot "materials"
New-Item -ItemType Directory -Path $materialsRoot -Force | Out-Null

$inventory = [System.Collections.Generic.List[object]]::new()
$sourceFiles = Get-ChildItem -LiteralPath $sourceRoot -Recurse -Force -File |
    Sort-Object FullName

foreach ($file in $sourceFiles) {
    $relativePath = Get-NormalizedRelativePath -Root $sourceRoot -FullName $file.FullName
    $decision = Get-ImportDecision -RelativePath $relativePath
    $sourceHash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $targetHash = ""

    if ($decision.Decision -in @("Import", "Sanitize")) {
        $targetNativePath = $decision.TargetPath.Replace("/", "\")
        $targetFullPath = [System.IO.Path]::GetFullPath((Join-Path $destinationRoot $targetNativePath))
        if (-not $targetFullPath.StartsWith($destinationRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Resolved target path escapes destination: $targetFullPath"
        }

        $targetDirectory = Split-Path -Parent $targetFullPath
        New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null

        if ($decision.Decision -eq "Sanitize") {
            Write-SanitizedWifiConfig -Path $targetFullPath
        }
        else {
            Copy-Item -LiteralPath $file.FullName -Destination $targetFullPath -Force
        }

        $targetHash = (Get-FileHash -LiteralPath $targetFullPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }

    $inventory.Add([pscustomobject]@{
        SourcePath = $relativePath
        Bytes = $file.Length
        Extension = $file.Extension.ToLowerInvariant()
        Sha256 = $sourceHash
        Decision = $decision.Decision
        Reason = $decision.Reason
        TargetPath = $decision.TargetPath
        TargetSha256 = $targetHash
    })
}

$inventoryPath = Join-Path $materialsRoot "source-inventory.csv"
$allowlistPath = Join-Path $materialsRoot "import-allowlist.csv"
$excludedPath = Join-Path $materialsRoot "excluded.csv"

$inventory |
    Export-Csv -LiteralPath $inventoryPath -NoTypeInformation -Encoding UTF8

$inventory |
    Where-Object { $_.Decision -in @("Import", "Sanitize") } |
    Select-Object SourcePath, TargetPath, Decision, Reason,
        @{ Name = "SourceSha256"; Expression = { $_.Sha256 } },
        TargetSha256 |
    Export-Csv -LiteralPath $allowlistPath -NoTypeInformation -Encoding UTF8

$inventory |
    Where-Object { $_.Decision -eq "Exclude" } |
    Select-Object SourcePath, Reason, Sha256 |
    Export-Csv -LiteralPath $excludedPath -NoTypeInformation -Encoding UTF8

$importedCount = ($inventory | Where-Object { $_.Decision -in @("Import", "Sanitize") }).Count
$excludedCount = ($inventory | Where-Object { $_.Decision -eq "Exclude" }).Count

Write-Output "[OK] Inventory: $($inventory.Count) files"
Write-Output "[OK] Imported: $importedCount files"
Write-Output "[OK] Excluded: $excludedCount files"
