/**
 * @file factory_test.c
 * @brief 产线画面检测流程 (移植自 Arduino XL9555 驱动逻辑)
 */

#include "factor_test.h"
#include "epdiy.h"
#include <string.h> 
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h>
#include <esp_log.h>

// ============================================================================
// ★ 用户配置区
// ============================================================================

#define TEST_I2C_NUM            I2C_NUM_0      // 使用的 I2C 端口
#define PCA9555_ADDR            0x24           // I2C 地址

// 引脚定义 (0-7 为 Port0, 8-15 为 Port1)
#define PIN_KEY_FRESH           4   // P0.4 -> Pin 4
#define PIN_BORDER_VPOS         8   // P1.0 -> Pin 8
#define PIN_BORDER_VNEG         9   // P1.1 -> Pin 9

// 模式定义
#define XL_INPUT                1
#define XL_OUTPUT               0
#define XL_HIGH                 1
#define XL_LOW                  0

// 寄存器地址映射 (参考你的 Arduino 代码)
// Port 0
#define REG_INPUT_0             0x00
#define REG_OUTPUT_0            0x02
#define REG_CONFIG_0            0x06
// Port 1
#define REG_INPUT_1             0x01
#define REG_OUTPUT_1            0x03
#define REG_CONFIG_1            0x07

// ============================================================================
// 1. I2C 底层读写 (保持不变)
// ============================================================================

static uint8_t i2c_read_byte(uint8_t reg) {
    uint8_t data = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCA9555_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCA9555_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(TEST_I2C_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return data;
}

static void i2c_write_byte(uint8_t reg, uint8_t val) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCA9555_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(TEST_I2C_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
}

// ============================================================================
// 2. 复刻 Arduino XL9555 驱动函数
// ============================================================================

/**
 * @brief 配置引脚模式 (对应 Arduino: XL9555_configurePin)
 * @param pin 0-15
 * @param mode XL_INPUT(1) 或 XL_OUTPUT(0)
 */
static void xl9555_pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 15) return;

    // 决定是 Config Port 0 还是 Port 1
    uint8_t config_reg = (pin < 8) ? REG_CONFIG_0 : REG_CONFIG_1;
    uint8_t pin_bit = pin % 8;

    // 读-改-写
    uint8_t config_val = i2c_read_byte(config_reg);
    
    if (mode == XL_INPUT) {
        config_val |= (1 << pin_bit);  // 设为 1 (Input)
    } else {
        config_val &= ~(1 << pin_bit); // 设为 0 (Output)
    }
    
    i2c_write_byte(config_reg, config_val);
}

/**
 * @brief 写引脚电平 (对应 Arduino: XL9555_writePin)
 * @param pin 0-15
 * @param level XL_HIGH(1) 或 XL_LOW(0)
 */
static void xl9555_digitalWrite(uint8_t pin, uint8_t level) {
    if (pin > 15) return;

    // 决定是 Output Port 0 还是 Port 1
    uint8_t output_reg = (pin < 8) ? REG_OUTPUT_0 : REG_OUTPUT_1;
    uint8_t pin_bit = pin % 8;

    // 读-改-写 (这样就不会覆盖掉库设置的电源位了)
    uint8_t output_val = i2c_read_byte(output_reg);

    if (level == XL_HIGH) {
        output_val |= (1 << pin_bit);
    } else {
        output_val &= ~(1 << pin_bit);
    }

    i2c_write_byte(output_reg, output_val);
}

/**
 * @brief 读引脚电平 (对应 Arduino: XL9555_readPin)
 * @param pin 0-15
 * @return 1 或 0
 */
static uint8_t xl9555_digitalRead(uint8_t pin) {
    if (pin > 15) return 0;

    // 决定是 Input Port 0 还是 Port 1
    uint8_t input_reg = (pin < 8) ? REG_INPUT_0 : REG_INPUT_1;
    uint8_t pin_bit = pin % 8;

    uint8_t port_val = i2c_read_byte(input_reg);
    
    return (port_val >> pin_bit) & 0x01;
}

// ============================================================================
// 3. 业务逻辑 (移植你的 Border 和按键逻辑)
// ============================================================================

