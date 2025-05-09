/**
 * @file detect_task.h
 * @brief 检测错误任务，通过接收数据时间来判断，提供检测钩子函数、错误存在函数
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
 * @attention
 * 如果要添加一个新设备
 * 1. 第一步在detect_task.h，添加设备名字在errorList的最后，例如
 * enum errorList
 * {
 *  ...
 *  XXX_TOE,  // 新设备
 *  ERROR_LIST_LENGTH,
 * };
 * 2. 在detect_init函数，添加offlineTime，onlineTime，priority参数，例如
 * uint16_t set_item[ERROR_LIST_LENGTH][3] =
 * {
 *  ...
 *  {n,n,n},  // XX_TOE
 * };
 * 3. 如果有data_is_error_fun，solve_data_error_fun函数，赋值到函数指针
 * 4. 在XXX_TOE设备数据来的时候，添加函数detect_hook(XXX_TOE)
 */

/*-----------------------------------预处理-----------------------------------*/

#ifndef DETECT_TASK_H
#define DETECT_TASK_H

#include "struct_typedef.h"

/*-----------------------------------宏定义-----------------------------------*/

#define DETECT_TASK_INIT_TIME 57
#define DETECT_CONTROL_TIME 10

/*-----------------------------------数据结构定义-----------------------------------*/

// 错误码以及对应设备顺序
enum errorList
{
  DBUS_TOE = 0,
  CHASSIS_MOTOR1_TOE,
  CHASSIS_MOTOR2_TOE,
  CHASSIS_MOTOR3_TOE,
  CHASSIS_MOTOR4_TOE,
  YAW_GIMBAL_MOTOR_TOE,
  PITCH_GIMBAL_MOTOR_TOE,
  TRIGGER_MOTOR_TOE,
  BOARD_GYRO_TOE,
  BOARD_ACCEL_TOE,
  BOARD_MAG_TOE,
  REFEREE_TOE,
  RM_IMU_TOE,
  OLED_TOE,
  ERROR_LIST_LENGTH,
};

typedef __packed struct
{
  uint32_t new_time;
  uint32_t last_time;
  uint32_t lost_time;
  uint32_t work_time;
  uint16_t set_offline_time : 12;
  uint16_t set_online_time : 12;
  uint8_t enable : 1;
  uint8_t priority : 4;
  uint8_t error_exist : 1;
  uint8_t is_lost : 1;
  uint8_t data_is_error : 1;

  fp32 frequency;
  bool_t (*data_is_error_fun)(void);
  void (*solve_lost_fun)(void);
  void (*solve_data_error_fun)(void);
} error_t;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 检测任务
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void detect_task(void const *pvParameters);

/**
 * @brief 获取设备对应的错误状态
 * @param[in] toe 设备目录
 * @retval true-错误，false-无错误
 */
extern bool_t toe_is_error(uint8_t err);

/**
 * @brief 记录时间
 * @param[in] toe 设备目录
 * @retval none
 */
extern void detect_hook(uint8_t toe);

/**
 * @brief 获取错误列表
 * @param none
 * @retval error_list的指针
 */
extern const error_t *get_error_list_point(void);

#endif
