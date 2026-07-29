# OpenEPD 4.7 资料治理与仓库基础计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立可审计的资料导入流程、许可证边界和单仓库基础结构。

**Architecture:** 原始资料保持只读；脚本生成全量清单；维护者只从显式白名单导入。原创代码采用 Apache-2.0，第三方代码和厂商资料分别保留许可证或授权说明。

**Tech Stack:** PowerShell 7、Git、YAML、Pester、gitleaks。

---

## Task 1：创建本地仓库与盘点测试

**Files:**

- Create: `D:\project\openepd-47\scripts\inventory-materials.ps1`
- Create: `D:\project\openepd-47\scripts\tests\inventory-materials.Tests.ps1`
- Create: `D:\project\openepd-47\materials\source-inventory.csv`

- [ ] 创建本地仓库和工作分支：

  ```powershell
  New-Item -ItemType Directory -Path 'D:\project\openepd-47'
  git -C 'D:\project\openepd-47' init
  git -C 'D:\project\openepd-47' switch -c codex/openepd-47-foundation
  ```

- [ ] 先写 Pester 失败测试，要求每条记录具有 `RelativePath`、`Bytes`、`Extension`、`Sha256`、`Decision` 和 `Reason`；源目录不得发生写入。

- [ ] 实现只读盘点脚本，核心参数固定为：

  ```powershell
  param(
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$OutputCsv
  )
  ```

- [ ] 脚本必须用 `Get-FileHash -Algorithm SHA256`，路径以相对形式写入 CSV，默认决定为 `review`，不得自动批准。

- [ ] 运行：

  ```powershell
  Invoke-Pester 'D:\project\openepd-47\scripts\tests\inventory-materials.Tests.ps1'
  pwsh -File 'D:\project\openepd-47\scripts\inventory-materials.ps1' `
    -Source 'D:\downloads\4.7inch墨水屏资料-英瑞达' `
    -OutputCsv 'D:\project\openepd-47\materials\source-inventory.csv'
  ```

  预期：Pester 全部通过；CSV 有 600 条文件记录，源目录内容与时间戳未变化。

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add scripts materials/source-inventory.csv
  git -C 'D:\project\openepd-47' commit -m "chore: inventory source materials"
  ```

## Task 2：建立允许清单与排除规则

**Files:**

- Create: `D:\project\openepd-47\materials\import-allowlist.yml`
- Create: `D:\project\openepd-47\materials\excluded.yml`
- Create: `D:\project\openepd-47\scripts\verify-materials.ps1`
- Create: `D:\project\openepd-47\scripts\tests\verify-materials.Tests.ps1`

- [ ] 先写失败测试：FT5446U、`.cache`、`.git`、`.bak`、`*backup*`、ZIP、`wifi_config.h` 或未登记 SHA-256 进入允许清单时返回非零。

- [ ] `excluded.yml` 写明以下规则：

  ```yaml
  - pattern: "**/FT5446U-DataSheet.pdf"
    reason: "触摸型号错误且文件带保密标识"
  - pattern: "**/.cache/**"
    reason: "本机构建缓存"
  - pattern: "**/.git/**"
    reason: "嵌套仓库元数据"
  - pattern: "**/*backup*"
    reason: "备份文件"
  - pattern: "**/*.zip"
    reason: "重复打包物"
  - pattern: "**/wifi_config.h"
    reason: "包含演示 Wi-Fi 凭据"
  ```

- [ ] 对厂商已确认可公开的资料逐项写入允许清单：

  ```yaml
  - sourcePath: "相对于资料源的精确路径"
    targetPath: "website/public/resources/vendor/规范化文件名"
    hardwareRevision: "资料标注的精确型号或版本"
    redistribution: "authorized"
    sha256: "source-inventory.csv 中的精确值"
  ```

- [ ] GT911 数据手册不进入允许清单；它在网站阶段作为 `external-link` 登记。

- [ ] 运行：

  ```powershell
  Invoke-Pester 'D:\project\openepd-47\scripts\tests\verify-materials.Tests.ps1'
  pwsh -File 'D:\project\openepd-47\scripts\verify-materials.ps1' `
    -Inventory 'D:\project\openepd-47\materials\source-inventory.csv' `
    -Allowlist 'D:\project\openepd-47\materials\import-allowlist.yml' `
    -Excluded 'D:\project\openepd-47\materials\excluded.yml'
  ```

  预期：退出码 0；每个允许项存在且哈希一致，禁止项均未获准。

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add materials scripts
  git -C 'D:\project\openepd-47' commit -m "chore: define material import policy"
  ```

## Task 3：建立治理文件和目录边界

**Files:**

- Create: `D:\project\openepd-47\README.md`
- Create: `D:\project\openepd-47\LICENSE`
- Create: `D:\project\openepd-47\LICENSES\README.md`
- Create: `D:\project\openepd-47\LICENSES\vendor-materials\README.md`
- Create: `D:\project\openepd-47\CONTRIBUTING.md`
- Create: `D:\project\openepd-47\SECURITY.md`
- Create: `D:\project\openepd-47\.gitignore`
- Create: `D:\project\openepd-47\.gitattributes`
- Create: `D:\project\openepd-47\driver\.gitkeep`
- Create: `D:\project\openepd-47\website\.gitkeep`
- Create: `D:\project\openepd-47\.github\.gitkeep`

- [ ] 先写结构测试，要求根目录存在 `driver/`、`website/`、`LICENSES/`、`.github/`，根 LICENSE 为 Apache-2.0。

- [ ] 在 `LICENSES/README.md` 明确：

  ```text
  原创代码 / Original code: Apache-2.0
  EPDiy: LGPL-3.0，见 LICENSES/epdiy/
  厂商授权资料 / Vendor-authorized materials: 见 LICENSES/vendor-materials/
  外链资料 / External links: 不在仓库内再分发
  ```

- [ ] `.gitignore` 排除 `.cache/`、`build/`、`sdkconfig*`、`node_modules/`、`website/dist/`、`website/.astro/`、`*.bak`、`*.zip`、`.env*` 和本地 Wi‑Fi 配置。

- [ ] 运行：

  ```powershell
  gitleaks detect --source 'D:\project\openepd-47' --no-git
  git -C 'D:\project\openepd-47' status --short
  ```

  预期：无秘密；状态只包含本任务预期文件。

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add README.md LICENSE LICENSES CONTRIBUTING.md SECURITY.md .gitignore .gitattributes driver website .github
  git -C 'D:\project\openepd-47' commit -m "chore: establish repository governance"
  ```

## 阶段出口

- `verify-materials.ps1` 和 Pester 全部通过。
- FT5446U、凭据、缓存、备份和重复包不在允许清单。
- 每个允许公开资料都有精确来源路径、目标路径、硬件版本、授权状态和 SHA-256。
- 工作区只具备仓库基础，不包含未经审计的驱动或网站内容。
