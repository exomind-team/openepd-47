# 6 寸墨水屏电子书 Demo 指南

## 硬件

- ESP32-S3 + 6 寸 ED060XC3（逻辑分辨率 758×1024 竖屏）
- 按键：IO0_6 上翻、IO0_7 下翻、IO0_5 确认、IO0_3/4 取消/刷新

## 烧录

1. 安装 ESP-IDF v5.5.x，目标 `esp32s3`
2. 配置 Wi-Fi：编辑 `main/common/wifi_config.h` 中的 `DEMO_WIFI_SSID` / `DEMO_WIFI_PASSWORD`
   - 或在 `idf.py menuconfig` → **E-Book Demo Configuration** 中填写（优先于头文件）
3. 分区表为 **8MB Flash**：`partitions_16M.csv`（factory 3MB + SPIFFS ~4.9MB）
4. 若刚修改过分区，请擦除 Flash 后烧录：
   ```bash
   idf.py fullclean build flash monitor
   ```
   首次挂载 SPIFFS 出现 `mount failed, formatting...` 属正常现象。

## 3 分钟演示脚本

1. **上电** → 主菜单「图书馆」，底部显示传书地址（约 30s 内连上 Wi-Fi）
2. **电脑/手机** 连同一热点，浏览器打开 `http://<设备IP>/`
3. **传书**：选择 UTF-8 编码 `.txt`，上传（保留原文件名）
4. **书架**：主菜单 → 我的书架 → 选中书籍 → 确认打开
5. **阅读**：下键翻页、上键回翻；进度自动保存，带 `[续读]` 标记
6. **相册**：主菜单 → 我的相册；网页上传照片后自动刷新
7. **返回**：各页面按取消键（FRESH）回主菜单

## Web 上传说明

| 类型 | 格式 | 存储路径 |
|------|------|----------|
| 电子书 | UTF-8 `.txt` | `/spiffs/books/<文件名>.txt` |
| 照片 | 任意图片（网页自动裁剪抖动） | `/spiffs/album.raw` |

书架页脚显示 SPIFFS 用量：`存储: 已用/总量 KB`。

## 隐藏产测

上电后 **5 秒内长按 FRESH 键 3 秒**，进入屏幕检测流程（全白/全黑/灰阶/网格），完成后停留在产测模式。

## 演示样章

`demo/books/` 目录提供两本 UTF-8 样章，可通过网页上传或复制到 SPIFFS `books/` 目录：

- `demo_chapter1.txt` — 短篇节选
- `demo_chapter2.txt` — 第二本演示书

相册测试图请用网页上传任意 JPG/PNG，系统会转为 758×1024 竖屏灰阶。

## 常见问题

| 现象 | 处理 |
|------|------|
| SPIFFS 格式化警告 | 分区变更后首次启动会自动格式化，可忽略 |
| 传图提示空间不足 | 删除大体积书籍后重试；或擦除 Flash 重新烧录 |
| 书架为空 | 确认上传的是 `.txt` 且为 UTF-8 编码 |
| Wi-Fi 连不上 | 检查 `wifi_config.h` 热点名称与密码 |
