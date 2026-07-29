# 资料目录与来源说明

原始资料来源于：

`D:\downloads\4.7inch墨水屏资料-英瑞达`

资料提供方已向项目发起人确认这些所获资料可以开源。仓库仍按来源和第三方许可证分别管理，避免把所有内容错误地统一声明为 Apache-2.0。

## 目录

- `vendor/display/`：显示屏规格书。
- `vendor/mechanical/`：结构与触摸 FPC 图纸。
- `vendor/power/`：TPS65185 相关资料。
- `reference-firmware/ebook/`：清理后的 ESP-IDF 电子书参考固件。
- `source-inventory.csv`：600 个原始文件的路径、大小、SHA-256 和处理决定。
- `import-allowlist.csv`：实际进入仓库的 108 个文件及导入后 SHA-256。
- `excluded.csv`：未导入文件和原因。
- `external-sources.yml`：只保留外链或上游引用的第三方资料。

## 已执行清理

- 排除错误型号 `FT5446U-DataSheet.pdf`。
- 排除原始 ZIP、缓存、`.vscode`、嵌套 `.git`、备份、生成的 `sdkconfig` 和未知来源下载图片。
- 不复制完整 `epdiy-upstream` 克隆，只记录上游 URL、提交和许可证。
- 将 `wifi_config.h` 中的演示 SSID/密码替换为公开占位值。

## 重新生成

在原始资料目录仍可用时运行：

```powershell
& '.\scripts\prepare-materials.ps1' `
  -Source 'D:\downloads\4.7inch墨水屏资料-英瑞达' `
  -Destination (Get-Location).Path
```

脚本不会写入原始资料目录。
