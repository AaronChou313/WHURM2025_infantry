/**
 * @file calibrate_task.c
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
 */

/*-----------------------------------头文件引用-----------------------------------*/

#include "calibrate_task.h"
#include "string.h"
#include "cmsis_os.h"
#include "bsp_adc.h"
#include "bsp_buzzer.h"
#include "bsp_flash.h"
#include "can_receive.h"
#include "remote_control.h"
#include "INS_task.h"
#include "gimbal_task.h"

/*-----------------------------------宏定义-----------------------------------*/

// 包含head、云台、陀螺仪、加速度计、磁力计。其中陀螺仪、加速度和磁力共用一个数据结构。总共5个（CALI_LIST_LENGTH）设备，需要数据长度 + 5*4字节（name[3]+cali）
#define FLASH_WRITE_BUF_LENGTH (sizeof(head_cali_t) + sizeof(gimbal_cali_t) + sizeof(imu_cali_t) * 3 + CALI_LIST_LENGTH * 4)

#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t calibrate_task_stack;
#endif

/*-----------------------------------变量声明-----------------------------------*/

static const RC_ctrl_t *calibrate_RC; // remote control point
static head_cali_t head_cali;         // head cali data
static gimbal_cali_t gimbal_cali;     // gimbal cali data
static imu_cali_t accel_cali;         // accel cali data
static imu_cali_t gyro_cali;          // gyro cali data
static imu_cali_t mag_cali;           // mag cali data

static uint8_t flash_write_buf[FLASH_WRITE_BUF_LENGTH];

cali_sensor_t cali_sensor[CALI_LIST_LENGTH];

static const uint8_t cali_name[CALI_LIST_LENGTH][3] = {"HD", "GM", "GYR", "ACC", "MAG"};

// 校准数据地址
static uint32_t *cali_sensor_buf[CALI_LIST_LENGTH] = {
    (uint32_t *)&head_cali, (uint32_t *)&gimbal_cali,
    (uint32_t *)&gyro_cali, (uint32_t *)&accel_cali,
    (uint32_t *)&mag_cali};

static uint8_t cali_sensor_size[CALI_LIST_LENGTH] = {
    sizeof(head_cali_t) / 4, sizeof(gimbal_cali_t) / 4,
    sizeof(imu_cali_t) / 4, sizeof(imu_cali_t) / 4, sizeof(imu_cali_t) / 4};

static uint32_t calibrate_systemTick;

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 使用遥控器开始校准，如陀螺仪校准，云台校准，底盘校准等
 * @param none
 * @retval none
 */
static void RC_cmd_to_calibrate(void);

/**
 * @brief 从flash读取校准数据
 * @param none
 * @retval none
 */
static void cali_data_read(void);

/**
 * @brief 向flash写入校准数据
 * @param none
 * @retval none
 */
static void cali_data_write(void);

/**
 * @brief head设备校准
 * @param cali 指向head数据的指针（当cmd为CALI_FUNC_CMD_INIT时，参数是输入；当cmd为CALI_FUNC_CMD_ON时，参数是输出）
 * @param[in] cmd 校准指令（CALI_FUNC_CMD_INIT：表示使用校准数据初始化原始数据；CALI_FUNC_CMD_ON：表示需要校准）
 * @retval 0-校准还未完成，1-校准已完成
 */
static bool_t cali_head_hook(uint32_t *cali, bool_t cmd); // header device cali function

/**
 * @brief 陀螺仪设备校准
 * @param cali 指向head数据的指针（当cmd为CALI_FUNC_CMD_INIT时，参数是输入；当cmd为CALI_FUNC_CMD_ON时，参数是输出）
 * @param[in] cmd 校准指令（CALI_FUNC_CMD_INIT：表示使用校准数据初始化原始数据；CALI_FUNC_CMD_ON：表示需要校准）
 * @retval 0-校准还未完成，1-校准已完成
 */
static bool_t cali_gyro_hook(uint32_t *cali, bool_t cmd); // gyro device cali function

/**
 * @brief 云台设备校准
 * @param cali 指向head数据的指针（当cmd为CALI_FUNC_CMD_INIT时，参数是输入；当cmd为CALI_FUNC_CMD_ON时，参数是输出）
 * @param[in] cmd 校准指令（CALI_FUNC_CMD_INIT：表示使用校准数据初始化原始数据；CALI_FUNC_CMD_ON：表示需要校准）
 * @retval 0-校准还未完成，1-校准已完成
 */
