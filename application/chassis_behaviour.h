/**
 * @file chassis_behaviour.h
 * @brief 根据遥控器的值，决定底盘行为
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
 * 如果要添加一个新的行为模式：
 * 1. 在chassis_behaviour.h文件中的chassis_behaviour_e枚举类型中添加一个新行为模式名字
 * erum
 * {
 *     ...
 *     ...
 *     CHASSIS_XXX_XXX, // 新添加的
 * }chassis_behaviour_e,
 * 2. 实现一个新的函数chassis_xxx_xxx_control(fp32 *vx, fp32 *vy, fp32 *wz, chassis_move_t * chassis)
 * vx, vy, wz参数是底盘运动控制输入量
 * vx控制纵向移动，正值为前进，负值为后退
 * vy控制横向移动，正值为左移，负值为右移
 * wz是角度控制或者旋转速度控制
 * 3. 在chassis_behaviour_mode_set函数中，添加新的逻辑判断，给chassis_behaviour_mode赋值成CHASSIS_XXX_XXX
 * 在函数的最后，添加else if(chassis_behaviour_mode == CHASSIS_XXX_XXX)
 * 目前有四种底盘行为模式：
 * CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW：vx和vy是速度控制，wz是角度控制（即云台和底盘的相对角度，可以把wz命名为xxx_angle_set）
 * CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW：vx和vy是速度控制，wz是角度控制（即底盘陀螺仪计算出的绝对角度，可以把wz命名为xxx_angle_set）
 * CHASSIS_VECTOR_NO_FOLLOW_YAW：vx和vy是速度控制，wz是旋转速度控制
 * CHASSIS_VECTOR_RAW：使用vx、vy、wz直接线性计算出车轮的电流值，电流值将直接发送到can总线上
 * 4. 在chassis_behaviour_control_set函数的最后，添加
 * else if(chassis_behaviour_mode == CHASSIS_XXX_XXX)
 * {
 *  chassis_xxx_xxx_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
 * }
 */

/*-----------------------------------预处理-----------------------------------*/

#ifndef CHASSIS_BEHAVIOUR_H
#define CHASSIS_BEHAVIOUR_H

#include "struct_typedef.h"
#include "chassis_task.h"

/*-----------------------------------数据结构定义-----------------------------------*/

typedef enum
{
  CHASSIS_ZERO_FORCE,                  // 底盘无力，跟没上电那样
  CHASSIS_NO_MOVE,                     // 底盘保持不动
  CHASSIS_INFANTRY_FOLLOW_GIMBAL_YAW,  // 正常步兵底盘跟随云台
  CHASSIS_ENGINEER_FOLLOW_CHASSIS_YAW, // 工程底盘角度控制底盘，由于底盘没有陀螺仪，故而角度是减去云台角度得到的，如果有底盘陀螺仪请在chassis_feedback_update函数中更新底盘的yaw，pitch，roll角度
  CHASSIS_NO_FOLLOW_YAW,               // 底盘不跟随角度，角度是开环的，但轮子是有速度环
  CHASSIS_OPEN,                        // 遥控器的值乘以比例成电流值，直接发送到can总线上
  CHASSIS_MY_DEFINED                   // 新增的自定义模式
} chassis_behaviour_e;

// 在chassis_open模型下，遥控器乘以该比例发送到can上
#define CHASSIS_OPEN_RC_SCALE 10

#define MAX_ROTATE_SPEED 8.0f

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 通过逻辑判断，赋值chassis_behaviour_mode成哪种模式
 */
extern void chassis_behaviour_mode_set(chassis_move_t *chassis_move_mode);

/**
 * @brief 设置控制量，根据不同底盘控制模式，三个参数会控制不同运动，在这个函数里面，会调用不同的控制函数
 * @param vx_set 纵向速度控制量
 * @param vy_set 横向速度控制量
 * @param angle_set 旋转角速度控制量
 */
extern void chassis_behaviour_control_set(fp32 *vx_set, fp32 *vy_set, fp32 *angle_set, chassis_move_t *chassis_move_rc_to_vector, fp32 *gimbal_yaw_set, fp32 *gimbal_pitch_set);

#endif
