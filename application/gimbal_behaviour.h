/**
 * @file gimbal_behaviour.h
 * @brief 云台控制任务
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
 * ----------------------------------------------------------------------------
 * 如果要添加一个新的行为模式
 */

/*-----------------------------------预处理-----------------------------------*/

#ifndef GIMBAL_BEHAVIOUR_H
#define GIMBAL_BEHAVIOUR_H

#include "struct_typedef.h"
#include "gimbal_task.h"

/*-----------------------------------数据结构定义-----------------------------------*/

typedef enum
{
  GIMBAL_ZERO_FORCE = 0, // 无力模式
  GIMBAL_INIT,           // 初始化模式
  GIMBAL_CALI,           // 校准模式
  GIMBAL_ABSOLUTE_ANGLE, // 绝对角度控制模式
  GIMBAL_RELATIVE_ANGLE, // 相对角度控制模式
  GIMBAL_MOTIONLESS,     // 空闲模式
  GIMBAL_AUTOAIM,        // 自瞄模式
} gimbal_behaviour_e;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 云台行为状态机和电机状态机设置，在gimbal_task.c中被gimbal_set_mode函数调用
 * @param[in] gimbal_mode_set 云台数据指针
 * @retval none
 */
extern void gimbal_behaviour_mode_set(gimbal_control_t *gimbal_mode_set);

/**
 * @brief 云台行为控制，根据不同行为采用不同控制函数
 * @param[out] add_yaw 设置的yaw角度增加值，单位rad
 * @param[out] add_pitch 设置的pitch角度增加值，单位rad
 * @param[in] gimbal_mode_set 云台数据指针
 * @retval none
 */
extern void gimbal_behaviour_control_set(fp32 *add_yaw, fp32 *add_pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 云台在某些行为下，需要底盘不动
 * @param[in] none
 * @retval 1-不动，0-正常
 */
extern bool_t gimbal_cmd_to_chassis_stop(void);

/**
 * @brief 云台在某些行为下，需要停止射击
 * @param[in] none
 * @retval 1-停止射击，0-正常
 */
extern bool_t gimbal_cmd_to_shoot_stop(void);

#endif
