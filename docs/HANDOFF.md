# OpenEPD 4.7 当前交接状态

**日期：** 2026-07-29

## 已完成

- 核实目标 GitHub 组织为 `exomind-team`。
- 核实屏幕型号、分辨率、GT911 触摸和 TPS65185 资料。
- 盘点原始资料目录的 600 个文件。
- 导入 108 个允许文件，排除 492 个缓存、重复、错误型号或不必要文件。
- 导入 4 份厂商授权的硬件文档。
- 导入清理后的电子书参考固件，替换演示 Wi‑Fi 凭据。
- 保存完整设计说明、总路线图和四份子计划。
- 保存 EPDiy 的 LGPL-3.0 许可证和精确上游提交信息。
- 建立 Astro + Starlight 网站：首页、资料中心、硬件/驱动文档和项目展示均已实现。
- 建立 5 项资料 Schema 与 SHA-256 校验，GT911 数据手册保持外链并标注来源。
- 建立 GitHub Issue Form 项目投稿、网站 CI 与 GitHub Pages 部署工作流。
- 网站生产地址设为 `https://exomind-team.github.io/openepd-47/`。

## 尚未完成

- 没有实现 `driver/components/openepd47`。
- 没有接入 GT911 官方组件。
- 没有验证现有参考固件能否在 ESP-IDF 5.5.4 干净构建。
- 没有建立 GitHub Release 自动化。
- 没有进行真实硬件冒烟测试。

## 推荐下一步

若优先保证硬件可用，从以下计划开始：

1. `docs/plans/2026-07-29-openepd-47-driver-plan.md`
2. Task 1：建立最小板级组件和失败基线。

网站 MVP 已完成，后续内容更新应同步维护：

1. `website/src/content/docs/`：开发文档。
2. `website/src/data/resources.yml`：资料来源、授权与哈希。
3. `website/src/data/projects.ts`：社区项目展示。
4. 修改后运行 `bun run check && bun run test:unit && bun run test:e2e && bun run build`。

## 关键提醒

- 真实触摸芯片是 **GT911**。
- `materials/reference-firmware/ebook/` 是资料快照，不应原样作为最终驱动架构。
- 参考固件内的 EPDiy 代码属于 LGPL-3.0 边界。
- GT911 数据手册目前只保存外部来源链接。
