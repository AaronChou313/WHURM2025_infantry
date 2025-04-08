/**
 * @file CAN_receive.h
 * @brief CAN中断接收函数用于接收电机数据，CAN发送函数用于向电机发送控制电流
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

#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include "struct_typedef.h"

/*-----------------------------------宏定义-----------------------------------*/

// CAN1用于控制底盘（2pin）
#define CHASSIS_CAN hcan1
// CAN2用于控制云台（4pin）
#define GIMBAL_CAN hcan2

/*-----------------------------------数据结构定义-----------------------------------*/

// CAN线收发id枚举列表
typedef enum
{
  CAN_CHASSIS_ALL_ID = 0x200, // 底盘总id
  CAN_3508_M1_ID = 0x201,     // 一号底盘电机id
  CAN_3508_M2_ID = 0x202,     // 二号底盘电机id
  CAN_3508_M3_ID = 0x203,     // 三号底盘电机id
  CAN_3508_M4_ID = 0x204,     // 四号底盘电机id

  CAN_YAW_MOTOR_ID = 0x205,     // 云台yaw电机id
  CAN_PIT_MOTOR_ID = 0x206,     // 云台pitch电机id
  CAN_TRIGGER_MOTOR_ID = 0x207, // 拨弹电机id
  CAN_GIMBAL_ALL_ID = 0x1FF,    // 云台总id
} can_msg_id_e;

// RM电机数据结构体
typedef struct
{
  uint16_t ecd;          // 电机编码器值
  int16_t speed_rpm;     // 电机转速
  int16_t given_current; // 给定电流
  uint8_t temperate;     // 电机温度
  int16_t last_ecd;      // 上次编码器值
} motor_measure_t;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 向云台的电机发送控制电流
 * @param[in] yaw id为0x205的云台yaw电机（6020电机）的控制电流，范围[-30000,30000]
 * @param[in] pitch id为0x206的云台pitch电机（6020电机）的控制电流，范围[-30000,30000]
 * @param[in] shoot id为0x207的拨弹电机（2006电机）的控制电流，范围[-10000,10000]
 * @param[in] rev id为0x208的电机的控制电流，暂未使用，为预留位
 * @retval none
 */
extern void CAN_cmd_gimbal(int16_t yaw, int16_t pitch, int16_t shoot, int16_t rev);

/**
 * @brief 发送ID为0x700的CAN包，它会设置3508电机进入快速设置ID模式
 * @param none
 * @retval none
 */
extern void CAN_cmd_chassis_reset_ID(void);

/**
 * @brief 向底盘的电机发送控制电流
 * @param[in] motor1 id为0x201的一号底盘电机的控制电流，范围[-16384,16384]
 * @param[in] motor2 id为0x202的二号底盘电机的控制电流，范围[-16384,16384]
 * @param[in] motor3 id为0x203的三号底盘电机的控制电流，范围[-16384,16384]
 * @param[in] motor4 id为0x204的四号底盘电机的控制电流，范围[-16384,16384]
 * @retval none
 */
extern void CAN_cmd_chassis(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4);

/**
 * @brief 返回yaw电机（6020电机）的数据指针
 * @param none
 * @retval yaw电机数据指针
 */
extern const motor_measure_t *get_yaw_gimbal_motor_measure_point(void);

/**
 * @brief 返回pitch电机（6020电机）的数据指针
 * @param none
 * @retval pitch电机数据指针
 */
extern const motor_measure_t *get_pitch_gimbal_motor_measure_point(void);

/**
 * @brief 返回拨弹电机（2006电机）的数据指针
 * @param none
 * @retval 拨弹电机数据指针
 */
extern const motor_measure_t *get_trigger_motor_measure_point(void);

/**
 * @brief 返回底盘电机（3508电机）的数据指针
 * @param[in] i 电机序号，范围[0,3]
 * @retval 底盘电机数据指针
 */
extern const motor_measure_t *get_chassis_motor_measure_point(uint8_t i);

#endif
