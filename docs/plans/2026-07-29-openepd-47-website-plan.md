# OpenEPD 4.7 网站与社区实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立平衡型中文门户，提供快速开始、在线文档、可追踪资料下载、驱动更新入口和社区项目展示。

**Architecture:** Astro + Starlight 输出纯静态站点；文档与项目用内容集合管理；资料和版本用 YAML 管理；社区投稿通过 GitHub Issue Forms/PR，不使用网站后端。

**Tech Stack:** Astro、Starlight、TypeScript、Bun、Zod、Playwright、axe。

**实施状态：** 网站 MVP 已完成；GitHub Pages 工作流已建立，等待主分支部署验收。

---

## 前置条件

- [资料治理与仓库基础计划](./2026-07-29-openepd-47-foundation-plan.md) 已完成。
- 驱动计划可并行执行；网站的驱动文档发布前必须以实际 API 和构建结果复核。

## Task 1：初始化 Starlight 与平衡型首页（已完成）

**Files:**

- Create: `D:\project\openepd-47\website\package.json`
- Create: `D:\project\openepd-47\website\bun.lock`
- Create: `D:\project\openepd-47\website\astro.config.mjs`
- Create: `D:\project\openepd-47\website\tsconfig.json`
- Create: `D:\project\openepd-47\website\src\content.config.ts`
- Create: `D:\project\openepd-47\website\src\styles\custom.css`
- Create: `D:\project\openepd-47\website\src\content\docs\index.mdx`
- Test: `D:\project\openepd-47\website\tests\home.spec.ts`

- [ ] 先写 Playwright 失败测试：首页存在“开始开发”“浏览资料”“项目展示”，三个入口键盘可达，移动端没有横向滚动。

- [ ] 用 `bun add --exact` 安装 Astro、Starlight、TypeScript、Playwright 和测试依赖，提交锁文件。

- [ ] 配置：

  ```js
  export default defineConfig({
    site: 'https://exomind-team.github.io',
    base: '/openepd-47',
    integrations: [starlight({ title: 'OpenEPD 4.7' })],
  });
  ```

- [ ] 实现已确认的平衡型首页：三入口、684×1216/4.7 英寸规格、最近更新、精选项目、贡献入口。视觉使用纸张白、墨黑与低饱和状态色。

- [ ] 运行：

  ```powershell
  Set-Location 'D:\project\openepd-47\website'
  bun install --frozen-lockfile
  bun run astro check
  bun run build
  bunx playwright test tests/home.spec.ts
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add website
  git -C 'D:\project\openepd-47' commit -m "feat(site): scaffold balanced documentation portal"
  ```

## Task 2：建立资料 Schema 与下载中心（已完成）

**Files:**

