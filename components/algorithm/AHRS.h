/**
 * @file AHRS.h
 * @brief 姿态解算
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

#ifndef AHRS_H
#define AHRS_H

#include "AHRS_MiddleWare.h"

/**
  * @brief          根据加速度和磁力计的数据进行四元数初始化
  * @param[in]      quat 需要初始化的四元数
  * @param[in]      accel 加速度计数据（x，y，z），不为空，单位m/s2
  * @param[in]      mag 磁力计数据（x，y，z），不为空，单位uT
  * @retval         空
  */
extern void AHRS_init(fp32 quat[4], const fp32 accel[3], const fp32 mag[3]);

/**
  * @brief          根据陀螺仪、加速度计和磁力计的数据进行四元数更新
  * @param[in]      quat 需要更新的四元数
  * @param[in]      timing_time 更新定时时间，固定定时调用（例如传入0.001f表示1000Hz）
  * @param[in]      gyro 用于更新的陀螺仪数据，数组顺序（x，y，z），单位rad
  * @param[in]      accel 用于初始化的加速度计数据，数组顺序（x，y，z），单位m/s2
  * @param[in]      mag 用于初始化的磁力计数据，数组顺序（x，y，z），单位uT
  * @retval         1-更新成功，0-更新失败
  */
extern bool_t AHRS_update(fp32 quat[4], const fp32 timing_time, const fp32 gyro[3], const fp32 accel[3], const fp32 mag[3]);

/**
  * @brief          根据四元数大小计算对应的欧拉角偏航yaw
  * @param[in]      quat 四元数数组，不为NULL
  * @retval         返回的偏航角yaw，单位rad
  */
extern fp32 get_yaw(const fp32 quat[4]);

/**
  * @brief          根据四元数大小计算对应的欧拉角俯仰pitch
  * @param[in]      quat 四元数数组，不为NULL
  * @retval         返回的俯仰角pitch，单位rad
  */
extern fp32 get_pitch(const fp32 quat[4]);

/**
  * @brief          根据四元数大小计算对应的欧拉角横滚roll
  * @param[in]      quat 四元数数组，不为NULL
  * @retval         返回的横滚角roll，单位rad
  */
extern fp32 get_roll(const fp32 quat[4]);

/**
  * @brief          根据四元数大小计算对应的欧拉角yaw、pitch、roll
  * @param[in]      quat 四元数数组，不为NULL
  * @param[in]      yaw yaw角指针，用于返回yaw角
  * @param[in]      pitch pitch角指针，用于返回pitch角
  * @param[in]      roll roll角指针，用于返回roll角
  * @retval         空
  */
extern void get_angle(const fp32 quat[4], fp32 *yaw, fp32 *pitch, fp32 *roll);

/**
  * @brief          返回当前的重力加速度
  * @param[in]      空
  * @retval         返回重力加速度，单位m/s2
  */
extern fp32 get_carrier_gravity(void);

#endif
