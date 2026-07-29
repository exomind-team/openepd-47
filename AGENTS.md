# OpenEPD 4.7 Agent 交接规则

本文件面向继续开发本仓库的 AI Agent 和人类贡献者。

## 当前状态

- 仓库只完成资料审计、资料导入和开发计划。
- `driver/` 尚未实现正式驱动。
- `website/` 尚未初始化 Astro/Starlight。
- `materials/reference-firmware/ebook/` 是清理后的参考快照，不是最终目录结构。

不要把“资料已导入”误判为“驱动已完成”。

## 开始工作前

1. 阅读 `docs/HANDOFF.md`，确认哪些阶段已经完成。
2. 阅读 `docs/design/2026-07-29-openepd-47-design.md`。
3. 阅读 `docs/plans/2026-07-29-openepd-47-implementation-plan.md`。
4. 按尚未完成的子计划执行，并在每个阶段出口运行验证。
5. 新工作使用独立分支；不要直接在 `main` 开发功能。

## 不可更改的已确认事实

- 触摸控制器是 **GT911**，不是 FT5446U。
- GT911 首选依赖是 `espressif/esp_lcd_touch_gt911`。
- 显示屏为 E0470A01-AF-S，4.7 英寸，684×1216，黑白。
- 驱动与网站必须位于同一个仓库的 `driver/` 和 `website/`。
- 网站目标是 GitHub Pages，MVP 不建设账号或后端。

## 资料规则

- `materials/source-inventory.csv` 是原始资料的全量哈希清单。
- `materials/import-allowlist.csv` 是进入仓库的精确文件清单。
- `materials/excluded.csv` 记录未导入内容和原因。
- 厂商授权资料可保存在仓库。
- GT911 数据手册只登记公开来源；未确认再分发许可前不要镜像。
- 不要重新导入 FT5446U、`.cache`、嵌套 `.git`、备份、重复 ZIP、生成配置或真实凭据。
- 不要直接修改 `materials/reference-firmware/`；从中提取内容到 `driver/` 后记录来源和变更。

## 开发和验证

- 中文作为说明文档主语言；英文 API、文件名和代码术语附中文解释。
- ESP-IDF 固定在 5.5.4 系列，除非计划和 CI 同步更新。
- 新功能先写失败测试，再实施。
- 网站变更至少运行 Astro check、生产构建和相关 Playwright 测试。
- 驱动变更至少构建 `hello_epaper` 和 `ebook`。
- 硬件相关结论必须有本地资料、上游代码或实测记录支撑。
- 每个提交只处理一个可验证目标。

## 发布边界

推送功能分支、合并 PR、启用 Pages、创建标签和 Release 都会改变外部状态。执行这些操作前确认用户授权；不要自动发布未经硬件验证的固件。
