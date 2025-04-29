/**
 * @file gimbal_behaviour.c
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
 */

/*-----------------------------------头文件引用-----------------------------------*/

#include "gimbal_behaviour.h"
#include "arm_math.h"
#include "bsp_buzzer.h"
#include "detect_task.h"
#include "string.h"
#include "user_lib.h"
#include "main.h"
#include "shoot.h"

/*-----------------------------------宏定义-----------------------------------*/

// 云台校准时，设置蜂鸣器频率和强度
#define gimbal_warn_buzzer_on() buzzer_on(31, 20000)
#define gimbal_warn_buzzer_off() buzzer_off()
#define int_abs(x) ((x) > 0 ? (x) : (-x))

/**
 * @brief 遥控器的死区判断，因为遥控器有误差，摇杆在中间位置的时候，值不一定为0
 * @param input 输入的遥控器值
 * @param output 输出的死区处理后遥控器值
 * @param deadline 死区值
 */
#define rc_deadband_limit(input, output, dealine)    \
  {                                                  \
    if ((input) > (dealine) || (input) < -(dealine)) \
    {                                                \
      (output) = (input);                            \
    }                                                \
    else                                             \
    {                                                \
      (output) = 0;                                  \
    }                                                \
  }

/**
 * @brief 通过判断角速度来判断云台是否达到极限位置
 * @param gyro 对应轴的角速度，单位rad/s
 * @param timing 计时时间，到达GIMBAL_CALI_STEP_TIME的时间后归零
 * @param record 记录的角度，单位rad
 * @param feedback 反馈的角度，单位rad
 * @param record 记录的编码值，单位raw
 * @param feedback 反馈的编码值，单位raw
 * @param cali 校准的步骤，完成一次就加一
 */
#define gimbal_cali_gyro_judge(gyro, cmd_time, angle_set, angle, ecd_set, ecd, step) \
  {                                                                                  \
    if ((gyro) < GIMBAL_CALI_GYRO_LIMIT)                                             \
    {                                                                                \
      (cmd_time)++;                                                                  \
      if ((cmd_time) > GIMBAL_CALI_STEP_TIME)                                        \
      {                                                                              \
        (cmd_time) = 0;                                                              \
        (angle_set) = (angle);                                                       \
        (ecd_set) = (ecd);                                                           \
        (step)++;                                                                    \
      }                                                                              \
    }                                                                                \
  }

/*-----------------------------------变量声明-----------------------------------*/

extern uint8_t KEY_flag;
extern InputData inputdata; // 上位机发来的自瞄数据
short yuntai_x;
short yuntai_y;

// 云台行为状态机
static gimbal_behaviour_e gimbal_behaviour = GIMBAL_ABSOLUTE_ANGLE;
// 自瞄开关标志符
uint8_t autoaim_flag = 1;
uint32_t pressRightTick = 0;

uint8_t is_target_active = 0;                      // 自瞄是否激活的标志位
fp32 last_shoot_yaw = 0, last_shoot_pitch = 0; // 保存上次的自瞄数据
extern __IO uint32_t uwTick;                   // 系统时钟
uint32_t autoTick = 0;                         // 自瞄计数

extern shoot_control_t shoot_control;

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 云台行为状态机设置
 * @param gimbal_mode_set 云台数据指针
 * @retval none
 */
static void gimbal_behaviour_set(gimbal_control_t *gimbal_mode_set);

