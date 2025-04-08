/**
 * @file led_flow_task.h
 * @brief led RGB灯效
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author RM
 * @date 2018-12-26
 * @remark 官步初始代码
 * ----------------------------------------------------------------------------
 * @version 1.0.0.1
 * @author 周明杨
 * @date 2024-12-30
 * @remark 优化整体架构
 */

/*-----------------------------------预处理-----------------------------------*/

#ifndef LED_TRIGGER_TASK_H
#define LED_TRIGGER_TASK_H

#include "struct_typedef.h"

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief led-RGB任务
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void led_RGB_flow_task(void const *argument);

#endif
