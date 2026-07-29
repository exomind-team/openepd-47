---
title: 驱动 API
description: OpenEPD 4.7 正式板级 API 的计划边界。
---

正式 API 尚未实施。计划保持接口窄小：

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

触摸读取计划支持最多五点，并在板级层完成方向转换和边界裁剪。API 实现前，本页只作为设计约束，不表示已有可链接符号。
