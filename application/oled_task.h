/**
 * @file oled_task.h
 * @brief oled屏幕显示错误码
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

#ifndef OLED_TASK_H
#define OLED_TASK_H

#include "struct_typedef.h"

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief oled任务
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void oled_task(void const *argument);

#endif
