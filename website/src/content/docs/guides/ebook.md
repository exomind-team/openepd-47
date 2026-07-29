---
title: 电子书参考工程
description: 厂商电子书参考固件的来源、清理范围和使用边界。
---

## 位置

```text
materials/reference-firmware/ebook/
```

## 已完成清理

- 移除 `.cache`、`.vscode` 和嵌套 Git。
- 移除生成的 `sdkconfig` 与备份。
- 移除未知来源下载图片。
- 将 Wi‑Fi 演示凭据替换为公开占位值。
- 保留原始来源和导入后 SHA‑256。

## 包含内容

- EPDiy 显示组件及板级适配。
- TPS65185 和 PCA9555 参考实现。
- 电子书、相册、Web 服务和存储逻辑。
- 自定义中文字体和波形数据。

## 不包含

参考工程没有 GT911 触摸驱动。后续应通过乐鑫官方组件接入，不应从错误的 FT5446U 资料移植。

## 许可证

参考工程内的 EPDiy 代码受 LGPL-3.0 约束，详见仓库 `LICENSES/epdiy/`。厂商资料授权说明见 `LICENSES/vendor-materials/`。