- Create: `D:\project\openepd-47\website\src\data\resources.yml`
- Create: `D:\project\openepd-47\website\scripts\validate-resources.ts`
- Create: `D:\project\openepd-47\website\public\resources\vendor\`
- Create: `D:\project\openepd-47\website\src\content\docs\resources\index.mdx`
- Test: `D:\project\openepd-47\website\tests\resources.spec.ts`

- [ ] 先写失败测试：缺授权状态、缺 SHA-256、本地文件不存在，或 `external-link` 同时设置本地镜像时构建失败。

- [ ] 从允许清单逐项复制厂商授权资料；文件名包含型号和版本，不覆盖旧版。

- [ ] GT911 只登记外部来源：

  ```yaml
  id: gt911-datasheet
  title: GT911 Data Sheet（数据手册）
  category: touch
  hardwareRevision: GT911
  sourceType: external-link
  sourceUrl: https://www.fortec-integrated.de/fileadmin/pdf/produkte/Touchcontroller/DDGroup/GT911_Datasheet.pdf
  redistribution: not-mirrored
  notes: 使用乐鑫官方驱动组件；仓库不镜像该 PDF
  ```

- [ ] 下载中心显示类型、大小、硬件版本、来源、授权状态、SHA-256 和下载/外链动作。

- [ ] 运行：

  ```powershell
  Set-Location 'D:\project\openepd-47\website'
  bun run validate:resources
  bun run build
  bunx playwright test tests/resources.spec.ts
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add website materials
  git -C 'D:\project\openepd-47' commit -m "feat(site): add traceable resource center"
  ```

## Task 3：补齐快速开始、硬件和驱动文档（已完成）

**Files:**

- Create: `D:\project\openepd-47\website\src\content\docs\quick-start\index.md`
- Create: `D:\project\openepd-47\website\src\content\docs\hardware\display.md`
- Create: `D:\project\openepd-47\website\src\content\docs\hardware\power.md`
- Create: `D:\project\openepd-47\website\src\content\docs\hardware\pinout.md`
- Create: `D:\project\openepd-47\website\src\content\docs\hardware\gt911.md`
- Create: `D:\project\openepd-47\website\src\content\docs\driver\api.md`
- Create: `D:\project\openepd-47\website\src\content\docs\driver\build.md`
- Create: `D:\project\openepd-47\website\src\content\docs\guides\ebook.md`
- Create: `D:\project\openepd-47\website\src\content\docs\faq.md`
- Test: `D:\project\openepd-47\website\tests\docs.spec.ts`

- [ ] 先写导航测试，覆盖首页→快速开始→构建→资料→故障排查。

- [ ] 每个硬件事实标注本地资料、上游代码或实测来源；未支撑的内容明确写“未验证”。

- [ ] GT911 文档链接：
  - <https://components.espressif.com/components/espressif/esp_lcd_touch_gt911>
  - <https://github.com/espressif/esp-bsp/tree/master/components/lcd_touch/esp_lcd_touch_gt911>
  - 资料中心的 GT911 外链记录

- [ ] 快速开始写明 ESP-IDF 版本、接线风险、构建命令、预期串口输出和失败诊断。

- [ ] 运行：

  ```powershell
  Set-Location 'D:\project\openepd-47\website'
  bun run astro check
  bun run check:links
  bun run build
  bunx playwright test tests/docs.spec.ts
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add website/src/content website/tests
  git -C 'D:\project\openepd-47' commit -m "docs: add hardware and driver guides"
  ```

## Task 4：建立项目展示与 GitHub 投稿（已完成）

**Files:**

- Create: `D:\project\openepd-47\website\src\content\projects\sample-project.mdx`
- Create: `D:\project\openepd-47\website\src\pages\projects\index.astro`
- Create: `D:\project\openepd-47\website\src\pages\projects\[...slug].astro`
- Create: `D:\project\openepd-47\.github\ISSUE_TEMPLATE\project-submission.yml`
- Create: `D:\project\openepd-47\.github\pull_request_template.md`
- Test: `D:\project\openepd-47\website\tests\projects.spec.ts`

- [ ] 先写 Schema 测试，要求名称、摘要、仓库 URL、封面、硬件、框架、许可证和提交者字段完整。

- [ ] 实现项目列表和按主控、框架、类型过滤；空结果和损坏图片有可访问降级。

- [ ] Issue Form 收集相同字段，并要求投稿者确认代码与图片授权；不上传固件和其他二进制。

- [ ] “提交项目”按钮链接到预填 Issue Form；维护者审核后用 PR 添加项目文件。

- [ ] 运行：

  ```powershell
  Set-Location 'D:\project\openepd-47\website'
  bun run validate:content
  bun run build
  bunx playwright test tests/projects.spec.ts
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add website .github
  git -C 'D:\project\openepd-47' commit -m "feat(community): add project showcase workflow"
  ```

## 阶段出口

- Astro 类型检查、生产构建、链接检查和 Playwright 全部通过。
- 首页三条主路径在桌面与移动端可用。
- 每个资料项具有来源和授权状态；GT911 PDF 只外链。
- 项目投稿不依赖自建后端，且有许可证与图片权利确认。
