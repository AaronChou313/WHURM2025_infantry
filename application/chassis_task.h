/**
 * @file chassis_task.h
 * @brief 底盘任务的头文件
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

#ifndef CHASSIS_TASK_H
#define CHASSIS_TASK_H

#include "struct_typedef.h"
#include "CAN_receive.h"
#include "gimbal_task.h"
#include "pid.h"
#include "remote_control.h"
#include "user_lib.h"

/*-----------------------------------宏定义-----------------------------------*/

// 任务开始空闲一段时间
#define CHASSIS_TASK_INIT_TIME 357
// 底盘前后运动的遥控器通道号码
#define CHASSIS_X_CHANNEL 1
// 底盘左右运动的遥控器通道号码
#define CHASSIS_Y_CHANNEL 0
// 底盘旋转的遥控器通道号码
#define CHASSIS_WZ_CHANNEL 2
// 云台yaw轴转动的遥控器通道号码
#define CHASSIS_YAW_CHANNEL 4
// 云台pitch轴转动的遥控器通道号码
#define CHASSIS_PITCH_CHANNEL 3
// 选择底盘状态的开关通道号码
#define CHASSIS_MODE_CHANNEL 0
// 遥控器前进摇杆（max 660）转化成车体前进速度（m/s）的比例
#define CHASSIS_VX_RC_SEN 0.006f
// 遥控器左右摇杆（max 660）转化成车体左右速度（m/s）的比例
#define CHASSIS_VY_RC_SEN 0.005f
// 跟随底盘yaw模式下，遥控器的yaw摇杆（max 660）增加到车体角度的比例
#define CHASSIS_ANGLE_Z_RC_SEN 0.000002f
// 不跟随云台的时候，遥控器的yaw摇杆（max 660）转化为车体旋转速度的比例
#define CHASSIS_WZ_RC_SEN -0.01f

#define CHASSIS_WZ_KEY_SEN 0.04f
#define CHASSIS_VX_KEY_SEN 0.006f
#define CHASSIS_VY_KEY_SEN 0.005f

// 底盘前后运动加速度
#define CHASSIS_ACCEL_X_NUM 0.1666666667f
// 底盘左右运动加速度
#define CHASSIS_ACCEL_Y_NUM 0.3333333333f
// 摇杆死区
#define CHASSIS_RC_DEADLINE 10

#define MOTOR_SPEED_TO_CHASSIS_SPEED_VX 0.25f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_VY 0.25f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_WZ 0.25f

#define MOTOR_DISTANCE_TO_CENTER 0.2f

// 底盘任务控制间隔 2ms
#define CHASSIS_CONTROL_TIME_MS 2
// 底盘任务控制间隔 0.002s
#define CHASSIS_CONTROL_TIME 0.002f
// 底盘任务控制频率，尚未使用这个宏
#define CHASSIS_CONTROL_FREQUENCE 500.0f
// 底盘3508最大can发送电流值
#define MAX_MOTOR_CAN_CURRENT 16000.0f
// 底盘摇摆按键
#define SWING_KEY KEY_PRESSED_OFFSET_CTRL
// 底盘前后左右控制按键
#define CHASSIS_FRONT_KEY KEY_PRESSED_OFFSET_W
#define CHASSIS_BACK_KEY KEY_PRESSED_OFFSET_S
#define CHASSIS_LEFT_KEY KEY_PRESSED_OFFSET_A
#define CHASSIS_RIGHT_KEY KEY_PRESSED_OFFSET_D
#define CHASSIS_SPRINT_KEY KEY_PRESSED_OFFSET_SHIFT
// M3508转化成底盘速度（m/s）的比例
#define M3508_MOTOR_RPM_TO_VECTOR 0.000415809748903494517209f
#define CHASSIS_MOTOR_RPM_TO_VECTOR_SEN M3508_MOTOR_RPM_TO_VECTOR
// 单个底盘电机最大速度
#define MAX_WHEEL_SPEED 4.0f
// 底盘运动过程最大前进速度
#define NORMAL_MAX_CHASSIS_SPEED_X 2.0f
// 底盘运动过程最大平移速度
#define NORMAL_MAX_CHASSIS_SPEED_Y 1.5f

#define CHASSIS_WZ_SET_SCALE 0.05f

// 摇摆原地不动摇摆最大角度（rad）
#define SWING_NO_MOVE_ANGLE 0.7f
// 摇摆过程底盘运动最大角度（rad）
#define SWING_MOVE_ANGLE 0.31415926535897932384626433832795f

