/**
 * @file gimbal_task.h
 * @brief 云台任务
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

#ifndef GIMBAL_TASK_H
#define GIMBAL_TASK_H

#include "struct_typedef.h"
#include "CAN_receive.h"
#include "pid.h"
#include "remote_control.h"

/*-----------------------------------宏定义-----------------------------------*/

// pitch速度环的pid参数，pid最大输出，pid最大积分输出
#define PITCH_SPEED_PID_KP 6000.0f//3000.0f
#define PITCH_SPEED_PID_KI 100.0f//0.0f
#define PITCH_SPEED_PID_KD 0.0f//0.0f
#define PITCH_SPEED_PID_MAX_OUT 30000.0f//60000.0f
#define PITCH_SPEED_PID_MAX_IOUT 1000.0f//10000.0f

// yaw速度环的pid参数，pid最大输出，pid最大积分输出
#define YAW_SPEED_PID_KP 6000.0f//6000.0f
#define YAW_SPEED_PID_KI 20.0f//0.0f
#define YAW_SPEED_PID_KD 0.0f//0.0f
#define YAW_SPEED_PID_MAX_OUT 28000.0f//28000.0f
#define YAW_SPEED_PID_MAX_IOUT 500.0f//500.0f

// pitch角度环（陀螺仪解算角）的pid参数，pid最大输出，pid最大积分输出
#define PITCH_GYRO_ABSOLUTE_PID_KP 10.0f//5.7f
#define PITCH_GYRO_ABSOLUTE_PID_KI 0.05f//0.1f
#define PITCH_GYRO_ABSOLUTE_PID_KD 0.0f//0.7f
#define PITCH_GYRO_ABSOLUTE_PID_MAX_OUT 12.0f//10.0f
#define PITCH_GYRO_ABSOLUTE_PID_MAX_IOUT 3.0f//1.0f

// yaw角度环（陀螺仪解算角）的pid参数，pid最大输出，pid最大积分输出
#define YAW_GYRO_ABSOLUTE_PID_KP 42.0f//20.0f
#define YAW_GYRO_ABSOLUTE_PID_KI 0.3f//0.1f
#define YAW_GYRO_ABSOLUTE_PID_KD 12.0f//11.3f
#define YAW_GYRO_ABSOLUTE_PID_MAX_OUT 8.0f//10.0f
#define YAW_GYRO_ABSOLUTE_PID_MAX_IOUT 4.0f//2.0f

// pitch角度环（编码器解算角）的pid参数，pid最大输出，pid最大积分输出
#define PITCH_ENCODE_RELATIVE_PID_KP 15.0f
#define PITCH_ENCODE_RELATIVE_PID_KI 0.00f
#define PITCH_ENCODE_RELATIVE_PID_KD 0.0f
#define PITCH_ENCODE_RELATIVE_PID_MAX_OUT 10.0f
#define PITCH_ENCODE_RELATIVE_PID_MAX_IOUT 0.0f

// yaw角度环（编码器解算角）的pid参数，pid最大输出，pid最大积分输出
#define YAW_ENCODE_RELATIVE_PID_KP 26.0f
#define YAW_ENCODE_RELATIVE_PID_KI 0.0f
#define YAW_ENCODE_RELATIVE_PID_KD 2.0f
#define YAW_ENCODE_RELATIVE_PID_MAX_OUT 10.0f
#define YAW_ENCODE_RELATIVE_PID_MAX_IOUT 0.0f

// 任务初始化，空闲一段时间
#define GIMBAL_TASK_INIT_TIME 201
// yaw控制通道
#define YAW_CHANNEL 4
// pitch控制通道
#define PITCH_CHANNEL 3
// 云台状态切换通道
#define GIMBAL_MODE_CHANNEL 0