/**
 * @brief 当云台行为模式为GIMBAL_ZERO_FORCE，即无力模式时，这个函数会被调用。云台控制模式是raw模式，即原始模式，意味着设定值会直接发送到CAN总线上，这个函数会将yaw和pitch都设为0
 * @param[out] yaw 发送给yaw电机的原始值
 * @param[out] pitch 发送给pitch电机的原始值
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
static void gimbal_zero_force_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 当云台行为模式为GIMBAL_INIT，即初始化模式时，这个函数会被调用。电机是陀螺仪角度控制，云台先抬起pitch轴，后旋转yaw轴
 * @param[out] yaw yaw轴角度控制，为角度增量，单位rad
 * @param[out] pitch pitch轴角度控制，为角度增量，单位rad
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
static void gimbal_init_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 当云台行为模式为GIMBAL_CALI，即校准模式时，这个函数会被调用。电机是raw控制，云台先抬起pitch，放下pitch，再正转yaw，最后反转yaw，并同时记录下角度和编码值
 * @param[out] yaw 发送给yaw电机的原始值，会直接通过CAN发送到电机
 * @param[out] pitch 发送给pitch电机的原始值，会直接通过CAN发送到电机
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
static void gimbal_cali_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 当云台行为模式为GIMBAL_ABSOLUTE_ANGLE，即绝对角度控制模式时，这个函数会被调用。云台控制模式是陀螺仪角度控制
 * @param[out] yaw yaw轴角度控制，为角度的增量，单位rad
 * @param[out] pitch pitch轴角度控制，为角度的增量，单位rad
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
static void gimbal_absolute_angle_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 当云台行为模式为GIMBAL_RELATIVE_ANGLE，即相对角度控制模式时，这个函数会被调用。云台控制模式是电机编码值控制
 * @param[out] yaw yaw轴角度控制，为角度的增量，单位rad
 * @param[out] pitch pitch轴角度控制，为角度的增量，单位rad
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
static void gimbal_relative_angle_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 当云台行为模式为GIMBAL_MOTIONLESS，即空闲模式时，这个函数会被调用。云台控制模式是电机编码值控制
 * @param[out] yaw yaw轴角度控制，为角度的增量，单位rad
 * @param[out] pitch pitch轴角度控制，为角度的增量，单位rad
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
static void gimbal_motionless_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/**
 * @brief 当云台行为模式为GIMBAL_AUTOAIM，即自瞄模式时，这个函数会被调用。云台控制模式是陀螺仪角度控制
 * @param[out] yaw yaw轴角度控制，为角度的增量，单位rad
 * @param[out] pitch pitch轴角度控制，为角度的增量，单位rad
 * @param[in] gimbal_control_set 云台数据指针
 * @retval none
 */
void gimbal_autoaim_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set);

/*-----------------------------------函数实现-----------------------------------*/

void gimbal_behaviour_mode_set(gimbal_control_t *gimbal_mode_set)
{
  if (gimbal_mode_set == NULL)
  {
    return;
  }
  // 云台行为状态机设置
  gimbal_behaviour_set(gimbal_mode_set);

  // 根据云台行为状态机设置电机状态机
  if (gimbal_behaviour == GIMBAL_ZERO_FORCE)
  {
    gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_RAW;
    gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_RAW;
  }
  else if (gimbal_behaviour == GIMBAL_INIT)
  {
    gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_ENCONDE;
    gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_ENCONDE;
  }
  // else if (gimbal_behaviour == GIMBAL_CALI)
  // {
  //     gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_RAW;
  //     gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_RAW;
  // }
  else if (gimbal_behaviour == GIMBAL_ABSOLUTE_ANGLE)
  {
    gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_GYRO;
    gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_GYRO;
  }
  else if (gimbal_behaviour == GIMBAL_RELATIVE_ANGLE)
  {
    gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_ENCONDE;
    gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_ENCONDE;
  }
  else if (gimbal_behaviour == GIMBAL_AUTOAIM)
  {
    gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_GYRO;
    gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_GYRO;
  }
  // else if (gimbal_behaviour == GIMBAL_MOTIONLESS)
  // {
  //     gimbal_mode_set->gimbal_yaw_motor.gimbal_motor_mode = GIMBAL_MOTOR_ENCONDE;
  //     gimbal_mode_set->gimbal_pitch_motor.gimbal_motor_mode = GIMBAL_MOTOR_ENCONDE;
  // }
}