static bool_t cali_gimbal_hook(uint32_t *cali, bool_t cmd); // gimbal device cali function

void *cali_hook_fun[CALI_LIST_LENGTH] = {cali_head_hook, cali_gimbal_hook, cali_gyro_hook, NULL, NULL};

/*-----------------------------------函数实现-----------------------------------*/

void calibrate_task(void const *pvParameters)
{
  static uint8_t i = 0;

  calibrate_RC = get_remote_ctrl_point_cali();

  while (1)
  {

    RC_cmd_to_calibrate();

    for (i = 0; i < CALI_LIST_LENGTH; i++)
    {
      if (cali_sensor[i].cali_cmd)
      {
        if (cali_sensor[i].cali_hook != NULL)
        {

          if (cali_sensor[i].cali_hook(cali_sensor_buf[i], CALI_FUNC_CMD_ON))
          {
            // done
            cali_sensor[i].name[0] = cali_name[i][0];
            cali_sensor[i].name[1] = cali_name[i][1];
            cali_sensor[i].name[2] = cali_name[i][2];
            // set 0x55
            cali_sensor[i].cali_done = CALIED_FLAG;

            cali_sensor[i].cali_cmd = 0;
            // write
            cali_data_write();
          }
        }
      }
    }
    osDelay(CALIBRATE_CONTROL_TIME);
#if INCLUDE_uxTaskGetStackHighWaterMark
    calibrate_task_stack = uxTaskGetStackHighWaterMark(NULL);
#endif
  }
}

int8_t get_control_temperature(void)
{

  return head_cali.temperature;
}

void get_flash_latitude(float *latitude)
{

  if (latitude == NULL)
  {

    return;
  }
  if (cali_sensor[CALI_HEAD].cali_done == CALIED_FLAG)
  {
    *latitude = head_cali.latitude;
  }
  else
  {
    *latitude = 22.0f;
  }
}

