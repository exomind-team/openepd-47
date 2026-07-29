$ErrorActionPreference = "Stop"

$scriptPath = Join-Path (Split-Path -Parent $PSScriptRoot) "prepare-materials.ps1"
$sourcePath = "D:\downloads\4.7inch墨水屏资料-英瑞达"

Describe "prepare-materials.ps1" {
    BeforeAll {
        $script:destinationPath = Join-Path $TestDrive "openepd-test"
        New-Item -ItemType Directory -Path $destinationPath -Force | Out-Null
        & $scriptPath -Source $sourcePath -Destination $destinationPath
    }

    It "inventories every source file without modifying the source" {
        $inventoryPath = Join-Path $destinationPath "materials\source-inventory.csv"
        $inventory = Import-Csv -LiteralPath $inventoryPath

        $inventory.Count | Should Be 600
        ($inventory | Where-Object { -not $_.Sha256 }).Count | Should Be 0
    }

    It "excludes the wrong FT5446U document and nested repositories" {
        $allowlistPath = Join-Path $destinationPath "materials\import-allowlist.csv"
        $allowlist = Import-Csv -LiteralPath $allowlistPath

        ($allowlist.SourcePath -match "FT5446U").Count | Should Be 0
        ($allowlist.SourcePath -match "epdiy-upstream").Count | Should Be 0
        ($allowlist.SourcePath -match "/\.git/").Count | Should Be 0
    }

    It "copies the four authorized vendor hardware documents" {
        $vendorRoot = Join-Path $destinationPath "materials\vendor"

        (Test-Path -LiteralPath (Join-Path $vendorRoot "display\E0470A01-AF-S-specification.pdf")) | Should Be $true
        (Test-Path -LiteralPath (Join-Path $vendorRoot "mechanical\E0470A01-AF-CF-A-260713-1.pdf")) | Should Be $true
        (Test-Path -LiteralPath (Join-Path $vendorRoot "mechanical\E0470A01-AF-CF-A-260713-1.dwg")) | Should Be $true
        (Test-Path -LiteralPath (Join-Path $vendorRoot "power\ESP32S3-TPS65185.pdf")) | Should Be $true
    }

    It "sanitizes the Wi-Fi configuration in the reference firmware" {
        $wifiPath = Join-Path $destinationPath "materials\reference-firmware\ebook\main\common\wifi_config.h"
        $wifiContent = Get-Content -LiteralPath $wifiPath -Raw

        $wifiContent | Should Not Match "123456789"
        $wifiContent | Should Match "your_wifi_ssid"
        $wifiContent | Should Match "your_wifi_password"
    }

    It "does not copy caches generated configs backups or unknown downloaded images" {
        $referenceRoot = Join-Path $destinationPath "materials\reference-firmware\ebook"

        (Test-Path -LiteralPath (Join-Path $referenceRoot ".cache")) | Should Be $false
        (Test-Path -LiteralPath (Join-Path $referenceRoot ".vscode")) | Should Be $false
        (Test-Path -LiteralPath (Join-Path $referenceRoot "sdkconfig")) | Should Be $false
        (Test-Path -LiteralPath (Join-Path $referenceRoot "sdkconfig.old")) | Should Be $false
        (Test-Path -LiteralPath (Join-Path $referenceRoot "pictures\downloaded-image.png")) | Should Be $false
    }

    It "records source and imported hashes for every allowed file" {
        $allowlistPath = Join-Path $destinationPath "materials\import-allowlist.csv"
        $allowlist = Import-Csv -LiteralPath $allowlistPath

        $allowlist.Count | Should BeGreaterThan 90
        ($allowlist | Where-Object { -not $_.SourceSha256 }).Count | Should Be 0
        ($allowlist | Where-Object { -not $_.TargetSha256 }).Count | Should Be 0
    }
}