void gimbal_behaviour_control_set(fp32 *add_yaw, fp32 *add_pitch, gimbal_control_t *gimbal_control_set)
{

  if (add_yaw == NULL || add_pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }

  if (gimbal_behaviour == GIMBAL_ZERO_FORCE)
  {
    gimbal_zero_force_control(add_yaw, add_pitch, gimbal_control_set);
  }
  else if (gimbal_behaviour == GIMBAL_INIT)
  {
    gimbal_init_control(add_yaw, add_pitch, gimbal_control_set);
  }
  // else if (gimbal_behaviour == GIMBAL_CALI)
  // {
  //     gimbal_cali_control(add_yaw, add_pitch, gimbal_control_set);
  // }
  else if (gimbal_behaviour == GIMBAL_ABSOLUTE_ANGLE)
  {
    gimbal_absolute_angle_control(add_yaw, add_pitch, gimbal_control_set);
  }
  else if (gimbal_behaviour == GIMBAL_RELATIVE_ANGLE)
  {
    gimbal_relative_angle_control(add_yaw, add_pitch, gimbal_control_set);
  }
  else if (gimbal_behaviour == GIMBAL_AUTOAIM)
  {
    gimbal_autoaim_control(add_yaw, add_pitch, gimbal_control_set);
  }
  // else if (gimbal_behaviour == GIMBAL_MOTIONLESS)
  // {
  //     gimbal_motionless_control(add_yaw, add_pitch, gimbal_control_set);
  // }
}

bool_t gimbal_cmd_to_chassis_stop(void)
{
  if (gimbal_behaviour == GIMBAL_INIT || gimbal_behaviour == GIMBAL_CALI || gimbal_behaviour == GIMBAL_MOTIONLESS || gimbal_behaviour == GIMBAL_ZERO_FORCE)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

bool_t gimbal_cmd_to_shoot_stop(void)
{
  if (gimbal_behaviour == GIMBAL_INIT || gimbal_behaviour == GIMBAL_CALI || gimbal_behaviour == GIMBAL_ZERO_FORCE)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

static void gimbal_behaviour_set(gimbal_control_t *gimbal_mode_set)
{
  extern char g_behaviour;
  if (gimbal_mode_set == NULL)
  {
    return;
  }
  // 校准行为，return不会设置其他模式
  if (gimbal_behaviour == GIMBAL_CALI && gimbal_mode_set->gimbal_cali.step != GIMBAL_CALI_END_STEP)
  {
    return;
  }
  // 如果外部使得校准步骤从0变成start，则进入校准模式
  if (gimbal_mode_set->gimbal_cali.step == GIMBAL_CALI_START_STEP && !toe_is_error(DBUS_TOE))
  {
    gimbal_behaviour = GIMBAL_CALI;
    return;
  }
  // 初始化模式判断是否达到中值位置
  if (gimbal_behaviour == GIMBAL_INIT)
  {
    static uint16_t init_time = 0;
    static uint16_t init_stop_time = 0;
    init_time++;

    if ((fabs(gimbal_mode_set->gimbal_yaw_motor.relative_angle - INIT_YAW_SET) < GIMBAL_INIT_ANGLE_ERROR &&
         fabs(gimbal_mode_set->gimbal_pitch_motor.absolute_angle - INIT_PITCH_SET) < GIMBAL_INIT_ANGLE_ERROR))
    {

      if (init_stop_time < GIMBAL_INIT_STOP_TIME)
      {
        init_stop_time++;
      }
    }
    else
    {

      if (init_time < GIMBAL_INIT_TIME)
      {
        init_time++;
      }
    }

    // 超过初始化最大时间，或者已经稳定到中值一段时间，退出初始化状态，开关打下档，或者掉线
    if (init_time < GIMBAL_INIT_TIME && init_stop_time < GIMBAL_INIT_STOP_TIME &&
        !switch_is_down(gimbal_mode_set->gimbal_rc_ctrl->rc.s[GIMBAL_MODE_CHANNEL]) && !toe_is_error(DBUS_TOE))
    {
      return;
    }
    else
    {
      init_stop_time = 0;
      init_time = 0;
    }
  }

  // 遥控器开关控制云台状态
  if (switch_is_down(gimbal_mode_set->gimbal_rc_ctrl->rc.s[GIMBAL_MODE_CHANNEL]))
  {
    gimbal_behaviour = GIMBAL_ZERO_FORCE;
  }
  else if (switch_is_mid(gimbal_mode_set->gimbal_rc_ctrl->rc.s[GIMBAL_MODE_CHANNEL]))
  {
    gimbal_behaviour = GIMBAL_RELATIVE_ANGLE;
  }
  else if (switch_is_up(gimbal_mode_set->gimbal_rc_ctrl->rc.s[GIMBAL_MODE_CHANNEL]))
  {
    gimbal_behaviour = GIMBAL_AUTOAIM;
  }

  // DBUS异常
  if (toe_is_error(DBUS_TOE))
  {
    gimbal_behaviour = GIMBAL_ZERO_FORCE;
  }

  // 判断进入init状态机
  {
    static gimbal_behaviour_e last_gimbal_behaviour = GIMBAL_ZERO_FORCE;
    if (last_gimbal_behaviour == GIMBAL_ZERO_FORCE && gimbal_behaviour != GIMBAL_ZERO_FORCE)
    {
      gimbal_behaviour = GIMBAL_INIT;
    }
    last_gimbal_behaviour = gimbal_behaviour;
  }
  g_behaviour = gimbal_behaviour;
}

static void gimbal_zero_force_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }

  *yaw = 0.0f;
  *pitch = 0.0f;
}

