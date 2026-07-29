# OpenEPD 4.7 驱动与示例实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立 ESP-IDF 5.5.4 下可复现构建的显示、供电、IO 与 GT911 触摸集成层，并提供最小示例和清理后的电子书示例。

**Architecture:** `openepd47` 是原创板级适配组件；GT911 使用乐鑫官方组件；EPDiy 保留 LGPL-3.0 与精确上游提交；硬件相关魔法数字集中在板级配置。

**Tech Stack:** ESP-IDF 5.5.4、ESP32-S3、C/CMake、Unity、EPDiy、`espressif/esp_lcd_touch_gt911`。

---

## 前置条件

- [资料治理与仓库基础计划](./2026-07-29-openepd-47-foundation-plan.md) 已完成。
- ESP-IDF 5.5.4 环境可用，执行 `idf.py --version` 返回对应版本。

## Task 1：建立最小板级组件和失败基线

**Files:**

- Create: `D:\project\openepd-47\driver\components\openepd47\CMakeLists.txt`
- Create: `D:\project\openepd-47\driver\components\openepd47\idf_component.yml`
- Create: `D:\project\openepd-47\driver\components\openepd47\include\openepd47.h`
- Create: `D:\project\openepd-47\driver\components\openepd47\include\openepd47_board.h`
- Create: `D:\project\openepd-47\driver\components\openepd47\openepd47.c`
- Create: `D:\project\openepd-47\driver\examples\hello_epaper\CMakeLists.txt`
- Create: `D:\project\openepd-47\driver\examples\hello_epaper\main\CMakeLists.txt`
- Create: `D:\project\openepd-47\driver\examples\hello_epaper\main\app_main.c`

- [ ] 先创建调用 `openepd47_init()` 的最小示例，运行 `idf.py build`，记录链接失败。

- [ ] 定义窄接口：

  ```c
  typedef struct {
      bool enable_touch;
      uint16_t width;
      uint16_t height;
  } openepd47_config_t;

  esp_err_t openepd47_init(const openepd47_config_t *config);
  esp_err_t openepd47_draw_test_pattern(void);
  esp_err_t openepd47_sleep(void);
  ```

- [ ] 组件清单锁定：

  ```yaml
  dependencies:
    idf: ">=5.5,<5.6"
    espressif/esp_lcd_touch_gt911: "1.2.0~3"
  ```

- [ ] 把面板 684×1216、I²C GPIO39/40 和其他已核实板级引脚集中在 `openepd47_board.h`；不能从不明文件猜测缺失引脚。

- [ ] 运行：

  ```powershell
  Set-Location 'D:\project\openepd-47\driver\examples\hello_epaper'
  idf.py set-target esp32s3
  idf.py build
  ```

  预期：构建成功，`dependencies.lock` 锁定 GT911 官方组件。

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add driver
  git -C 'D:\project\openepd-47' commit -m "feat(driver): add minimal board component"
  ```

## Task 2：接入 EPDiy、TPS65185 与 PCA9555

**Files:**

- Create: `D:\project\openepd-47\driver\components\epdiy\UPSTREAM.md`
- Create: `D:\project\openepd-47\LICENSES\epdiy\LICENSE`
- Create: `D:\project\openepd-47\driver\components\openepd47\openepd47_display.c`
- Create: `D:\project\openepd-47\driver\components\openepd47\openepd47_power.c`
- Create: `D:\project\openepd-47\driver\components\openepd47\openepd47_io.c`
- Create: `D:\project\openepd-47\driver\test_apps\geometry\`
- Test: `D:\project\openepd-47\driver\test_apps\geometry\main\test_geometry.c`

- [ ] 先写 Unity 测试，覆盖宽高、0/90/180/270 度旋转和越界坐标，运行测试应用构建并确认未实现时失败。

- [ ] 从 `vroland/epdiy` 精确提交 `8781397a7154a19fc1b458dbef2aa465ab03cb10` 导入所需代码，保留 LGPL-3.0。

- [ ] `UPSTREAM.md` 记录上游 URL、完整提交、导入日期和实际本地补丁。

- [ ] 从允许清单提取 TPS65185/PCA9555 必要实现；不能分离授权边界时，依据公开数据手册重写薄适配层。

- [ ] 所有上电、状态、刷新和休眠错误通过 `esp_err_t` 返回；任一步失败都停止后续硬件动作。

- [ ] 构建：

  ```powershell
  Set-Location 'D:\project\openepd-47\driver\test_apps\geometry'
  idf.py set-target esp32s3
  idf.py build
  Set-Location 'D:\project\openepd-47\driver\examples\hello_epaper'
  idf.py build
  ```

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add driver LICENSES
  git -C 'D:\project\openepd-47' commit -m "feat(driver): integrate display and power stack"
  ```

