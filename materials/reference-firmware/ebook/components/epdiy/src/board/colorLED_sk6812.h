#pragma once

#include <stdint.h>

// ---------- 底层：LED 颜色状态 ----------
// 这里只管理“灯是什么颜色”，不关心系统状态。

typedef enum {
    LED_COLOR_OFF = 0,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_YELLOW,
    LED_COLOR_CYAN,
    LED_COLOR_MAGENTA,
    LED_COLOR_WHITE,
    LED_COLOR_CUSTOM,   // 通过 set_rgb 设置的自定义颜色
} led_color_t;

// 初始化 LED 驱动：只做一次
//  - set_reg / clear_reg : 对应 GPIO 置位/清零寄存器地址
//  - gpio_mask           : 该 GPIO 对应的 bit 掩码
//  - cpu_freq_mhz        : 当前 CPU 频率 (一般 240)
//  - global_brightness   : 全局亮度 (255=全亮)
void colorLED_init(volatile uint32_t *set_reg,
                   volatile uint32_t *clear_reg,
                   uint32_t gpio_mask,
                   uint32_t cpu_freq_mhz,
                   uint8_t global_brightness);

// 设置为某个预定义颜色（内部有状态机，重复设置同一颜色不会重新发送）
void colorLED_set_color(led_color_t color);

// 设置为任意 RGB（0~255），会把当前颜色标记为 LED_COLOR_CUSTOM
void colorLED_set_rgb(uint8_t r, uint8_t g, uint8_t b);