static void gimbal_init_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }

  // 初始化状态控制量计算
  if (fabs(INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.absolute_angle) > GIMBAL_INIT_ANGLE_ERROR)
  {
    *pitch = (INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.absolute_angle) * GIMBAL_INIT_PITCH_SPEED;
    *yaw = 0.0f;
    // *yaw = (INIT_YAW_SET - gimbal_control_set->gimbal_yaw_motor.relative_angle) * GIMBAL_INIT_YAW_SPEED;
  }
  else
  {
    *pitch = (INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.absolute_angle) * GIMBAL_INIT_PITCH_SPEED;
    *yaw = (INIT_YAW_SET - gimbal_control_set->gimbal_yaw_motor.relative_angle) * GIMBAL_INIT_YAW_SPEED;
  }
}

static void gimbal_cali_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }
  static uint16_t cali_time = 0;

  if (gimbal_control_set->gimbal_cali.step == GIMBAL_CALI_PITCH_MAX_STEP)
  {

    *pitch = GIMBAL_CALI_MOTOR_SET;
    *yaw = 0;

    // 判断陀螺仪数据，并记录最大最小角度数据
    gimbal_cali_gyro_judge(gimbal_control_set->gimbal_pitch_motor.motor_gyro, cali_time, gimbal_control_set->gimbal_cali.max_pitch,
                           gimbal_control_set->gimbal_pitch_motor.absolute_angle, gimbal_control_set->gimbal_cali.max_pitch_ecd,
                           gimbal_control_set->gimbal_pitch_motor.gimbal_motor_measure->ecd, gimbal_control_set->gimbal_cali.step);
  }
  else if (gimbal_control_set->gimbal_cali.step == GIMBAL_CALI_PITCH_MIN_STEP)
  {
    *pitch = -GIMBAL_CALI_MOTOR_SET;
    *yaw = 0;

    gimbal_cali_gyro_judge(gimbal_control_set->gimbal_pitch_motor.motor_gyro, cali_time, gimbal_control_set->gimbal_cali.min_pitch,
                           gimbal_control_set->gimbal_pitch_motor.absolute_angle, gimbal_control_set->gimbal_cali.min_pitch_ecd,
                           gimbal_control_set->gimbal_pitch_motor.gimbal_motor_measure->ecd, gimbal_control_set->gimbal_cali.step);
  }
  else if (gimbal_control_set->gimbal_cali.step == GIMBAL_CALI_YAW_MAX_STEP)
  {
    *pitch = 0;
    *yaw = GIMBAL_CALI_MOTOR_SET;

    gimbal_cali_gyro_judge(gimbal_control_set->gimbal_yaw_motor.motor_gyro, cali_time, gimbal_control_set->gimbal_cali.max_yaw,
                           gimbal_control_set->gimbal_yaw_motor.absolute_angle, gimbal_control_set->gimbal_cali.max_yaw_ecd,
                           gimbal_control_set->gimbal_yaw_motor.gimbal_motor_measure->ecd, gimbal_control_set->gimbal_cali.step);
  }

  else if (gimbal_control_set->gimbal_cali.step == GIMBAL_CALI_YAW_MIN_STEP)
  {
    *pitch = 0;
    *yaw = -GIMBAL_CALI_MOTOR_SET;

    gimbal_cali_gyro_judge(gimbal_control_set->gimbal_yaw_motor.motor_gyro, cali_time, gimbal_control_set->gimbal_cali.min_yaw,
                           gimbal_control_set->gimbal_yaw_motor.absolute_angle, gimbal_control_set->gimbal_cali.min_yaw_ecd,
                           gimbal_control_set->gimbal_yaw_motor.gimbal_motor_measure->ecd, gimbal_control_set->gimbal_cali.step);
  }
  else if (gimbal_control_set->gimbal_cali.step == GIMBAL_CALI_END_STEP)
  {
    cali_time = 0;
  }
}

