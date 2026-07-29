// colorLED_sk6812.c —— SK6812MINI-012 底层颜色状态机 + bit-bang 驱动

#include "colorLED_sk6812.h"

#include <stdbool.h>
#include "freertos/portmacro.h"
#include <xtensa/core-macros.h>
#include "esp_rom_sys.h"

// ====== SK6812 时序参数（单位 ns） ======
// 参考 SK6812MINI-012 datasheet 实测值：
//   0 码：T0H≈0.3us, T0L≈0.9us
//   1 码：T1H≈0.7us, T1L≈0.6us
#define LED_T0H_NS      300
#define LED_T0L_NS      900
#define LED_T1H_NS      700
#define LED_T1L_NS      600
#define LED_TRESET_US   100   // Reset 低电平时间 >80us，这里给 200us 裕量

// ========== 全局配置 & 状态 ==========

static volatile uint32_t *s_set_reg   = NULL;
static volatile uint32_t *s_clear_reg = NULL;
static uint32_t s_gpio_mask           = 0;
static uint32_t s_cpu_freq_mhz        = 240;
static uint8_t  s_global_brightness   = 32;

static led_color_t s_current_color = LED_COLOR_OFF;

// ns -> cycles
static inline uint32_t led_ns_to_cycles(uint32_t ns, uint32_t cpu_freq_mhz)
{
    uint64_t cycles = (uint64_t)cpu_freq_mhz * (uint64_t)ns;
    cycles = (cycles + 999) / 1000;   // 四舍五入
    if (cycles < 20) cycles = 20;     // 避免太小，留出指令开销
    return (uint32_t)cycles;
}

// 简单忙等 cycles
static inline void IRAM_ATTR led_delay_cycles(uint32_t cycles)
{
    uint32_t start = XTHAL_GET_CCOUNT();
    while ((uint32_t)(XTHAL_GET_CCOUNT() - start) < cycles) {
        // busy wait
    }
}

// 低层：发送一颗 GRB 数据（24bit），不做状态机
static void IRAM_ATTR sk6812_send_grb_internal(uint8_t g, uint8_t r, uint8_t b)
{
    uint32_t grb = ((uint32_t)g << 16) |
                   ((uint32_t)r << 8)  |
                   ((uint32_t)b);

    uint32_t t0h = led_ns_to_cycles(LED_T0H_NS, s_cpu_freq_mhz);
    uint32_t t0l = led_ns_to_cycles(LED_T0L_NS, s_cpu_freq_mhz);
    uint32_t t1h = led_ns_to_cycles(LED_T1H_NS, s_cpu_freq_mhz);
    uint32_t t1l = led_ns_to_cycles(LED_T1L_NS, s_cpu_freq_mhz);

    // 整帧 24bit 期间关中断，保证时序连续
    UBaseType_t int_mask = portSET_INTERRUPT_MASK_FROM_ISR();

    for (int i = 23; i >= 0; --i) {
        bool bit = (grb >> i) & 0x01;

        if (bit) {
            // 1 码：高电平长，低电平短
            *s_set_reg = s_gpio_mask;
            led_delay_cycles(t1h);
            *s_clear_reg = s_gpio_mask;
            led_delay_cycles(t1l);
        } else {
            // 0 码：高电平短，低电平长
            *s_set_reg = s_gpio_mask;
            led_delay_cycles(t0h);
            *s_clear_reg = s_gpio_mask;
            led_delay_cycles(t0l);
        }
    }

    // 恢复中断
    portCLEAR_INTERRUPT_MASK_FROM_ISR(int_mask);

    // 复位：保持低电平一段时间
    esp_rom_delay_us(LED_TRESET_US);
}

// 内部：按 RGB 发送一帧（带亮度限幅）
static void colorLED_send_rgb_internal(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_set_reg || !s_clear_reg || s_gpio_mask == 0) {
        return; // 尚未初始化
    }

    uint8_t r_scaled = (uint8_t)((r * (uint32_t)s_global_brightness) / 255);
    uint8_t g_scaled = (uint8_t)((g * (uint32_t)s_global_brightness) / 255);
    uint8_t b_scaled = (uint8_t)((b * (uint32_t)s_global_brightness) / 255);

    // SK6812 要 GRB 顺序
    sk6812_send_grb_internal(g_scaled, r_scaled, b_scaled);
}

// ========== 对外接口：初始化 & 颜色状态机 ==========

void colorLED_init(volatile uint32_t *set_reg,
                   volatile uint32_t *clear_reg,
                   uint32_t gpio_mask,
                   uint32_t cpu_freq_mhz,
                   uint8_t global_brightness)
{
    s_set_reg         = set_reg;
    s_clear_reg       = clear_reg;
    s_gpio_mask       = gpio_mask;
    s_cpu_freq_mhz    = cpu_freq_mhz;
    s_global_brightness = global_brightness ? global_brightness : 1;

    // 初始化时默认关灯
    colorLED_set_color(LED_COLOR_OFF);
}

void colorLED_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    colorLED_send_rgb_internal(r, g, b);
    s_current_color = LED_COLOR_CUSTOM;
}

void colorLED_set_color(led_color_t color)
{
    if (color == s_current_color) {
        return; // 颜色没变就不重复发送
    }
    s_current_color = color;

    uint8_t r = 0, g = 0, b = 0;

    switch (color) {
    case LED_COLOR_OFF:
        r = g = b = 0;
        break;
    case LED_COLOR_RED:
        r = 20; g = 0; b = 0;
        break;
    case LED_COLOR_GREEN:
        r = 0; g = 20; b = 0;
        break;
    case LED_COLOR_BLUE:
        r = 0; g = 0; b = 20;
        break;
    case LED_COLOR_YELLOW:
        r = 20; g = 20; b = 0;
        break;
    case LED_COLOR_CYAN:
        r = 0; g = 20; b = 20;
        break;
    case LED_COLOR_MAGENTA:
        r = 20; g = 0; b = 20;
        break;
    case LED_COLOR_WHITE:
        r = 20; g = 20; b = 20;
        break;
    case LED_COLOR_CUSTOM:
    default:
        // CUSTOM 不在这里处理，由 colorLED_set_rgb 直接发
        return;
    }

    colorLED_send_rgb_internal(r, g, b);
}