// 底盘电机速度环PID
#define M3505_MOTOR_SPEED_PID_KP 15000.0f
#define M3505_MOTOR_SPEED_PID_KI 10.0f
#define M3505_MOTOR_SPEED_PID_KD 0.0f
#define M3505_MOTOR_SPEED_PID_MAX_OUT MAX_MOTOR_CAN_CURRENT
#define M3505_MOTOR_SPEED_PID_MAX_IOUT 2000.0f

// 底盘旋转跟随PID
#define CHASSIS_FOLLOW_GIMBAL_PID_KP 40.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_KI 0.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_KD 0.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT 6.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT 0.2f

/*-----------------------------------数据结构定义-----------------------------------*/

// 底盘类型枚举
typedef enum
{
  CHASSIS_WHEEL_MECANUM,        // 麦克纳姆轮
  CHASSIS_WHEEL_OMNIDIRECTIONAL // 全向轮
} chassis_wheel_type_e;

typedef enum
{
  CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW,  // 底盘会跟随云台相对角度
  CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW, // 底盘有底盘角度控制闭环
  CHASSIS_VECTOR_NO_FOLLOW_YAW,      // 底盘有旋转速度控制
  CHASSIS_VECTOR_RAW,                // 直接向CAN发送电流控制
  CHASSIS_VECTOR_DEFINED             // 底盘云台独立运动
} chassis_mode_e;

typedef struct
{
  const motor_measure_t *chassis_motor_measure;
  fp32 accel;
  fp32 speed;
  fp32 speed_set;
  int16_t give_current;
} chassis_motor_t;

typedef struct
{
  const RC_ctrl_t *chassis_RC;               // 底盘使用的遥控器指针
  const gimbal_motor_t *chassis_yaw_motor;   // 底盘使用到yaw云台电机的相对角度来计算底盘的欧拉角
  const gimbal_motor_t *chassis_pitch_motor; // 底盘使用到pitch云台电机的相对角度来计算底盘的欧拉角
  const fp32 *chassis_INS_angle;             // 获取陀螺仪解算出的欧拉角指针
  chassis_mode_e chassis_mode;               // 底盘控制状态机
  chassis_mode_e last_chassis_mode;          // 底盘上次的控制状态机
  chassis_motor_t motor_chassis[4];          // 底盘电机数据
  pid_type_def motor_speed_pid[4];           // 底盘电机速度pid
  pid_type_def chassis_angle_pid;            // 底盘跟随角度pid

  first_order_filter_type_t chassis_cmd_slow_set_vx; // 使用一阶低通滤波减缓设定值
  first_order_filter_type_t chassis_cmd_slow_set_vy; // 使用一阶低通滤波减缓设定值

  fp32 vx;                         // 底盘速度 前进方向 前为正，单位 m/s
  fp32 vy;                         // 底盘速度 左右方向 左为正，单位 m/s
  fp32 wz;                         // 底盘旋转角速度，逆时针为正 单位 rad/s
  fp32 vx_set;                     // 底盘设定速度 前进方向 前为正，单位 m/s
  fp32 vy_set;                     // 底盘设定速度 左右方向 左为正，单位 m/s
  fp32 wz_set;                     // 底盘设定旋转角速度 逆时针为正 单位 rad/s
  fp32 chassis_relative_angle;     // 底盘与云台的相对角度，单位 rad
  fp32 chassis_relative_angle_set; // 设置相对云台控制角度
  fp32 chassis_yaw_set;

  fp32 vx_max_speed;  // 前进方向最大速度，单位 m/s
  fp32 vx_min_speed;  // 后退方向最大速度，单位 m/s
  fp32 vy_max_speed;  // 左方向最大速度，单位 m/s
  fp32 vy_min_speed;  // 右方向最大速度，单位 m/s
  fp32 chassis_yaw;   // 陀螺仪和云台电机叠加的yaw角度
  fp32 chassis_pitch; // 陀螺仪和云台电机叠加的pitch角度
  fp32 chassis_roll;  // 陀螺仪和云台电机叠加的roll角度

} chassis_move_t;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 底盘任务，间隔 CHASSIS_CONTROL_TIME_MS 2ms
 * @param[in] pvParameters 空
 * @retval none
 */
extern void chassis_task(void const *pvParameters);

/**
 * @brief 根据遥控器通道值，计算纵向和横移速度
 * @param[out] vx_set 纵向速度指针
 * @param[out] vy_set 横移速度指针
 * @param[out] chassis_move_rc_to_vector chassis_move变量指针
 * @retval none
 */
extern void chassis_rc_to_control_vector(fp32 *vx_set, fp32 *vy_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 获取底盘电机电流
 * @param[in] none
 * @retval 电流值
 */
uint16_t get_chassis_current(void);

#endif
