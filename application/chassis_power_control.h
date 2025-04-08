/**
 * @file chassis_power_control.h
 * @brief 底盘功率控制，只控制80w的功率，主要限制电机电流设定值，如果功率限制是40w，就降低JUDGE_TOTAL_CURRENT_LIMIT、POWER_CURRENT_LIMIT和底盘最大速度
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

#ifndef CHASSIS_POWER_CONTROL_H
#define CHASSIS_POWER_CONTROL_H

#include "chassis_task.h"
#include "main.h"

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 限制功率，主要限制电机电流
 * @param[in] chassis_power_control 底盘数据
 * @retval none
 */
extern void chassis_power_control(chassis_move_t *chassis_power_control);

#endif
