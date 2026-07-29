#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <stdbool.h>

// 全局按键状态标志（供各 App 检查，用完后需要手动清零）
extern volatile bool g_key_up_pressed;    // IO0_6: 上一页
extern volatile bool g_key_down_pressed;  // IO0_7: 下一页
extern volatile bool g_key_set_pressed;   // IO0_5: 设置/菜单
extern volatile bool g_key_fresh_pressed; // IO0_3 或 IO0_4: 刷新
extern volatile bool g_key_fresh_held;  // 刷新键当前是否按下

void key_manager_init(void);
bool key_manager_fresh_held(void);

#endif // KEY_MANAGER_H