#include <stdint.h>
#include "epd_board.h"
#include "epdiy.h"

#include "../output_common/render_method.h"
#include "../output_lcd/lcd_driver.h"
#include "esp_log.h"
#include "pca9555.h"
#include "tps65185.h"
#include "colorLED_sk6812.h"  // 251122: LED 
#include "soc/gpio_struct.h"  // 251122: LED

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <sdkconfig.h>

// Make this compile von the ESP32 without ifdefing the whole file
#ifndef CONFIG_IDF_TARGET_ESP32S3
#define GPIO_NUM_40 -1
#define GPIO_NUM_41 -1
#define GPIO_NUM_42 -1
#define GPIO_NUM_43 -1
#define GPIO_NUM_44 -1
#define GPIO_NUM_45 -1
#define GPIO_NUM_46 -1
#define GPIO_NUM_47 -1
#define GPIO_NUM_48 -1
#endif

#define CFG_SCL GPIO_NUM_40
#define CFG_SDA GPIO_NUM_39
#define CFG_INTR GPIO_NUM_41
#define EPDIY_I2C_PORT I2C_NUM_0

#define CFG_PIN_PWRGOOD (PCA_PIN_PC17 >> 8)
#define CFG_PIN_INT (PCA_PIN_PC16 >> 8)
#define CFG_PIN_PWRUP (PCA_PIN_PC14 >> 8)
#define CFG_PIN_WAKEUP (PCA_PIN_PC13 >> 8)
#define CFG_PIN_VCOM_CTRL (PCA_PIN_PC15 >> 8)

#define CFG_PIN_22V_EN (PCA_PIN_P00 & 0xFF)   // P00 -> bit0
#define CFG_PIN_25V_EN (PCA_PIN_P01 & 0xFF)   // P01 -> bit1

#define D15 GPIO_NUM_19
#define D14 GPIO_NUM_18
#define D13 GPIO_NUM_17
#define D12 GPIO_NUM_16
#define D11 GPIO_NUM_15
#define D10 GPIO_NUM_14
#define D9 GPIO_NUM_13
#define D8 GPIO_NUM_12

#define D7 GPIO_NUM_11
#define D6 GPIO_NUM_10
#define D5 GPIO_NUM_9
#define D4 GPIO_NUM_8
#define D3 GPIO_NUM_7
#define D2 GPIO_NUM_6
#define D1 GPIO_NUM_5
#define D0 GPIO_NUM_4

/* Control Lines */
#define CKV GPIO_NUM_48
#define STH GPIO_NUM_46
#define LEH GPIO_NUM_3
#define STV GPIO_NUM_47

// === EPD OE / MODE 直接连 ESP32S3 ===
#define EPD_OE   GPIO_NUM_20   // EP_OE -> IO20
#define EPD_MODE GPIO_NUM_45   // EP_MODE -> IO45

// color LED drive IO pin GPIO42
#define BOARD_LED_GPIO_NUM   42

/* Edges */
#define CKH GPIO_NUM_21

typedef struct {
    i2c_port_t port;
    bool pwrup;
    bool vcom_ctrl;
    bool wakeup;
    bool others[8];
} epd_config_register_t;

/** The VCOM voltage to use. */
static int vcom = 1600;

static epd_config_register_t config_reg;

// 对应 out1 组寄存器 (GPIO32~47)
#if BOARD_LED_GPIO_NUM < 32
    #define BOARD_LED_SET_REG    (&GPIO.out_w1ts)
    #define BOARD_LED_CLEAR_REG  (&GPIO.out_w1tc)
    #define BOARD_LED_MASK       (1U << BOARD_LED_GPIO_NUM)
#else
    #define BOARD_LED_SET_REG    (&GPIO.out1_w1ts.val)
    #define BOARD_LED_CLEAR_REG  (&GPIO.out1_w1tc.val)
    #define BOARD_LED_MASK       (1U << (BOARD_LED_GPIO_NUM - 32))
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S3)
    #define BOARD_CPU_FREQ_MHZ CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ
#elif defined(CONFIG_IDF_TARGET_ESP32)
    #define BOARD_CPU_FREQ_MHZ CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ
#else
    #define BOARD_CPU_FREQ_MHZ 240
#endif

#define BOARD_LED_BRIGHTNESS  32   // 全局亮度，后面觉得太亮可以再调小

static bool interrupt_done = false;

static void IRAM_ATTR interrupt_handler(void* arg) {
    interrupt_done = true;
}

