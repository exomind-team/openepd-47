---
title: 快速开始
description: 从资料基线开始理解 OpenEPD 4.7，并准备 ESP-IDF 开发环境。
---

OpenEPD 4.7 当前公开了经过审计的硬件资料和参考固件。正式的 `openepd47` 板级组件仍在开发计划中，因此本页先帮助你建立可靠的环境和事实基线。

> **当前状态**
>
> 仓库中的 `materials/reference-firmware/ebook/` 是厂商参考工程的清理快照，不等同于已经验证完成的正式驱动。

## 你需要准备

- ESP32-S3 开发板或对应控制板。
- E0470A01-AF-S 4.7 英寸、684×1216 黑白电子纸。
- GT911 触摸 FPC。
- ESP-IDF 5.5.4。
- Git 与 Python。

## 1. 克隆仓库

```bash
git clone https://github.com/exomind-team/openepd-47.git
cd openepd-47
```

## 2. 先确认资料

在接线和烧录前，先查看：

- [显示屏规格与限制](/openepd-47/hardware/display/)
- [接口与引脚](/openepd-47/hardware/pinout/)
- [GT911 触摸](/openepd-47/hardware/gt911/)
- [资料中心](/openepd-47/resources/)

## 3. 检查 ESP-IDF

```bash
idf.py --version
```

目标版本是 ESP-IDF 5.5.4。后续正式示例将通过仓库 CI 固定并验证该版本。

## 4. 理解两个代码入口

| 目录 | 用途 |
|---|---|
| `materials/reference-firmware/ebook/` | 已脱敏的原始参考工程，用于迁移和比对 |
| `driver/` | 正式板级组件与可复现示例的目标目录 |

目前请不要把参考工程直接发布为稳定驱动。下一步可阅读[构建环境](/openepd-47/driver/build/)和[电子书参考工程](/openepd-47/guides/ebook/)。

## 安全提醒

电子纸电源涉及多路升压和负压。未核对 TPS65185 电源时序、FPC 方向和板级引脚前，不要带电插拔屏幕。