// 掉头180度按键
#define TURN_KEYBOARD KEY_PRESSED_OFFSET_F
// 掉头云台速度
#define TURN_SPEED 0.04f
// 测试按键尚未使用
#define TEST_KEYBOARD KEY_PRESSED_OFFSET_R
// 遥控器输入死区，因为遥控器存在差异，摇杆在中间时，其值不一定为0
#define RC_DEADBAND 15

#define YAW_CAMERA_SEN 0.00001f
#define PITCH_CAMERA_SEN 0.000015f

#define YAW_RC_SEN -0.000005f
#define PITCH_RC_SEN -0.000006f

#define YAW_AUTO_SEN    0.002f
#define PITCH_AUTO_SEN  -0.002f

#define YAW_MOUSE_SEN -0.00010f
#define PITCH_MOUSE_SEN 0.00005f

#define YAW_ENCODE_SEN 0.01f
#define PITCH_ENCODE_SEN 0.01f

#define GIMBAL_CONTROL_TIME 1

// 云台测试模式，宏定义为0表示不使用测试模式
#define GIMBAL_TEST_MODE 0

// pitch电机是否反转（1-是，0-否）
#define PITCH_TURN 1
// yaw电机是否反转（1-是，0-否）
#define YAW_TURN 0

// 电机码盘中值
#define HALF_ECD_RANGE 4096
// 电机码盘最大值
#define ECD_RANGE 8191
// 云台初始化回中值时允许的误差，并且在误差范围内停止一段时间以及最大时间6s后解除初始化状态
#define GIMBAL_INIT_ANGLE_ERROR 0.1f
#define GIMBAL_INIT_STOP_TIME 1000
#define GIMBAL_INIT_TIME 6000
#define GIMBAL_CALI_REDUNDANT_ANGLE 0.1f
// 云台初始化回中值的速度以及控制到的角度
#define GIMBAL_INIT_PITCH_SPEED 0.002f
#define GIMBAL_INIT_YAW_SPEED 0.002f

#define INIT_YAW_SET 0.0f
#define INIT_PITCH_SET 0.1f

// 云台校准中值的时候，发送原始电流值，以及堵转时间，通过陀螺仪判断堵转
#define GIMBAL_CALI_MOTOR_SET 8000
#define GIMBAL_CALI_STEP_TIME 2000
#define GIMBAL_CALI_GYRO_LIMIT 0.1f

#define GIMBAL_CALI_PITCH_MAX_STEP 1
#define GIMBAL_CALI_PITCH_MIN_STEP 2
#define GIMBAL_CALI_YAW_MAX_STEP 3
#define GIMBAL_CALI_YAW_MIN_STEP 4

#define GIMBAL_CALI_START_STEP GIMBAL_CALI_PITCH_MAX_STEP
#define GIMBAL_CALI_END_STEP 5

// 判断遥控器无输入的时间以及遥控器无输入判断，设置云台yaw回中值以防陀螺仪漂移
#define GIMBAL_MOTIONLESS_RC_DEADLINE 10
#define GIMBAL_MOTIONLESS_TIME_MAX 3000

// 电机编码值转换成角度值
#ifndef MOTOR_ECD_TO_RAD
#define MOTOR_ECD_TO_RAD 0.000766990394f //      2*  PI  /8192
#endif

/*-----------------------------------数据结构定义-----------------------------------*/

typedef enum
{
  GIMBAL_MOTOR_RAW = 0, // 电机原始值控制
  GIMBAL_MOTOR_GYRO,    // 电机陀螺仪角度控制
  GIMBAL_MOTOR_ENCONDE, // 电机编码值角度控制
} gimbal_motor_mode_e;

// 云台PID结构体
typedef struct
{
  fp32 kp;
  fp32 ki;
  fp32 kd;

  fp32 set;
  fp32 get;
  fp32 err;

  fp32 max_out;
  fp32 max_iout;

  fp32 Pout;
  fp32 Iout;
  fp32 Dout;

  fp32 out;
} gimbal_PID_t;