// 按键等待
static void ft_wait_key(void) {
    // 确保按键脚是输入模式 (通常 epd_init 已经做了，但多做一次无害)
    xl9555_pinMode(PIN_KEY_FRESH, XL_INPUT);

    // 卡住等待低电平
    while (xl9555_digitalRead(PIN_KEY_FRESH) == XL_HIGH) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Border + 波形刷白 (替代 epd_clear)
// 逻辑：Border黑+屏幕黑 -> Border白+屏幕白
static void ft_custom_border_clear(EpdiyHighlevelState* hl, int temperature) {
    EpdRect full_screen = epd_full_screen();
    uint8_t* fb = epd_hl_get_framebuffer(hl);

    // 1. 确保 Border 引脚是输出模式
    xl9555_pinMode(PIN_BORDER_VPOS, XL_OUTPUT);
    xl9555_pinMode(PIN_BORDER_VNEG, XL_OUTPUT);

    // ==========================================
    // 阶段 1: 全局刷黑 (Border + Screen)
    // ==========================================
    
    //  准备黑色数据
    // epd_fill_rect(full_screen, 0x00, fb);
    // epd_hl_update_screen(hl, MODE_GC16, temperature);
    for (int i = 0; i < 10; i++) {
        epd_push_pixels(full_screen, 100, 0); // 0 = Darken (黑)
        // 稍微喂狗或延时，防止 watchdog 叫，同时让电压稳定
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
    //  打开 Border VPOS (驱动边缘变白)
    xl9555_digitalWrite(PIN_BORDER_VNEG, XL_HIGH);
    //  保持 Border 驱动一会儿 (配合屏幕刷新时间)
    vTaskDelay(pdMS_TO_TICKS(500));
    //  关闭 Border VPOS
    xl9555_digitalWrite(PIN_BORDER_VNEG, XL_LOW);


    // ==========================================
    // 阶段 2: 全局刷白 (Border + Screen)
    // ==========================================

    // A. 准备白色数据
    // epd_fill_rect(full_screen, 0xF0, fb);
    // epd_hl_update_screen(hl, MODE_GC16, temperature);
    for (int i = 0; i < 10; i++) {
        epd_push_pixels(full_screen, 100, 1); // 1 = Lighten (白)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    // B. 打开 Border VNEG (驱动边缘变黑)
    xl9555_digitalWrite(PIN_BORDER_VPOS, XL_HIGH);
    // D. 保持 Border 驱动一会儿
    vTaskDelay(pdMS_TO_TICKS(500));
    // E. 关闭 Border VNEG
    xl9555_digitalWrite(PIN_BORDER_VPOS, XL_LOW);

    
}

// ============================================================================
// 4. 绘图函数 (保持不变)
// ============================================================================

static void ft_draw_black(EpdiyHighlevelState* hl) {
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    epd_fill_rect(epd_full_screen(), 0x00, fb);
}
static void ft_draw_white(EpdiyHighlevelState* hl) {
    epd_hl_set_all_white(hl);
}
static void ft_draw_light_gray(EpdiyHighlevelState* hl) {
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    epd_fill_rect(epd_full_screen(), 0xc0, fb);
}
static void ft_draw_1x1_grid(EpdiyHighlevelState* hl) {
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    int w = epd_width(); int h = epd_height();
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            epd_draw_pixel(x, y, ((x+y)%2)?0x00:0xF0, fb);
}

/**
 * @brief 在指定区域绘制 2x2 网格 (逐像素方式)
 * @param area 要绘制的区域，通常是 epd_full_screen()
 */
static void ft_draw_2x2_grid(EpdiyHighlevelState* hl) {
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    int w = epd_width(); int h = epd_height();
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
           epd_draw_pixel(x, y, (((x+1)/2 + y/2)%2) ? 0x00 : 0xF0, fb);
}


static void ft_draw_checkerboard_32px(EpdiyHighlevelState* hl) {
    uint8_t* fb = epd_hl_get_framebuffer(hl);
    int w = epd_width(); int h = epd_height();
    const int BOX_SIZE = 80;
    for (int y = 0; y < h; y += BOX_SIZE) {
        for (int x = 0; x < w; x += BOX_SIZE) {
            uint8_t color = (((x/BOX_SIZE)+(y/BOX_SIZE))%2)==0 ? 0xF0 : 0x00;
            EpdRect rect = {.x=x, .y=y, .width=BOX_SIZE, .height=BOX_SIZE};
            if(rect.x+rect.width>w) rect.width=w-rect.x;
            if(rect.y+rect.height>h) rect.height=h-rect.y;
            epd_fill_rect(rect, color, fb);
        }
    }
}

// ============================================================================
// 5. 核心调度流程
// ============================================================================

static void show_pattern_safe(EpdiyHighlevelState* hl, 
                              void (*draw_func)(EpdiyHighlevelState*), 
                              int temperature,
                              bool use_border_clear) 
{
    epd_poweron();
    // 1. 清屏 (分情况)
    if (use_border_clear) {
        // 最后一张图：Border 刷黑
        ft_custom_border_clear(hl, temperature);
    // } else {
    //     // 普通图：标准清屏
    //     epd_clear();
      }

    // 2. 状态同步
    int fb_size = epd_width() / 2 * epd_height();
    epd_hl_set_all_white(hl);               
    memset(hl->back_fb, 0xFF, fb_size);     

    // 3. 绘制
    if (draw_func) {
        draw_func(hl);
    }

    // 4. 显示 (White -> Target)
    epd_hl_update_screen(hl, MODE_GC16, temperature);
    epd_poweroff();
    // 5. 等待按键
    //ft_wait_key();
    vTaskDelay(pdMS_TO_TICKS(2500));
    
    // 消抖
    vTaskDelay(pdMS_TO_TICKS(500)); 
}

void factory_test_run(EpdiyHighlevelState* hl, int temperature) {
    //epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));
    while(1)
    {
        show_pattern_safe(hl, ft_draw_checkerboard_32px, temperature, false);

        show_pattern_safe(hl, ft_draw_1x1_grid, temperature, true);

        show_pattern_safe(hl, ft_draw_2x2_grid, temperature, true);

        show_pattern_safe(hl, ft_draw_black, temperature, false);

        show_pattern_safe(hl, ft_draw_light_gray, temperature, true);

        show_pattern_safe(hl, ft_draw_white, temperature, true);
    }
      while (1)
    {     
    }
}
