/**
 * @file factory_test.h
 * @brief 产线画面检测流程
 */

#pragma once

#include "epd_highlevel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行产线画面检测流程
 * * 流程：
 * 1. 物理清屏 (确保无残影) -> 显示全白
 * 2. 物理清屏 -> 显示全黑
 * 3. 物理清屏 -> 显示浅灰
 * 4. 物理清屏 -> 显示1x1网格
 * 5. 物理清屏 -> 显示棋盘格
 * * @param hl 已初始化的 HighLevel 状态指针
 * @param temperature 当前环境温度
 */
void factory_test_run(EpdiyHighlevelState* hl, int temperature);


#ifdef __cplusplus
}
#endif