# OpenEPD 4.7 CI、发布与验收计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立可靠的 PR 检查、GitHub Pages 部署和驱动 Release 流程，并用真实硬件完成首发前验收。

**Architecture:** 网站与驱动 CI 独立运行；`main` 部署 Pages；`driver-v*` 标签生成 Release。任何发布都必须在资料审计、自动检查和硬件冒烟全部通过后进行。

**Tech Stack:** GitHub Actions、Bun、Playwright、ESP-IDF 5.5.4、actionlint、gitleaks、GitHub Pages、GitHub Releases。

---

## 前置条件

- [驱动与示例计划](./2026-07-29-openepd-47-driver-plan.md) 已完成。
- [网站与社区计划](./2026-07-29-openepd-47-website-plan.md) 已完成。

## Task 1：建立 PR 质量门槛

**Files:**

- Create: `D:\project\openepd-47\.github\workflows\site-ci.yml`
- Create: `D:\project\openepd-47\.github\workflows\driver-ci.yml`
- Create: `D:\project\openepd-47\.github\dependabot.yml`

- [ ] 先运行 `actionlint` 验证尚不存在的工作流，保留失败作为基线。

- [ ] `site-ci.yml` 在 `website/**`、资料台账或自身变化时执行：冻结安装、Astro check、资源/内容/链接校验、构建、Playwright。

- [ ] `driver-ci.yml` 固定 ESP-IDF 5.5.4，构建 `hello_epaper`、`ebook` 和测试应用，并执行 gitleaks。

- [ ] Dependabot 每月检查 GitHub Actions 和网站依赖；硬件依赖禁止自动合并。

- [ ] 运行：

  ```powershell
  $workflowFiles = Get-ChildItem 'D:\project\openepd-47\.github\workflows\*.yml'
  $workflowFiles | ForEach-Object { actionlint $_.FullName }
  Set-Location 'D:\project\openepd-47\website'
  bun run check
  Set-Location 'D:\project\openepd-47\driver\examples\hello_epaper'
  idf.py build
  Set-Location 'D:\project\openepd-47\driver\examples\ebook'
  idf.py build
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add .github
  git -C 'D:\project\openepd-47' commit -m "ci: validate site driver and dependencies"
  ```

## Task 2：建立 GitHub Pages 工作流

**Files:**

- Create: `D:\project\openepd-47\.github\workflows\pages.yml`
- Modify: `D:\project\openepd-47\website\astro.config.mjs`
- Test: `D:\project\openepd-47\website\tests\base-path.spec.ts`

- [ ] 先写测试：所有站内资源和导航在 `/openepd-47/` 基础路径下可用，确认配置错误时失败。

- [ ] `pages.yml` 只在 `main` 且网站检查通过后部署 `website/dist`，权限固定：

  ```yaml
  permissions:
    contents: read
    pages: write
    id-token: write
  ```

- [ ] 使用官方 `actions/configure-pages`、`actions/upload-pages-artifact` 和 `actions/deploy-pages` 的固定主版本。

- [ ] 运行：

  ```powershell
  actionlint 'D:\project\openepd-47\.github\workflows\pages.yml'
  Set-Location 'D:\project\openepd-47\website'
  bun run build
  bunx playwright test tests/base-path.spec.ts
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add .github/workflows/pages.yml website
  git -C 'D:\project\openepd-47' commit -m "ci: add GitHub Pages deployment"
  ```

## Task 3：建立驱动 Release 工作流

**Files:**

- Create: `D:\project\openepd-47\.github\workflows\driver-release.yml`
- Create: `D:\project\openepd-47\scripts\package-release.ps1`
- Create: `D:\project\openepd-47\scripts\tests\package-release.Tests.ps1`
- Create: `D:\project\openepd-47\scripts\hardware-smoke-test.md`
- Create: `D:\project\openepd-47\CHANGELOG.md`

- [ ] 先写 Pester 测试，要求缺任一固件、许可证告知、源码归档或 SHA-256 时打包失败。

- [ ] 工作流只响应 `driver-v*` 标签；构建两个示例并生成：

  ```text
  openepd47-hello-<version>.zip
  openepd47-ebook-<version>.zip
  openepd47-source-<version>.zip
  THIRD_PARTY_NOTICES.txt
  SHA256SUMS.txt
  ```

- [ ] Release 说明链接对应文档版本，并列出已完成的硬件冒烟记录。

- [ ] 运行：

  ```powershell
  Invoke-Pester 'D:\project\openepd-47\scripts\tests\package-release.Tests.ps1'
  actionlint 'D:\project\openepd-47\.github\workflows\driver-release.yml'
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add .github scripts CHANGELOG.md
  git -C 'D:\project\openepd-47' commit -m "ci: add reproducible driver releases"
  ```

## Task 4：真实硬件与全仓首发验收

**Files:**

- Create: `D:\project\openepd-47\docs\release-checklist.md`
- Create: `D:\project\openepd-47\docs\hardware-validation\openepd47-v1.md`
- Create: `D:\project\openepd-47\website\src\data\releases.yml`
- Modify: `D:\project\openepd-47\CHANGELOG.md`

- [ ] 在真实 ESP32-S3 + 4.7 英寸屏幕上记录：冷启动、全刷、局刷、五点触摸、四方向旋转、休眠、唤醒、断电恢复和连续运行。

- [ ] 对所有公开文件重新计算 SHA-256，与资源台账逐项比对。

- [ ] 执行全仓验证：

  ```powershell
  pwsh -File 'D:\project\openepd-47\scripts\verify-materials.ps1' `
    -Inventory 'D:\project\openepd-47\materials\source-inventory.csv' `
    -Allowlist 'D:\project\openepd-47\materials\import-allowlist.yml' `
    -Excluded 'D:\project\openepd-47\materials\excluded.yml'
  Set-Location 'D:\project\openepd-47\website'
  bun run check
  bunx playwright test
  Set-Location 'D:\project\openepd-47\driver\examples\hello_epaper'
  idf.py fullclean
  idf.py build
  Set-Location 'D:\project\openepd-47\driver\examples\ebook'
  idf.py fullclean
  idf.py build
  gitleaks detect --source 'D:\project\openepd-47'
  git -C 'D:\project\openepd-47' status --short
  ```

  预期：全部退出码 0；Git 工作区干净；硬件记录没有未解释失败。

- [ ] 提交验收记录：

  ```powershell
  git -C 'D:\project\openepd-47' add docs website/src/data/releases.yml CHANGELOG.md
  git -C 'D:\project\openepd-47' commit -m "docs: record OpenEPD 4.7 release validation"
  ```

- [ ] 停在发布边界，向用户展示提交列表、CI 结果、资料摘要、硬件记录和拟发布对象。

- [ ] 只有用户再次明确批准后，才创建 `exomind-team/openepd-47`、推送分支、启用 Pages 和创建 `driver-v0.1.0` Release。

## 阶段出口

- Action lint、本地全仓验证和真实硬件清单全部通过。
- Pages 与 Release 工作流权限最小化、版本固定、产物可校验。
- 公开发布前存在明确人工批准点；计划执行本身不会自动对外发布。