static void gimbal_absolute_angle_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }

  static int16_t yaw_channel = 0, pitch_channel = 0;

  rc_deadband_limit(-gimbal_control_set->gimbal_rc_ctrl->rc.ch[YAW_CHANNEL], yaw_channel, RC_DEADBAND);
  rc_deadband_limit(gimbal_control_set->gimbal_rc_ctrl->rc.ch[PITCH_CHANNEL], pitch_channel, RC_DEADBAND);

  if (KEY_flag)
  {
    *yaw = yaw_channel * YAW_RC_SEN - gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN;
    *pitch = pitch_channel * PITCH_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.y * PITCH_MOUSE_SEN;
  }
  else
  {
    *yaw = yaw_channel * YAW_RC_SEN - gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN - yuntai_x * YAW_CAMERA_SEN;
    // *yaw = yaw_channel * YAW_RC_SEN - gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN;
    *pitch = pitch_channel * PITCH_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.y * PITCH_MOUSE_SEN + yuntai_y * PITCH_CAMERA_SEN;
  }
}

static void gimbal_relative_angle_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }
  static int16_t yaw_channel = 0, pitch_channel = 0;

  rc_deadband_limit(-gimbal_control_set->gimbal_rc_ctrl->rc.ch[YAW_CHANNEL], yaw_channel, RC_DEADBAND);
  rc_deadband_limit(gimbal_control_set->gimbal_rc_ctrl->rc.ch[PITCH_CHANNEL], pitch_channel, RC_DEADBAND);
  // *yaw = yaw_channel * YAW_RC_SEN - gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN;
  // *pitch = pitch_channel * PITCH_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.y * PITCH_MOUSE_SEN;
  *yaw = yaw_channel * YAW_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN;
  *pitch = pitch_channel * PITCH_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.y * PITCH_MOUSE_SEN;
}

static void gimbal_motionless_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }
  *yaw = 0.0f;
  *pitch = 0.0f;
}

