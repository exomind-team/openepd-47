---
title: 构建环境
description: OpenEPD 4.7 的 ESP-IDF 环境约束与参考工程构建边界。
---

## 目标工具链

| 工具 | 目标 |
|---|---|
| ESP-IDF | 5.5.4 |
| Target | `esp32s3` |
| Build system | CMake + `idf.py` |
| GT911 | `espressif/esp_lcd_touch_gt911` |

## 参考工程

参考固件位于：

```text
materials/reference-firmware/ebook/
```

可以用以下命令进行探索性构建：

```bash
cd materials/reference-firmware/ebook
idf.py set-target esp32s3
idf.py build
```

> **尚未验证**
>
> 该参考工程尚未在公开仓库 CI 中完成 ESP-IDF 5.5.4 干净构建。构建失败时，请保留完整日志，不要通过提交本机 `sdkconfig` 或缓存绕过问题。

## 正式组件目标

计划中的正式组件位于 `driver/components/openepd47/`，会把以下边界集中管理：

- 板级 GPIO 和 I²C。
- TPS65185 电源时序。
- PCA9555 扩展 IO。
- EPDiy 显示初始化。
- GT911 复位、INT 地址选择和坐标转换。

完整步骤见仓库的[驱动实施计划](https://github.com/exomind-team/openepd-47/blob/main/docs/plans/2026-07-29-openepd-47-driver-plan.md)。