// 云台电机结构体
typedef struct
{
  const motor_measure_t *gimbal_motor_measure;
  gimbal_PID_t gimbal_motor_absolute_angle_pid;
  gimbal_PID_t gimbal_motor_relative_angle_pid;
  pid_type_def gimbal_motor_gyro_pid;
  gimbal_motor_mode_e gimbal_motor_mode;
  gimbal_motor_mode_e last_gimbal_motor_mode;
  uint16_t offset_ecd;
  fp32 max_relative_angle; // rad
  fp32 min_relative_angle; // rad

  fp32 relative_angle;     // rad
  fp32 relative_angle_set; // rad
  fp32 absolute_angle;     // rad
  fp32 absolute_angle_set; // rad
  fp32 motor_gyro;         // rad/s
  fp32 motor_gyro_set;
  fp32 motor_speed;
  fp32 raw_cmd_current;
  fp32 current_set;
  int16_t given_current;

} gimbal_motor_t;

// 云台校准结构体
typedef struct
{
  fp32 max_yaw;
  fp32 min_yaw;
  fp32 max_pitch;
  fp32 min_pitch;
  uint16_t max_yaw_ecd;
  uint16_t min_yaw_ecd;
  uint16_t max_pitch_ecd;
  uint16_t min_pitch_ecd;
  uint8_t step;
} gimbal_step_cali_t;

// 云台控制结构体
typedef struct
{
  const RC_ctrl_t *gimbal_rc_ctrl;
  const fp32 *gimbal_INS_angle_point;
  const fp32 *gimbal_INS_gyro_point;
  gimbal_motor_t gimbal_yaw_motor;
  gimbal_motor_t gimbal_pitch_motor;
  gimbal_step_cali_t gimbal_cali;
} gimbal_control_t;

/*-----------------------------------变量声明-----------------------------------*/

extern gimbal_control_t gimbal_control_1;

/*-----------------------------------外部函数声明-----------------------------------*/
/**
 * @brief 返回yaw电机数据指针
 * @param none
 * @retval yaw电机数据指针
 */
extern const gimbal_motor_t *get_yaw_motor_point(void);

/**
 * @brief 返回pitch电机数据指针
 * @param none
 * @retval pitch电机数据指针
 */
extern const gimbal_motor_t *get_pitch_motor_point(void);

/**
 * @brief 云台任务，间隔GIMBAL_CONTROL_TIME ms执行一次
 * @param[in] pvParameters 空
 * @retval none
 */
extern void gimbal_task(void const *pvParameters);

/**
 * @brief 云台校准计算，将校准记录的中值、最大值、最小值返回
 * @param[out] yaw_offse yaw中值指针
 * @param[out] pitch_offset pitch中值指针
 * @param[out] max_yaw yaw最大相对角度指针
 * @param[out] min_yaw yaw最小相对角度指针
 * @param[out] max_pitch pitch最大相对角度指针
 * @param[out] min_pitch pitch最小相对角度指针
 * @retval none
 */
extern bool_t cmd_cali_gimbal_hook(uint16_t *yaw_offset, uint16_t *pitch_offset, fp32 *max_yaw, fp32 *min_yaw, fp32 *max_pitch, fp32 *min_pitch);

/**
 * @brief 云台校准设置，设置校准的云台中值以及最大最小相对角度
 * @param[in] yaw_offse yaw中值
 * @param[in] pitch_offset pitch中值
 * @param[in] max_yaw yaw最大相对角度
 * @param[in] min_yaw yaw最小相对角度
 * @param[in] max_yaw pitch最大相对角度
 * @param[in] min_yaw pitch最小相对角度
 * @retval none
 */
extern void set_cali_gimbal_hook(const uint16_t yaw_offset, const uint16_t pitch_offset, const fp32 max_yaw, const fp32 min_yaw, const fp32 max_pitch, const fp32 min_pitch);

#endif