static lcd_bus_config_t lcd_config = {
    .clock = CKH,
    .ckv = CKV,
    .leh = LEH,
    .start_pulse = STH,
    .stv = STV,
    .data[0] = D0,
    .data[1] = D1,
    .data[2] = D2,
    .data[3] = D3,
    .data[4] = D4,
    .data[5] = D5,
    .data[6] = D6,
    .data[7] = D7,
    .data[8] = D8,
    .data[9] = D9,
    .data[10] = D10,
    .data[11] = D11,
    .data[12] = D12,
    .data[13] = D13,
    .data[14] = D14,
    .data[15] = D15,
};

static void epd_board_init(uint32_t epd_row_width) {
    gpio_hold_dis(CKH);  // free CKH after wakeup

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = CFG_SDA;
    conf.scl_io_num = CFG_SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    conf.clk_flags = 0;
    ESP_ERROR_CHECK(i2c_param_config(EPDIY_I2C_PORT, &conf));

    ESP_ERROR_CHECK(i2c_driver_install(EPDIY_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    config_reg.port = EPDIY_I2C_PORT;
    config_reg.pwrup = false;
    config_reg.vcom_ctrl = false;
    config_reg.wakeup = false;
    for (int i = 0; i < 8; i++) {
        config_reg.others[i] = false;
    }

    // =====================================================
    // ★ 新增：PCA9555 Port0 初始化
    // P00: 22V_EN  输出 高
    // P01: 25V_EN  输出 低
    // P02: 28V_EN  输出 低
    // P03~P07: 按键输入 + 内部上拉
    // =====================================================

    uint8_t cfg0 = 0xF8;  // 1111 1000: P0.0~P0.2 输出(0)，P0.3~P0.7 输入(1)
    ESP_ERROR_CHECK(pca9555_set_config(config_reg.port, cfg0, 0));  // high_port=0 → Port0

    uint8_t val0 = 0xF9;  // 1111 1001: 
                          // P00=1(22V_EN高), P01=0(25V_EN低), P02=0(28V_EN低),
                          // P03~P07=1(输入+上拉)
    ESP_ERROR_CHECK(pca9555_set_value(config_reg.port, val0, 0));   // 设置 P0 输出/上拉

    ESP_LOGI("epdiy", "PCA9555 P0 init: 22V_EN=H, 25V_EN=L, 28V_EN=L, P3~P7 input with pull-up");


    // ★ 新增：EP_OE / EP_MODE 改为直接由 ESP32S3 控制
    gpio_reset_pin(EPD_OE);
    gpio_set_direction(EPD_OE, GPIO_MODE_OUTPUT);
    gpio_set_level(EPD_OE, 0);     // 上电默认关 OE（安全）

    gpio_reset_pin(EPD_MODE);
    gpio_set_direction(EPD_MODE, GPIO_MODE_OUTPUT);
    gpio_set_level(EPD_MODE, 0);   // 默认 0，真正生效会在 poweron 时设置
    
    gpio_set_direction(CFG_INTR, GPIO_MODE_INPUT);
    gpio_set_intr_type(CFG_INTR, GPIO_INTR_NEGEDGE);

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));

    ESP_ERROR_CHECK(gpio_isr_handler_add(CFG_INTR, interrupt_handler, (void*)CFG_INTR));

    // set all epdiy lines to output except TPS interrupt + PWR good
    ESP_ERROR_CHECK(pca9555_set_config(config_reg.port, CFG_PIN_PWRGOOD | CFG_PIN_INT, 1));

    const EpdDisplay_t* display = epd_get_display();

    LcdEpdConfig_t config = {
        .pixel_clock = display->bus_speed * 1000 * 1000,
        .ckv_high_time = 60,
        .line_front_porch = 4,
        .le_high_time = 4,
        .bus_width = display->bus_width,
        .bus = lcd_config,
    };
    epd_lcd_init(&config, display->width, display->height);

    // ---- 板载 LED GPIO 初始化（GPIO42） ----
    gpio_config_t led_io = {
        .pin_bit_mask = 1ULL << BOARD_LED_GPIO_NUM,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_io);

    // 初始拉低
    GPIO.out1_w1tc.val = BOARD_LED_MASK;

    // 初始化 SK6812 颜色驱动
    colorLED_init(BOARD_LED_SET_REG,
                  BOARD_LED_CLEAR_REG,
                  BOARD_LED_MASK,
                  BOARD_CPU_FREQ_MHZ,
                  BOARD_LED_BRIGHTNESS);

    // 系统刚初始化时：认为 EPD 处于“关闭”状态 → 红灯
    colorLED_set_color(LED_COLOR_RED);
}

