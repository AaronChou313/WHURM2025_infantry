/**
 * @file servo_task.h
 * @brief
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

#ifndef SERVO_TASK_H
#define SERVO_TASK_H

#include "struct_typedef.h"

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 舵机任务
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void servo_task(void const *argument);

#endif
