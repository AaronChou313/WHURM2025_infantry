/**
 * @file calibrate_task.h
 * @brief 校准设备，包括云台、陀螺仪、加速度计、磁力计、底盘。
 *        云台校准用于计算中点、最大/最小相对角度；
 *        陀螺仪校准用于计算零点漂移；
 *        加速度和磁力校准尚未实现，因为加速度无需校准，磁力也未使用；
 *        底盘校准用于使3508电机进入快速重置ID模式。
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
 * 使用遥控器开始校准
 * 1. 遥控器的两个switch开关都打到最下方（第三档）
 * 2. 两个摇杆均打向内侧下方（左摇杆向右下打，右摇杆向左下打），保持两秒
 * 3. 两个摇杆均打向外侧下方（左摇杆向左下大，右摇杆向右下打），开始陀螺仪校准
 * 4. 两个摇杆均打向外侧上方（左摇杆向左上打，右摇杆向右上打），开始云台校准
 * 5. 两个摇杆均打向内侧上方（左摇杆向右上打，右摇杆向左上打），开始底盘校准
 *
 * 数据在flash中，包括校验数据、名字name[3]、校验标志位cali_flag
 * 例如head_cali有八个字节，但它在flash中需要12字节
 * 0x080A0000-0x080A0007：head_cali数据
 * 0x080A0008：name[0]
 * 0x080A0009：name[1]
 * 0x080A000A：name[2]
 * 0x080A000B：cali_flag（当cali_flag == 0x55时，表示head_cali已经校准完成）
 *
 * 添加新设备
 * 1. 在calibrate_task.h的cali_id_e中添加数据结构，例如：
 * typedef enum
 * {
 *  ...
 *  CALI_XXX,
 *  CALI_LIST_LENGTH,
 * }
 * 2. 在calibrate_task.h里添加数据结构，长度必须是4字节的倍数，例如：
 * typedef struct
 * {
 *  uint16_t xxx;
 *  uint16_t yyy;
 *  fp32 zzz;
 * } xxx_cali_t;
 * 3. 在FLASH_WRITE_BUF_LENGTH中添加sizeof(xxx_cali_t)，并实现新函数bool_t cali_xxx_hook(uint32_t *cali, bool_t cmd)，并在cali_name[CALI_LIST_LENGTH][3]中添加名称，声明变量xxx_cali_t xxx_cali，在cali_sensor_buf中添加数据长度，最后在cali_hook_fun[CALI_LIST_LENGTH]中添加函数。
 */

#ifndef CALIBRATE_TASK_H
#define CALIBRATE_TASK_H

#include "struct_typedef.h"

/*-----------------------------------宏定义-----------------------------------*/

// 当imu在校准时，蜂鸣器的设置频率和强度
#define imu_start_buzzer() buzzer_on(95, 10000)
// 当云台在校准时，蜂鸣器的设置频率和强度
#define gimbal_start_buzzer() buzzer_on(31, 19999)
// 关闭蜂鸣器
#define cali_buzzer_off() buzzer_off()

// 获取STM32片内的温度，计算imu的温度
#define cali_get_mcu_temperature() get_temprate()

// flash读取函数
#define cali_flash_read(address, buf, len) flash_read((address), (buf), (len))
// flash写入函数
#define cali_flash_write(address, buf, len) flash_write_single_address((address), (buf), (len))
// flash擦除函数
#define cali_flash_erase(address, page_num) flash_erase_address((address), (page_num))

// 获取遥控器指针
#define get_remote_ctrl_point_cali() get_remote_control_point()
// 当imu在校准时，失能遥控器
#define gyro_cali_disable_control() RC_unable()
// 重启遥控器
#define gyro_cali_enable_control() RC_restart(SBUS_RX_BUF_NUM)

// 计算陀螺仪零漂
#define gyro_cali_fun(cali_scale, cali_offset, time_count) INS_cali_gyro((cali_scale), (cali_offset), (time_count))
// 设置在INS task内的陀螺仪零漂
#define gyro_set_cali(cali_scale, cali_offset) INS_set_cali_gyro((cali_scale), (cali_offset))

