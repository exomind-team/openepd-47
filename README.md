# OpenEPD 4.7

4.7 英寸、684×1216 黑白电子纸的开源资料、驱动与项目展示门户。

Open-source resources, drivers, and a community showcase for the 4.7-inch 684×1216 monochrome e-paper display.

> 当前状态 / Current status：**资料基线与网站 MVP 已建立；驱动仍处于参考代码整理阶段。**

网站 / Website：<https://exomind-team.github.io/openepd-47/>

## 仓库目标

- `driver/`：ESP-IDF 板级适配、GT911 触摸、最小示例和电子书示例。
- `website/`：Astro + Starlight 文档、资料下载、版本更新和社区项目展示。
- `materials/`：已授权资料、清理后的参考固件和完整来源清单。
- `docs/`：已确认设计、总路线图和分阶段实施计划。

## 从这里开始

1. 浏览 [在线网站](https://exomind-team.github.io/openepd-47/)。
2. 阅读 [当前交接状态](docs/HANDOFF.md)。
3. 阅读 [设计说明](docs/design/2026-07-29-openepd-47-design.md)。
4. 从 [总实施路线图](docs/plans/2026-07-29-openepd-47-implementation-plan.md) 选择下一阶段。
5. 查阅 [资料目录与来源说明](materials/README.md)。

## 已核实硬件

| 项目 | 信息 |
|---|---|
| 显示屏 | E0470A01-AF-S |
| 尺寸 | 4.7 英寸 |
| 分辨率 | 684×1216 |
| 显示 | 黑白电子纸 |
| 触摸控制器 | **GT911** |
| 触摸接口 | I²C，VDD/GND/SDA/SCL/RST/INT |
| 电源相关 | TPS65185 |
| 目标主控 | ESP32-S3 |

触摸控制器不是 FT5446U。错误型号资料没有导入仓库。

## 重要来源

- GT911 驱动：[Espressif `esp_lcd_touch_gt911`](https://components.espressif.com/components/espressif/esp_lcd_touch_gt911)
- GT911 源码：[Espressif ESP-BSP](https://github.com/espressif/esp-bsp/tree/master/components/lcd_touch/esp_lcd_touch_gt911)
- EPDiy 上游：[vroland/epdiy](https://github.com/vroland/epdiy)
- 其他来源与再分发状态：[materials/external-sources.yml](materials/external-sources.yml)

## 许可证

仓库原创代码和文档默认使用 [Apache-2.0](LICENSE)。EPDiy、厂商资料和其他第三方内容保持各自的许可证或授权边界，详见 [LICENSES/README.md](LICENSES/README.md)。

## 项目归属

由 [ExoMind Collective](https://github.com/exomind-team) 发起维护。