static void RC_cmd_to_calibrate(void)
{
  static const uint8_t BEGIN_FLAG = 1;
  static const uint8_t GIMBAL_FLAG = 2;
  static const uint8_t GYRO_FLAG = 3;
  static const uint8_t CHASSIS_FLAG = 4;

  static uint8_t i;
  static uint32_t rc_cmd_systemTick = 0;
  static uint16_t buzzer_time = 0;
  static uint16_t rc_cmd_time = 0;
  static uint8_t rc_action_flag = 0;

  // 如果已经在校准，就返回
  for (i = 0; i < CALI_LIST_LENGTH; i++)
  {
    if (cali_sensor[i].cali_cmd)
    {
      buzzer_time = 0;
      rc_cmd_time = 0;
      rc_action_flag = 0;

      return;
    }
  }

  if (rc_action_flag == 0 && rc_cmd_time > RC_CMD_LONG_TIME)
  {
    rc_cmd_systemTick = xTaskGetTickCount();
    rc_action_flag = BEGIN_FLAG;
    rc_cmd_time = 0;
  }
  else if (rc_action_flag == GIMBAL_FLAG && rc_cmd_time > RC_CMD_LONG_TIME)
  {
    // 云台gimbal校准
    rc_action_flag = 0;
    rc_cmd_time = 0;
    cali_sensor[CALI_GIMBAL].cali_cmd = 1;
    cali_buzzer_off();
  }
  else if (rc_action_flag == 3 && rc_cmd_time > RC_CMD_LONG_TIME)
  {
    // 陀螺仪gyro校准
    rc_action_flag = 0;
    rc_cmd_time = 0;
    cali_sensor[CALI_GYRO].cali_cmd = 1;
    // 更新控制温度
    head_cali.temperature = (int8_t)(cali_get_mcu_temperature()) + 10;
    if (head_cali.temperature > (int8_t)(GYRO_CONST_MAX_TEMP))
    {
      head_cali.temperature = (int8_t)(GYRO_CONST_MAX_TEMP);
    }
    cali_buzzer_off();
  }
  else if (rc_action_flag == CHASSIS_FLAG && rc_cmd_time > RC_CMD_LONG_TIME)
  {
    rc_action_flag = 0;
    rc_cmd_time = 0;
    // 通过CAN线发送重设ID的命令给底盘3508电机
    CAN_cmd_chassis_reset_ID();
    CAN_cmd_chassis_reset_ID();
    CAN_cmd_chassis_reset_ID();
    cali_buzzer_off();
  }

  if (calibrate_RC->rc.ch[0] < -RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[1] < -RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[2] > RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[3] < -RC_CALI_VALUE_HOLE && switch_is_down(calibrate_RC->rc.s[0]) && switch_is_down(calibrate_RC->rc.s[1]) && rc_action_flag == 0)
  {
    // 两个摇杆都向中下打，保持2s
    rc_cmd_time++;
  }
  else if (calibrate_RC->rc.ch[0] > RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[1] > RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[2] < -RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[3] > RC_CALI_VALUE_HOLE && switch_is_down(calibrate_RC->rc.s[0]) && switch_is_down(calibrate_RC->rc.s[1]) && rc_action_flag != 0)
  {
    // 两个摇杆都向外上打，保持2s
    rc_cmd_time++;
    rc_action_flag = GIMBAL_FLAG;
  }
  else if (calibrate_RC->rc.ch[0] > RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[1] < -RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[2] < -RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[3] < -RC_CALI_VALUE_HOLE && switch_is_down(calibrate_RC->rc.s[0]) && switch_is_down(calibrate_RC->rc.s[1]) && rc_action_flag != 0)
  {
    // 两个摇杆都向外下打，保持2s
    rc_cmd_time++;
    rc_action_flag = GYRO_FLAG;
  }
  else if (calibrate_RC->rc.ch[0] < -RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[1] > RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[2] > RC_CALI_VALUE_HOLE && calibrate_RC->rc.ch[3] > RC_CALI_VALUE_HOLE && switch_is_down(calibrate_RC->rc.s[0]) && switch_is_down(calibrate_RC->rc.s[1]) && rc_action_flag != 0)
  {
    // 两个摇杆都向中上打，保持2s
    rc_cmd_time++;
    rc_action_flag = CHASSIS_FLAG;
  }
  else
  {
    rc_cmd_time = 0;
  }

  calibrate_systemTick = xTaskGetTickCount();

  if (calibrate_systemTick - rc_cmd_systemTick > CALIBRATE_END_TIME)
  {
    // 超过20s，停止
    rc_action_flag = 0;
    return;
  }
  else if (calibrate_systemTick - rc_cmd_systemTick > RC_CALI_BUZZER_MIDDLE_TIME && rc_cmd_systemTick != 0 && rc_action_flag != 0)
  {
    rc_cali_buzzer_middle_on();
  }
  else if (calibrate_systemTick - rc_cmd_systemTick > 0 && rc_cmd_systemTick != 0 && rc_action_flag != 0)
  {
    rc_cali_buzzer_start_on();
  }

  if (rc_action_flag != 0)
  {
    buzzer_time++;
  }

  if (buzzer_time > RCCALI_BUZZER_CYCLE_TIME && rc_action_flag != 0)
  {
    buzzer_time = 0;
  }
  if (buzzer_time > RC_CALI_BUZZER_PAUSE_TIME && rc_action_flag != 0)
  {
    cali_buzzer_off();
  }
}

void cali_param_init(void)
{
  uint8_t i = 0;

  for (i = 0; i < CALI_LIST_LENGTH; i++)
  {
    cali_sensor[i].flash_len = cali_sensor_size[i];
    cali_sensor[i].flash_buf = cali_sensor_buf[i];
    cali_sensor[i].cali_hook = (bool_t(*)(uint32_t *, bool_t))cali_hook_fun[i];
  }

  cali_data_read();

  for (i = 0; i < CALI_LIST_LENGTH; i++)
  {
    if (cali_sensor[i].cali_done == CALIED_FLAG)
    {
      if (cali_sensor[i].cali_hook != NULL)
      {
        // 如果已经校准过，设置为init状态
        cali_sensor[i].cali_hook(cali_sensor_buf[i], CALI_FUNC_CMD_INIT);
      }
    }
  }
}

static void cali_data_read(void)
{
  uint8_t flash_read_buf[CALI_SENSOR_HEAD_LENGTH * 4];
  uint8_t i = 0;
  uint16_t offset = 0;
  for (i = 0; i < CALI_LIST_LENGTH; i++)
  {

    // 从flash读取数据
    cali_flash_read(FLASH_USER_ADDR + offset, cali_sensor[i].flash_buf, cali_sensor[i].flash_len);

    offset += cali_sensor[i].flash_len * 4;

    // 读取name和cali flag
    cali_flash_read(FLASH_USER_ADDR + offset, (uint32_t *)flash_read_buf, CALI_SENSOR_HEAD_LENGTH);

    cali_sensor[i].name[0] = flash_read_buf[0];
    cali_sensor[i].name[1] = flash_read_buf[1];
    cali_sensor[i].name[2] = flash_read_buf[2];
    cali_sensor[i].cali_done = flash_read_buf[3];

    offset += CALI_SENSOR_HEAD_LENGTH * 4;

    if (cali_sensor[i].cali_done != CALIED_FLAG && cali_sensor[i].cali_hook != NULL)
    {
      cali_sensor[i].cali_cmd = 1;
    }
  }
}

