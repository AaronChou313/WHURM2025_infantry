/**
 * @file INS_task.h
 * @brief 使用陀螺仪bmi088计算欧拉角，不使用磁力计ist8310，因此只启用data ready引脚以节省CPU时间
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

#ifndef INS_Task_H
#define INS_Task_H

#include "struct_typedef.h"

/*-----------------------------------宏定义-----------------------------------*/

#define SPI_DMA_GYRO_LENGTH 8
#define SPI_DMA_ACCEL_LENGTH 9
#define SPI_DMA_ACCEL_TEMP_LENGTH 4

#define IMU_DR_SHFITS 0
#define IMU_SPI_SHFITS 1
#define IMU_UPDATE_SHFITS 2
#define IMU_NOTIFY_SHFITS 3

#define BMI088_GYRO_RX_BUF_DATA_OFFSET 1
#define BMI088_ACCEL_RX_BUF_DATA_OFFSET 2

// ist83100原始数据在缓冲区buf的位置
#define IST8310_RX_BUF_DATA_OFFSET 16
// 温度控制PID的kp
#define TEMPERATURE_PID_KP 1600.0f
// 温度控制PID的ki
#define TEMPERATURE_PID_KI 0.2f
// 温度控制PID的kd
#define TEMPERATURE_PID_KD 0.0f
// 温度控制PID的max_out
#define TEMPERATURE_PID_MAX_OUT 4500.0f
// 温度控制PID的max_iout
#define TEMPERATURE_PID_MAX_IOUT 4400.0f
// mpu6500控制温度的设置TIM的重载值，即给PWM最大为 MPU6500_TEMP_PWM_MAX - 1
#define MPU6500_TEMP_PWM_MAX 5000
// 任务开始初期等待一段时间
#define INS_TASK_INIT_TIME 7

#define INS_YAW_ADDRESS_OFFSET 0
#define INS_PITCH_ADDRESS_OFFSET 1
#define INS_ROLL_ADDRESS_OFFSET 2

#define INS_GYRO_X_ADDRESS_OFFSET 0
#define INS_GYRO_Y_ADDRESS_OFFSET 1
#define INS_GYRO_Z_ADDRESS_OFFSET 2

#define INS_ACCEL_X_ADDRESS_OFFSET 0
#define INS_ACCEL_Y_ADDRESS_OFFSET 1
#define INS_ACCEL_Z_ADDRESS_OFFSET 2

#define INS_MAG_X_ADDRESS_OFFSET 0
#define INS_MAG_Y_ADDRESS_OFFSET 1
#define INS_MAG_Z_ADDRESS_OFFSET 2

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief imu任务，初始化bmi088，ist8310，计算欧拉角
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void INS_task(void const *pvParameters);

/**
 * @brief 校准陀螺仪
 * @param[out] cali_scale 陀螺仪的比例因子，1.0f为默认值，不修改
 * @param[out] cali_offset 陀螺仪的零漂，采集陀螺仪的静止的输出作为offset
 * @param[out] time_count 陀螺仪的时刻，每次在gyro_offset调用会加1
 * @retval none
 */
extern void INS_cali_gyro(fp32 cali_scale[3], fp32 cali_offset[3], uint16_t *time_count);

/**
 * @brief 校准陀螺仪设置，将从flash或者其他地方传入校准值
 * @param[in] cali_scale 陀螺仪的比例因子，1.0f为默认值，不修改
 * @param[in] cali_offset 陀螺仪的零漂
 * @retval none
 */
extern void INS_set_cali_gyro(fp32 cali_scale[3], fp32 cali_offset[3]);

/**
 * @brief 获取四元数
 * @param none
 * @retval INS_quat指针
 */
extern const fp32 *get_INS_quat_point(void);

/**
 * @brief 获取欧拉角（0:yaw, 1:pitch, 2:roll)，单位rad
 * @param none
 * @retval INS_angle指针
 */
extern const fp32 *get_INS_angle_point(void);

/**
 * @brief 获取陀螺仪数据角速度（0:x轴，1:y轴，2:roll轴），单位rad/s
 * @param none
 * @retval INS_gyro指针
 */
extern const fp32 *get_gyro_data_point(void);

/**
 * @brief 获取陀螺仪数据加速度（0:x轴，1:y轴，2:roll轴），单位m/s2
 * @param none
 * @retval INS_gyro指针
 */
extern const fp32 *get_accel_data_point(void);

/**
 * @brief 获取磁力计数据加速度（0:x轴，1:y轴，2:roll轴），单位ut
 * @param none
 * @retval INS_mag指针
 */
extern const fp32 *get_mag_data_point(void);

#endif