static void epd_board_deinit() {
    // 关屏时把 OE / MODE 拉到安全电平
    gpio_set_level(EPD_OE, 0);
    gpio_set_level(EPD_MODE, 0);
    gpio_set_direction(EPD_OE, GPIO_MODE_INPUT);
    gpio_set_direction(EPD_MODE, GPIO_MODE_INPUT);


    epd_lcd_deinit();

    ESP_ERROR_CHECK(pca9555_set_config(
        config_reg.port, CFG_PIN_PWRGOOD | CFG_PIN_INT | CFG_PIN_VCOM_CTRL | CFG_PIN_PWRUP, 1
    ));

    int tries = 0;
    while (!((pca9555_read_input(config_reg.port, 1) & 0xC0) == 0x80)) {
        if (tries >= 50) {
            ESP_LOGE("epdiy", "failed to shut down TPS65185!");
            break;
        }
        tries++;
        vTaskDelay(1);
    }

    // Not sure why we need this delay, but the TPS65185 seems to generate an interrupt after some
    // time that needs to be cleared.
    vTaskDelay(50);
    pca9555_read_input(config_reg.port, 0);
    pca9555_read_input(config_reg.port, 1);
    i2c_driver_delete(EPDIY_I2C_PORT);

    gpio_uninstall_isr_service();
}

static void epd_board_set_ctrl(epd_ctrl_state_t* state, const epd_ctrl_state_t* const mask) {
    // ★ 1. 先处理直接连到 ESP32S3 的 OE / MODE
    if (mask->ep_output_enable) {
        gpio_set_level(EPD_OE, state->ep_output_enable ? 1 : 0);
    }

    if (mask->ep_mode) {
        gpio_set_level(EPD_MODE, state->ep_mode ? 1 : 0);
    }

    // ★ 2. 再处理仍然在 PCA9555 上的 TPS 控制脚（PWRUP / VCOM_CTRL / WAKEUP）
    if (mask->ep_output_enable || mask->ep_mode || mask->ep_stv) {
        uint8_t value = 0x00;

        if (config_reg.pwrup)
            value |= CFG_PIN_PWRUP;
        if (config_reg.vcom_ctrl)
            value |= CFG_PIN_VCOM_CTRL;
        if (config_reg.wakeup)
            value |= CFG_PIN_WAKEUP;

        ESP_ERROR_CHECK(pca9555_set_value(config_reg.port, value, 1));
    }
}

