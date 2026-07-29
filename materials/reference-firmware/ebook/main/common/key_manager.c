#include "key_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "KEY_MGR"

volatile bool g_key_up_pressed = false;
volatile bool g_key_down_pressed = false;
volatile bool g_key_set_pressed = false;
volatile bool g_key_fresh_pressed = false;
volatile bool g_key_fresh_held = false;

// XL9555 / PCA9555 I2C 配置
#define TEST_I2C_NUM            I2C_NUM_0
#define PCA9555_ADDR            0x24

// 寄存器地址映射
#define REG_INPUT_0             0x00
#define REG_CONFIG_0            0x06

// 引脚定义 (Port 0)
#define PIN_KEY_FRESH1          3   // IO0_3
#define PIN_KEY_FRESH2          4   // IO0_4
#define PIN_KEY_SET             5   // IO0_5
#define PIN_KEY_UP              6   // IO0_6
#define PIN_KEY_DOWN            7   // IO0_7

// 底层 I2C 读写
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
    
    // 如果 I2C_NUM_0 没有被初始化，这里可能会失败，但 epdiy 通常会初始化 I2C 
    // 以防万一，我们忽略错误，只返回数据
    esp_err_t ret = i2c_master_cmd_begin(TEST_I2C_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        // ESP_LOGE(TAG, "I2C read failed!");
        return 0xFF; // 读失败返回全高电平
    }
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

// 扫描任务
static void key_scan_task(void *pvParameters) {
    // 初始化引脚模式：配置为输入 (写 1 为 Input)
    uint8_t config_val = i2c_read_byte(REG_CONFIG_0);
    config_val |= (1 << PIN_KEY_FRESH1) | (1 << PIN_KEY_FRESH2) |
                  (1 << PIN_KEY_SET)    | (1 << PIN_KEY_UP)     |
                  (1 << PIN_KEY_DOWN);
    i2c_write_byte(REG_CONFIG_0, config_val);

    uint8_t last_state = 0xFF;

    while (1) {
        uint8_t current_state = i2c_read_byte(REG_INPUT_0);

        bool fresh_down = !((current_state >> PIN_KEY_FRESH1) & 1)
                       || !((current_state >> PIN_KEY_FRESH2) & 1);
        g_key_fresh_held = fresh_down;
        
        // 低电平有效 (0 表示按下)
        // 只有在 前一状态是高电平 且 当前是低电平时，才认为是按键按下 (下降沿)
        
        if (((last_state >> PIN_KEY_UP) & 1) && !((current_state >> PIN_KEY_UP) & 1)) {
            g_key_up_pressed = true;
            ESP_LOGI(TAG, "KEY_UP Pressed");
        }
        if (((last_state >> PIN_KEY_DOWN) & 1) && !((current_state >> PIN_KEY_DOWN) & 1)) {
            g_key_down_pressed = true;
            ESP_LOGI(TAG, "KEY_DOWN Pressed");
        }
        if (((last_state >> PIN_KEY_SET) & 1) && !((current_state >> PIN_KEY_SET) & 1)) {
            g_key_set_pressed = true;
            ESP_LOGI(TAG, "KEY_SET Pressed");
        }
        if ( (((last_state >> PIN_KEY_FRESH1) & 1) && !((current_state >> PIN_KEY_FRESH1) & 1)) ||
             (((last_state >> PIN_KEY_FRESH2) & 1) && !((current_state >> PIN_KEY_FRESH2) & 1)) ) {
            g_key_fresh_pressed = true;
            ESP_LOGI(TAG, "KEY_FRESH Pressed");
        }

        last_state = current_state;
        
        // 每 50ms 轮询一次按键（自带简单消抖效果）
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void key_manager_init(void) {
    ESP_LOGI(TAG, "Initializing Key Manager...");
    xTaskCreate(key_scan_task, "key_scan_task", 2048, NULL, 5, NULL);
}

bool key_manager_fresh_held(void)
{
    return g_key_fresh_held;
}