void gimbal_autoaim_control(fp32 *yaw, fp32 *pitch, gimbal_control_t *gimbal_control_set)
{
  if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
  {
    return;
  }

  if (uwTick - pressRightTick >= 1000)
  {
    if(gimbal_control_set->gimbal_rc_ctrl->mouse.press_r)
    {
      pressRightTick = uwTick;
      if(autoaim_flag)
        autoaim_flag = 0;
      else
        autoaim_flag = 1;
    }
  }

  // yaw和pitch遥控器通道输入值（静态变量）
  static int16_t yaw_channel = 0, pitch_channel = 0;
  // 死区限制
  rc_deadband_limit(-gimbal_control_set->gimbal_rc_ctrl->rc.ch[YAW_CHANNEL], yaw_channel, RC_DEADBAND);
  rc_deadband_limit(gimbal_control_set->gimbal_rc_ctrl->rc.ch[PITCH_CHANNEL], pitch_channel, RC_DEADBAND);

  fp32 yaw1, yaw2, pitch1, pitch2 = 0;           // 自瞄与遥控同时存在

  // 根据限制后的遥控器通道输入值计算手动角度控制值
  yaw1 = yaw_channel * YAW_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN;
  pitch1 = pitch_channel * PITCH_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.y * PITCH_MOUSE_SEN;
  // 获取当前绝对角度
  fp32 current_yaw = gimbal_control_set->gimbal_yaw_motor.absolute_angle;
  fp32 current_pitch = gimbal_control_set->gimbal_pitch_motor.absolute_angle;
  // 设置增量值
  fp32 yaw_increment = inputdata.shoot_yaw - current_yaw;
  fp32 pitch_increment = inputdata.shoot_pitch - current_pitch;
  // 防止角度跳变，设置一个最小的增量阈值，若增量超过阈值，则继续调整角度
  // 装甲板宽度（角）约为0.066f，瞄准到中心附近的0.03范围内可认为自瞄成功，故停止阈值设置为0.015较为合理
  const fp32 increment_threshold = 0.015f;
  // 距离目标点0.03f时可认为进入装甲板范围，此时减小变化速度
  const fp32 armor_threshold = 0.03f;
  // 距离目标点0.12f时可认为进入对方机器人身体范围，敌方机器人小陀螺大致在此范围内运动
  const fp32 robot_threshold = 0.12f;
  // 相机视线中心到边界距离约为0.2~0.23
  const fp32 camera_threshold = 0.20f;

  // 每个35ms检测一次自瞄数据变化，若变化则启动自瞄控制，否则启动手动控制
  if (uwTick - autoTick >= 15)
  {
    autoTick = uwTick;
    // 判断自瞄数据是否变化，不变化表示未检测到目标
    if (inputdata.shoot_yaw != last_shoot_yaw || inputdata.shoot_pitch != last_shoot_pitch)
    {
      is_target_active = 1; // 检测到目标，可以转动
    }
    else
    {
      inputdata.fire = 0;   // 停止开火
      is_target_active = 0; // 未检测到目标
    }

    // 更新上次的自瞄数据
    last_shoot_yaw = inputdata.shoot_yaw;
    last_shoot_pitch = inputdata.shoot_pitch;
  }

  // 满足条件则执行自瞄（遥控器无数据，且自瞄开启，且目标有效）
  if (yaw1 == 0 && pitch1 == 0 && autoaim_flag && is_target_active) // 如果遥控器没有输入，则启动自瞄
  {
    // 确保上位机数据不为初值，避免炮口向零值转动
    if (inputdata.shoot_yaw != 0 && inputdata.shoot_pitch != 0) // 判断yaw的当前角度与自瞄目标角度差值是否大于阈值
    {
      // 离目标位置的距离越远，速度越快，越近则越慢，直至距离小于阈值
			if(fabs(yaw_increment) > increment_threshold && fabs(yaw_increment) < 0.15)
			{
				yaw2 = (yaw_increment > 0 ? 1 : -1) * YAW_AUTO_SEN * (30*fabs(yaw_increment*yaw_increment*yaw_increment)+3.6*fabs(yaw_increment*yaw_increment)+0.8*fabs(yaw_increment));
			}
      else if(fabs(yaw_increment) >= 0.15)
      {
        yaw2 = (yaw_increment > 0 ? 1 : -1) * YAW_AUTO_SEN * (30*fabs(yaw_increment*yaw_increment*yaw_increment)+4.2*fabs(yaw_increment*yaw_increment)+4.4*fabs(yaw_increment)-0.3);
      }
      else // 达到阈值，则不再变化
      {
        yaw2 = 0;
      }

			if(fabs(pitch_increment) > increment_threshold && fabs(pitch_increment) < 0.15)
			{
				pitch2 = (pitch_increment > 0 ? 1 : -1)*fabs(pitch_increment) * PITCH_AUTO_SEN;
			}
      else if(fabs(pitch_increment) >= 0.15)
      {
        pitch2 = (pitch_increment > 0 ? 1 : -1) * PITCH_AUTO_SEN*0.4;
      }
      else // 达到阈值，则不再变化
      {
        pitch2 = 0;
      }

      // 判断是否需要开火
      if (inputdata.fire)
      {
        if (shoot_control.shoot_mode == SHOOT_READY)
        {
          shoot_control.shoot_mode = SHOOT_BULLET;
        }
      }
    }
  }
  else
  {
    // 如果有遥控器输入，则使用遥控输入控制云台
    yaw2 = 0;
    pitch2 = 0;
    is_target_active = 0; // 标记自瞄暂时关闭，由手动接管
  }
  *yaw = yaw1 + yaw2;
  *pitch = pitch1 + pitch2;
}