## Task 3：接入 GT911 触摸

**Files:**

- Create: `D:\project\openepd-47\driver\components\openepd47\include\openepd47_touch.h`
- Create: `D:\project\openepd-47\driver\components\openepd47\openepd47_touch.c`
- Create: `D:\project\openepd-47\driver\test_apps\touch_transform\`
- Test: `D:\project\openepd-47\driver\test_apps\touch_transform\main\test_touch_transform.c`
- Create: `D:\project\openepd-47\driver\docs\gt911-sources.md`

- [ ] 先写 0–5 点、四方向旋转、裁剪和边界测试，确认坐标转换尚未实现时失败。

- [ ] 使用 `esp_lcd_touch_gt911` 公开 API；板级代码仅负责 RST、INT、I²C 地址选择和坐标转换。

- [ ] 暴露：

  ```c
  typedef struct {
      uint8_t count;
      uint16_t x[5];
      uint16_t y[5];
      uint16_t strength[5];
  } openepd47_touch_points_t;

  esp_err_t openepd47_touch_read(openepd47_touch_points_t *points);
  ```

- [ ] `gt911-sources.md` 链接乐鑫组件页、乐鑫源码和公开数据手册；明确 PDF 只外链，不镜像。

- [ ] 运行：

  ```powershell
  rg -n "FT5446" 'D:\project\openepd-47\driver'
  rg -n "GT911|esp_lcd_touch_gt911" 'D:\project\openepd-47\driver'
  Set-Location 'D:\project\openepd-47\driver\test_apps\touch_transform'
  idf.py build
  Set-Location 'D:\project\openepd-47\driver\examples\hello_epaper'
  idf.py build
  ```

  预期：第一条无命中；后两项成功。

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add driver
  git -C 'D:\project\openepd-47' commit -m "feat(driver): add GT911 touch integration"
  ```

## Task 4：迁移并清理电子书示例

**Files:**

- Create: `D:\project\openepd-47\driver\examples\ebook\`
- Create: `D:\project\openepd-47\driver\examples\ebook\main\wifi_config.example.h`
- Create: `D:\project\openepd-47\driver\examples\ebook\README.md`
- Create: `D:\project\openepd-47\driver\examples\ebook\ORIGIN.md`

- [ ] 用 `materials/import-allowlist.yml` 逐文件复制，禁止 `Copy-Item -Recurse` 针对原示例根目录。

- [ ] 删除缓存、备份、嵌套 Git、构建输出和机器相关 `sdkconfig`；将 Wi‑Fi 改为未跟踪的本地配置。

- [ ] 示例配置只保留显式占位值：

  ```c
  #define OPENEPD_WIFI_SSID "replace-me"
  #define OPENEPD_WIFI_PASSWORD "replace-me"
  ```

- [ ] `ORIGIN.md` 记录资料提供方、原始相对路径、导入日期、文件哈希和清理项。

- [ ] 运行：

  ```powershell
  Set-Location 'D:\project\openepd-47\driver\examples\ebook'
  idf.py set-target esp32s3
  idf.py build
  rg -n "123456789|FT5446|\.cache" 'D:\project\openepd-47\driver\examples\ebook'
  gitleaks detect --source 'D:\project\openepd-47\driver' --no-git
  ```

  预期：构建成功；rg 无命中；gitleaks 无发现。

- [ ] 提交：

  ```powershell
  git -C 'D:\project\openepd-47' add driver/examples/ebook
  git -C 'D:\project\openepd-47' commit -m "feat(driver): migrate sanitized ebook example"
  ```

## 阶段出口

- 两个示例均可在 ESP-IDF 5.5.4 干净构建。
- GT911 依赖、来源和坐标测试正确，不含 FT5446U 内容。
- EPDiy 完整保留 LGPL-3.0 和精确上游提交。
- 驱动目录秘密扫描通过，电子书示例不含缓存或真实凭据。