static void cali_data_write(void)
{
  uint8_t i = 0;
  uint16_t offset = 0;

  for (i = 0; i < CALI_LIST_LENGTH; i++)
  {
    // 复制设备校准数据
    memcpy((void *)(flash_write_buf + offset), (void *)cali_sensor[i].flash_buf, cali_sensor[i].flash_len * 4);
    offset += cali_sensor[i].flash_len * 4;

    // 复制设备的名称和cali flag
    memcpy((void *)(flash_write_buf + offset), (void *)cali_sensor[i].name, CALI_SENSOR_HEAD_LENGTH * 4);
    offset += CALI_SENSOR_HEAD_LENGTH * 4;
  }

  // 擦除本页闪存
  cali_flash_erase(FLASH_USER_ADDR, 1);
  // 写入数据
  cali_flash_write(FLASH_USER_ADDR, (uint32_t *)flash_write_buf, (FLASH_WRITE_BUF_LENGTH + 3) / 4);
}

static bool_t cali_head_hook(uint32_t *cali, bool_t cmd)
{
  head_cali_t *local_cali_t = (head_cali_t *)cali;
  if (cmd == CALI_FUNC_CMD_INIT)
  {
    // memcpy(&head_cali, local_cali_t, sizeof(head_cali_t));
    return 1;
  }
  // self id
  local_cali_t->self_id = SELF_ID;
  // imu control temperature
  local_cali_t->temperature = (int8_t)(cali_get_mcu_temperature()) + 10;
  // head_cali.temperature = (int8_t)(cali_get_mcu_temperature()) + 10;
  if (local_cali_t->temperature > (int8_t)(GYRO_CONST_MAX_TEMP))
  {
    local_cali_t->temperature = (int8_t)(GYRO_CONST_MAX_TEMP);
  }

  local_cali_t->firmware_version = FIRMWARE_VERSION;
  // shenzhen latitude
  local_cali_t->latitude = 22.0f;

  return 1;
}

static bool_t cali_gyro_hook(uint32_t *cali, bool_t cmd)
{
  imu_cali_t *local_cali_t = (imu_cali_t *)cali;
  if (cmd == CALI_FUNC_CMD_INIT)
  {
    gyro_set_cali(local_cali_t->scale, local_cali_t->offset);

    return 0;
  }
  else if (cmd == CALI_FUNC_CMD_ON)
  {
    static uint16_t count_time = 0;
    gyro_cali_fun(local_cali_t->scale, local_cali_t->offset, &count_time);
    if (count_time > GYRO_CALIBRATE_TIME)
    {
      count_time = 0;
      cali_buzzer_off();
      gyro_cali_enable_control();
      return 1;
    }
    else
    {
      // 禁用遥控器使机器人不运动
      gyro_cali_disable_control();
      imu_start_buzzer();

      return 0;
    }
  }

  return 0;
}

static bool_t cali_gimbal_hook(uint32_t *cali, bool_t cmd)
{

  gimbal_cali_t *local_cali_t = (gimbal_cali_t *)cali;
  if (cmd == CALI_FUNC_CMD_INIT)
  {
    set_cali_gimbal_hook(local_cali_t->yaw_offset, local_cali_t->pitch_offset,
                         local_cali_t->yaw_max_angle, local_cali_t->yaw_min_angle,
                         local_cali_t->pitch_max_angle, local_cali_t->pitch_min_angle);

    return 0;
  }
  else if (cmd == CALI_FUNC_CMD_ON)
  {
    if (cmd_cali_gimbal_hook(&local_cali_t->yaw_offset, &local_cali_t->pitch_offset,
                             &local_cali_t->yaw_max_angle, &local_cali_t->yaw_min_angle,
                             &local_cali_t->pitch_max_angle, &local_cali_t->pitch_min_angle))
    {
      cali_buzzer_off();

      return 1;
    }
    else
    {
      gimbal_start_buzzer();

      return 0;
    }
  }

  return 0;
}
