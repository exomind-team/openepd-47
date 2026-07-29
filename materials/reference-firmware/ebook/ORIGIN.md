# 参考固件来源

来源：厂商提供的 `4.7-inch(684x1216)_ebook` ESP-IDF 工程。

导入日期：2026-07-29

## 导入处理

- 保留参考源码、组件、工具、演示文本、依赖锁和 `sdkconfig.defaults`。
- 移除 `.cache`、`.vscode`、`.clangd`、`sdkconfig`、`sdkconfig.old`、备份和未知来源下载图片。
- 将 `main/common/wifi_config.h` 的演示凭据替换为占位值。
- 完整文件哈希见仓库根目录下的 `materials/import-allowlist.csv`。

## 重要说明

- 这是供后续迁移和比对的资料快照，不是最终驱动目录。
- 其中没有 GT911 实现。
- `components/epdiy` 受 LGPL-3.0 约束，许可证见 `LICENSES/epdiy/LICENSE`。