static void epd_board_poweron(epd_ctrl_state_t* state) {
    // 1. 初始化 Mask
    epd_ctrl_state_t mask = {
        .ep_output_enable = true,
        .ep_mode = true,
        .ep_stv = true,
    };

    // ★ 关键 1：确保输出关闭，减轻负载
    state->ep_output_enable = false; 
    state->ep_stv = true;
    state->ep_mode = false;

    // 2. 只有 WAKEUP 先拉高 (唤醒芯片，但还没开启升压)
    config_reg.wakeup = true;
    config_reg.pwrup = false;   // 先不开启 Power Up
    config_reg.vcom_ctrl = false; 
    epd_board_set_ctrl(state, &mask);
    
    vTaskDelay(pdMS_TO_TICKS(10)); // 给它一点唤醒时间

    // ========================================================
    // ★ 关键 2：重试循环机制 (最多试 3 次)
    // ========================================================
    int retry_count = 0;
    bool success = false;

    while (retry_count < 3 && !success) {
        if (retry_count > 0) {
            ESP_LOGW("epdiy", "TPS startup failed, retrying... (%d/3)", retry_count + 1);
            // 失败后先拉低一下 PWRUP 复位状态
            config_reg.pwrup = false;
            epd_board_set_ctrl(state, &mask);
            vTaskDelay(pdMS_TO_TICKS(100)); // 歇 100ms 让电源电压回升
        }

        // --- 尝试启动 ---
        config_reg.pwrup = true; // 拉高 PWRUP，开始升压序列
        config_reg.vcom_ctrl = true;
        epd_board_set_ctrl(state, &mask);

        // 等待 Power Good (带超时)
        int wait_pwr_good = 0;
        bool pg_ok = false;
        
        // 循环检测 PWRGOOD 引脚
        while (wait_pwr_good < 200) { // 200ms 超时
            if (pca9555_read_input(config_reg.port, 1) & CFG_PIN_PWRGOOD) {
                pg_ok = true;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
            wait_pwr_good++;
        }

        if (pg_ok) {
            success = true; // 硬件信号好了
        } else {
            retry_count++; // 没好，准备下一次重试
        }
    }

    // ========================================================
    // 3. 结果判断
    // ========================================================
    if (!success) {
        ESP_LOGE("epdiy", "TPS PowerOn Failed after 3 retries!");
        // 读取错误码看看
        uint8_t status = tps_read_register(config_reg.port, 0x01);
        ESP_LOGE("epdiy", "Final Status Reg: 0x%02X", status);
        return; // 放弃治疗
    }

    // 4. 配置 TPS 寄存器
    ESP_ERROR_CHECK(tps_write_register(config_reg.port, TPS_REG_ENABLE, 0x3F));
    tps_set_vcom(config_reg.port, vcom);

    state->ep_sth = true;
    epd_ctrl_state_t sth_mask = { .ep_sth = true };
    epd_board_set_ctrl(state, &sth_mask);

    // 5. 等待 PG (Power Good) 寄存器标志
    int tries = 0;
    while (!((tps_read_register(config_reg.port, TPS_REG_PG) & 0xFA) == 0xFA)) {
        if (tries >= 200) {
             ESP_LOGE("epdiy", "TPS Register PG Timeout!");
             return;
        }
        tries++;
        vTaskDelay(1);
    }

    // ★ 关键 3：一切就绪后，终于开启屏幕输出
    state->ep_output_enable = true; 
    epd_ctrl_state_t out_mask = { .ep_output_enable = true };
    epd_board_set_ctrl(state, &out_mask);

    // 成功
    colorLED_set_color(LED_COLOR_GREEN);
    ESP_LOGI("epdiy", "TPS Startup Success!");
}

static void epd_board_measure_vcom(epd_ctrl_state_t* state) {
    epd_ctrl_state_t mask = {
        .ep_output_enable = true,
        .ep_mode = true,
        .ep_stv = true,
    };
    state->ep_stv = true;
    state->ep_mode = false;
    state->ep_output_enable = true;
    config_reg.wakeup = true;
    epd_board_set_ctrl(state, &mask);
    config_reg.pwrup = true;
    epd_board_set_ctrl(state, &mask);

    // give the IC time to powerup and set lines
    vTaskDelay(1);
    state->ep_sth = true;
    mask = (const epd_ctrl_state_t){
        .ep_sth = true,
    };
    epd_board_set_ctrl(state, &mask);

    while (!(pca9555_read_input(config_reg.port, 1) & CFG_PIN_PWRGOOD)) {
    }
    ESP_LOGI("epdiy", "Power rails enabled");

    state->ep_sth = true;
    mask = (const epd_ctrl_state_t){
        .ep_sth = true,
    };
    epd_board_set_ctrl(state, &mask);

    int tries = 0;
    while (!((tps_read_register(config_reg.port, TPS_REG_PG) & 0xFA) == 0xFA)) {
        if (tries >= 500) {
            ESP_LOGE(
                "epdiy",
                "Power enable failed! PG status: %X",
                tps_read_register(config_reg.port, TPS_REG_PG)
            );
            return;
        }
        tries++;
        vTaskDelay(1);
    }
}

static void epd_board_poweroff(epd_ctrl_state_t* state) {
    epd_ctrl_state_t mask = {
        .ep_stv = true,
        .ep_output_enable = true,
        .ep_mode = true,
    };
    config_reg.vcom_ctrl = false;
    config_reg.pwrup = false;
    state->ep_stv = false;
    state->ep_output_enable = false;
    state->ep_mode = false;
    epd_board_set_ctrl(state, &mask);
    vTaskDelay(1);
    config_reg.wakeup = false;
    epd_board_set_ctrl(state, &mask);

    // EPD 关闭：红灯
    //colorLED_set_color(LED_COLOR_RED);
}

static float epd_board_ambient_temperature() {
    return 20;
}

static void set_vcom(int value) {
    vcom = value;
}

const EpdBoardDefinition epd_board_v7 = {
    .init = epd_board_init,
    .deinit = epd_board_deinit,
    .set_ctrl = epd_board_set_ctrl,
    .poweron = epd_board_poweron,
    .poweroff = epd_board_poweroff,

    .measure_vcom = epd_board_measure_vcom,
    .get_temperature = epd_board_ambient_temperature,
    .set_vcom = set_vcom,

    // unimplemented for now, but shares v6 implementation
    .gpio_set_direction = NULL,
    .gpio_read = NULL,
    .gpio_write = NULL,
};
