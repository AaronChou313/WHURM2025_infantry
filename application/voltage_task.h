/**
 * @file voltage_task.h
 * @brief 24v电源电压ADC任务，获取电压并计算电量百分比。如果电源不直连开发板，请修改VOLTAGE_DROP
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

#ifndef VOLTAGE_TASK_H
#define VOLTAGE_TASK_H

#include "struct_typedef.h"

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 电源采样和计算电源百分比
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void battery_voltage_task(void const *argument);

/**
 * @brief 获取电量
 * @param none
 * @retval 电量，单位为1, 1 = 1%
 */
extern uint16_t get_battery_percentage(void);
float Get_battery_voltage(void);

#endif