// 保存的flash地址
#define FLASH_USER_ADDR ADDR_FLASH_SECTOR_9
// 最大陀螺仪控制温度
#define GYRO_CONST_MAX_TEMP 45.0f
// 设置校准
#define CALI_FUNC_CMD_ON 1
// 已经校准过，设置校准值
#define CALI_FUNC_CMD_INIT 0
// 1ms系统延时
#define CALIBRATE_CONTROL_TIME 1
// 校准数据头长度
#define CALI_SENSOR_HEAD_LENGTH 1

// 自己ID
#define SELF_ID 0
// 固件版本
#define FIRMWARE_VERSION 12345
// 校准标志位（表示已校验）
#define CALIED_FLAG 0x55
// 有20s可以用遥控器进行校准
#define CALIBRATE_END_TIME 20000
// 当10s的时候，蜂鸣器切换成高频声音
#define RC_CALI_BUZZER_MIDDLE_TIME 10000
// 当开始校准的时候，蜂鸣器切换成低频声音
#define RC_CALI_BUZZER_START_TIME 0

#define rc_cali_buzzer_middle_on() gimbal_start_buzzer()
#define rc_cali_buzzer_start_on() imu_start_buzzer()
#define RC_CMD_LONG_TIME 2000

#define RCCALI_BUZZER_CYCLE_TIME 400
#define RC_CALI_BUZZER_PAUSE_TIME 200
// 遥控器阈值，遥控器通道的最大值为660
#define RC_CALI_VALUE_HOLE 600

// 陀螺仪校准时间
#define GYRO_CALIBRATE_TIME 20000

/*-----------------------------------数据结构定义-----------------------------------*/

// 校准设备名称
typedef enum
{
  CALI_HEAD = 0,
  CALI_GIMBAL = 1,
  CALI_GYRO = 2,
  CALI_ACC = 3,
  CALI_MAG = 4,
  // add more...
  CALI_LIST_LENGTH,
} cali_id_e;

typedef __packed struct
{
  uint8_t name[3];                                  // 设备名称
  uint8_t cali_done;                                // 校准标志位，0x55表示校准完成
  uint8_t flash_len : 7;                            // 缓冲区长度
  uint8_t cali_cmd : 1;                             // 校准命令位，1表示运行校准钩子函数
  uint32_t *flash_buf;                              // 连接设备校准数据
  bool_t (*cali_hook)(uint32_t *point, bool_t cmd); // 校准函数
} cali_sensor_t;

// 头设备校准数据结构体
typedef __packed struct
{
  uint8_t self_id;           // 自身ID
  uint16_t firmware_version; // 固件版本
  int8_t temperature;        // imu温度
  fp32 latitude;             // latitude
} head_cali_t;

// 云台设备校准数据结构体
typedef struct
{
  uint16_t yaw_offset;
  uint16_t pitch_offset;
  fp32 yaw_max_angle;
  fp32 yaw_min_angle;
  fp32 pitch_max_angle;
  fp32 pitch_min_angle;
} gimbal_cali_t;

// IMU校准数据结构体
typedef struct
{
  fp32 offset[3]; // x,y,z
  fp32 scale[3];  // x,y,z
} imu_cali_t;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 校准参数初始化
 * @param none
 * @retval none
 */
extern void cali_param_init(void);

/**
 * @brief 获取imu控制温度，单位摄氏度
 * @param none
 * @retval imu控制温度
 */
extern int8_t get_control_temperature(void);

/**
 * @brief 获取维度，默认22.0f
 * @param[out] latitude fp32指针
 * @retval none
 */
extern void get_flash_latitude(float *latitude);

/**
 * @brief 校准任务，由main函数创建
 * @param[in] pvParameters null
 * @retval none
 */
extern void calibrate_task(void const *pvParameters);

#endif
