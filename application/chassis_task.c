/**
 * @file chassis_task.c
 * @brief 底盘任务
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

#include "chassis_task.h"
#include "chassis_behaviour.h"
#include "gimbal_task.h"
#include "cmsis_os.h"
#include "usart.h"
#include "arm_math.h"
#include "pid.h"
#include "remote_control.h"
#include "CAN_receive.h"
#include "detect_task.h"
#include "INS_task.h"
#include "chassis_power_control.h"
#include "ui.h"

/*-----------------------------------宏定义-----------------------------------*/

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

#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t chassis_high_water;
#endif

/*-----------------------------------变量声明-----------------------------------*/

// 底盘运动数据
chassis_move_t chassis_move;

// 底盘轮胎类型
chassis_wheel_type_e chassis_wheel = CHASSIS_WHEEL_OMNIDIRECTIONAL;

float test_angle_set;

fp32 vx_set_tem, vy_set_tem, offset_angle;

extern float rotate_speed;

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief  初始化chassis_move变量，包括pid初始化、遥控器指针初始化、3508底盘电机指针初始化、云台电机初始化、陀螺仪角度指针初始化
 * @param[out] chassis_move_init chassis_move变量指针
 * @retval none
 */
static void chassis_init(chassis_move_t *chassis_move_init);

/**
 * @brief  设置底盘控制模式，主要在chassis_behaviour_mode_set函数中改变
 * @param[out] chassis_move_mode chassis_move变量指针
 * @retval none
 */
static void chassis_set_mode(chassis_move_t *chassis_move_mode);

/**
 * @brief  底盘模式改变，有些参数需要改变，例如底盘控制yaw角度设定值应该变成当前底盘yaw角度
 * @param[out] chassis_move_transit chassis_move变量指针
 * @retval none
 */
void chassis_mode_change_control_transit(chassis_move_t *chassis_move_transit);

/**
 * @brief  底盘测量数据更新，包括电机速度、欧拉角度、机器人速度
 * @param[out] chassis_move_update chassis_move变量指针
 * @retval none
 */
static void chassis_feedback_update(chassis_move_t *chassis_move_update);

/**
 * @brief  设置底盘控制设定点，三个运动控制值由chassis_behaviour_control_set设置
 * @param[out] chassis_move_control chassis_move变量指针
 * @retval none
 */
static void chassis_set_contorl(chassis_move_t *chassis_move_control);

/**
 * @brief  通过三个参数计算四个麦轮速度
 * @param[in] vx_set 纵向速度
 * @param[in] vy_set 横移速度
 * @param[in] wz_set 旋转角速度
 * @param[out] wheel_speed 四个麦轮速度的数组
 * @retval none
 */
static void chassis_vector_to_mecanum_wheel_speed(const fp32 vx_set, const fp32 vy_set, const fp32 wz_set, fp32 wheel_speed[4]);

/**
 * @brief  通过三个参数计算四个全向轮速度
 * @param[in] vx_set 纵向速度
 * @param[in] vy_set 横移速度
 * @param[in] wz_set 旋转角速度
 * @param[out] wheel_speed 四个全向轮速度的数组
 * @retval none
 */
static void chassis_vector_to_omnidirectional_wheel_speed(const fp32 vx_set, const fp32 vy_set, const fp32 wz_set, fp32 wheel_speed[4]);

/**
 * @brief  控制循环，根据控制设定值，计算电机电流值，进行控制
 * @param[out] chassis_move_control_loop chassis_move变量指针
 * @retval none
 */
static void chassis_control_loop(chassis_move_t *chassis_move_control_loop);

/*-----------------------------------函数实现-----------------------------------*/

void chassis_task(void const *pvParameters)
{
  // 空闲一段时间
  vTaskDelay(CHASSIS_TASK_INIT_TIME);
  // 底盘初始化
  chassis_init(&chassis_move);
  // 判断底盘电机是否都在线
  // while (toe_is_error(CHASSIS_MOTOR1_TOE) || toe_is_error(CHASSIS_MOTOR2_TOE) || toe_is_error(CHASSIS_MOTOR3_TOE) || toe_is_error(CHASSIS_MOTOR4_TOE) || toe_is_error(DBUS_TOE))
  // {
  //   vTaskDelay(CHASSIS_CONTROL_TIME_MS);
  // }
  while (1)
  {
    // data_transition();
    // USART1_SEND();
    // 设置底盘控制模式
    chassis_set_mode(&chassis_move);
    // 模式切换数据保存
    chassis_mode_change_control_transit(&chassis_move);
    // 底盘数据更新
    chassis_feedback_update(&chassis_move);
    // 底盘控制量设置
    chassis_set_contorl(&chassis_move);
    // 底盘控制PID计算
    chassis_control_loop(&chassis_move);

    // 确保至少一个电机在线，这样CAN控制包也可以被接收到
    if (!(toe_is_error(CHASSIS_MOTOR1_TOE) && toe_is_error(CHASSIS_MOTOR2_TOE) && toe_is_error(CHASSIS_MOTOR3_TOE) && toe_is_error(CHASSIS_MOTOR4_TOE)))
    {
      // 当遥控器掉线的时候，发送给底盘电机零电流
      if (toe_is_error(DBUS_TOE))
      {
        CAN_cmd_chassis(0, 0, 0, 0);
      }
      else
      {
        // 发送控制电流
        CAN_cmd_chassis(chassis_move.motor_chassis[0].give_current, chassis_move.motor_chassis[1].give_current,
                        chassis_move.motor_chassis[2].give_current, chassis_move.motor_chassis[3].give_current);
      }
    }
    // 系统延时
    vTaskDelay(CHASSIS_CONTROL_TIME_MS);

#if INCLUDE_uxTaskGetStackHighWaterMark
    chassis_high_water = uxTaskGetStackHighWaterMark(NULL);
#endif
  }
}

static void chassis_init(chassis_move_t *chassis_move_init)
{
  if (chassis_move_init == NULL)
  {
    return;
  }

  // 底盘速度环pid值
  const static fp32 motor_speed_pid[3] = {M3505_MOTOR_SPEED_PID_KP, M3505_MOTOR_SPEED_PID_KI, M3505_MOTOR_SPEED_PID_KD};

  // 底盘角度pid值
  const static fp32 chassis_yaw_pid[3] = {CHASSIS_FOLLOW_GIMBAL_PID_KP, CHASSIS_FOLLOW_GIMBAL_PID_KI, CHASSIS_FOLLOW_GIMBAL_PID_KD};

  const static fp32 chassis_x_order_filter[1] = {CHASSIS_ACCEL_X_NUM};
  const static fp32 chassis_y_order_filter[1] = {CHASSIS_ACCEL_Y_NUM};
  uint8_t i;

  // 底盘开机状态为原始模式
  chassis_move_init->chassis_mode = CHASSIS_VECTOR_RAW;
  // 获取遥控器指针
  chassis_move_init->chassis_RC = get_remote_control_point();
  // 获取陀螺仪姿态角指针
  chassis_move_init->chassis_INS_angle = get_INS_angle_point();
  // 获取云台电机数据指针
  // chassis_move_init->chassis_yaw_motor = get_yaw_motor_point();
  // chassis_move_init->chassis_pitch_motor = get_pitch_motor_point();
  // 获取底盘电机数据指针，并初始化pid
  for (i = 0; i < 4; i++)
  {
    chassis_move_init->motor_chassis[i].chassis_motor_measure = get_chassis_motor_measure_point(i);
    PID_init(&chassis_move_init->motor_speed_pid[i], PID_POSITION, motor_speed_pid, M3505_MOTOR_SPEED_PID_MAX_OUT, M3505_MOTOR_SPEED_PID_MAX_IOUT);
  }
  // 初始化角度pid
  PID_init(&chassis_move_init->chassis_angle_pid, PID_POSITION, chassis_yaw_pid, CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT, CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT);

  // 用一阶滤波代替斜波函数生成
  first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vx, CHASSIS_CONTROL_TIME, chassis_x_order_filter);
  first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vy, CHASSIS_CONTROL_TIME, chassis_y_order_filter);

  // 最大、最小速度
  chassis_move_init->vx_max_speed = NORMAL_MAX_CHASSIS_SPEED_X * 2.0f / 3.0f;
  chassis_move_init->vx_min_speed = -NORMAL_MAX_CHASSIS_SPEED_X * 2.0f / 3.0f;

  chassis_move_init->vy_max_speed = NORMAL_MAX_CHASSIS_SPEED_Y * 2.0f / 3.0f;
  chassis_move_init->vy_min_speed = -NORMAL_MAX_CHASSIS_SPEED_Y * 2.0f / 3.0f;

  // 更新数据
  chassis_feedback_update(chassis_move_init);
}

static void chassis_set_mode(chassis_move_t *chassis_move_mode)
{
  if (chassis_move_mode == NULL)
  {
    return;
  }

  // 该函数位于"chassis_behaviour.c"文件中
  chassis_behaviour_mode_set(chassis_move_mode);
}

static void chassis_mode_change_control_transit(chassis_move_t *chassis_move_transit)
{
  if (chassis_move_transit == NULL)
  {
    return;
  }

  if (chassis_move_transit->last_chassis_mode == chassis_move_transit->chassis_mode)
  {
    return;
  }

  // 切入跟随云台模式
  if ((chassis_move_transit->last_chassis_mode != CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW) && chassis_move_transit->chassis_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW)
  {
    chassis_move_transit->chassis_relative_angle_set = 0.0f;
  }
  // 切入跟随底盘角度模式
  else if ((chassis_move_transit->last_chassis_mode != CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW) && chassis_move_transit->chassis_mode == CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW)
  {
    chassis_move_transit->chassis_yaw_set = chassis_move_transit->chassis_yaw;
  }
  // 切入不跟随云台模式
  else if ((chassis_move_transit->last_chassis_mode != CHASSIS_VECTOR_NO_FOLLOW_YAW) && chassis_move_transit->chassis_mode == CHASSIS_VECTOR_NO_FOLLOW_YAW)
  {
    chassis_move_transit->chassis_yaw_set = chassis_move_transit->chassis_yaw;
  }
  // 切入自定义模式
  else if ((chassis_move_transit->last_chassis_mode != CHASSIS_VECTOR_DEFINED) && chassis_move_transit->chassis_mode == CHASSIS_VECTOR_DEFINED)
  {
    chassis_move_transit->chassis_yaw_set = chassis_move_transit->chassis_yaw;
  }
  chassis_move_transit->last_chassis_mode = chassis_move_transit->chassis_mode;
}

static void chassis_feedback_update(chassis_move_t *chassis_move_update)
{
  if (chassis_move_update == NULL)
  {
    return;
  }

  uint8_t i = 0;
  for (i = 0; i < 4; i++)
  {
    // 更新电机速度，加速度是速度的PID微分
    chassis_move_update->motor_chassis[i].speed = CHASSIS_MOTOR_RPM_TO_VECTOR_SEN * chassis_move_update->motor_chassis[i].chassis_motor_measure->speed_rpm;
    chassis_move_update->motor_chassis[i].accel = chassis_move_update->motor_speed_pid[i].Dbuf[0] * CHASSIS_CONTROL_FREQUENCE;
  }

  // 更新底盘纵向速度x、平移速度y、旋转角速度wz，坐标系为右手系（向前为x正，向左为y正，向上为z正）
  chassis_move_update->vx = (chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed + chassis_move_update->motor_chassis[2].speed - chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_VX;
  chassis_move_update->vy = (-chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed + chassis_move_update->motor_chassis[2].speed + chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_VY;
  chassis_move_update->wz = (-chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed - chassis_move_update->motor_chassis[2].speed - chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_WZ / MOTOR_DISTANCE_TO_CENTER;

  // 计算底盘姿态角度，如果底盘上有陀螺仪请更改这部分代码
  chassis_move_update->chassis_yaw = rad_format(*(chassis_move_update->chassis_INS_angle + INS_YAW_ADDRESS_OFFSET) - chassis_move_update->chassis_yaw_motor->relative_angle);
  chassis_move_update->chassis_pitch = rad_format(*(chassis_move_update->chassis_INS_angle + INS_PITCH_ADDRESS_OFFSET) - chassis_move_update->chassis_pitch_motor->relative_angle);
  chassis_move_update->chassis_roll = *(chassis_move_update->chassis_INS_angle + INS_ROLL_ADDRESS_OFFSET);
}

void chassis_rc_to_control_vector(fp32 *vx_set, fp32 *vy_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (chassis_move_rc_to_vector == NULL || vx_set == NULL || vy_set == NULL)
  {
    return;
  }

  int16_t vx_channel, vy_channel;
  int16_t vx_key_channel, vy_key_channel;
  fp32 vx_set_channel, vy_set_channel;
  // 死区限制，因为遥控器可能存在差异，摇杆在正中间的时候，传出的值不一定为0
  rc_deadband_limit(chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_X_CHANNEL], vx_channel, CHASSIS_RC_DEADLINE);
  rc_deadband_limit(chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_Y_CHANNEL], vy_channel, CHASSIS_RC_DEADLINE);
  // 将遥控器原始数值转化为速度，单位m/s
  vx_set_channel = vx_channel * CHASSIS_VX_RC_SEN;
  vy_set_channel = vy_channel * -CHASSIS_VY_RC_SEN;

  // 键盘代替遥控器控制
  if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_FRONT_KEY)
  {
		if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_SPRINT_KEY){
			vx_set_channel = chassis_move_rc_to_vector->vx_max_speed;
		}
		else{
			vx_set_channel = chassis_move_rc_to_vector->vx_max_speed/2;
		}
  }
  else if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_BACK_KEY)
  {
    if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_SPRINT_KEY){
			vx_set_channel = -chassis_move_rc_to_vector->vx_max_speed;
		}
		else{
			vx_set_channel = -chassis_move_rc_to_vector->vx_max_speed/2;
		}
  }

  if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_LEFT_KEY)
  {
    if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_SPRINT_KEY){
			vy_set_channel = chassis_move_rc_to_vector->vy_max_speed;
		}
		else{
			vy_set_channel = chassis_move_rc_to_vector->vy_max_speed/2;
		}
  }
  else if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_RIGHT_KEY)
  {
    if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_SPRINT_KEY){
			vy_set_channel = -chassis_move_rc_to_vector->vy_max_speed;
		}
		else{
			vy_set_channel = -chassis_move_rc_to_vector->vy_max_speed/2;
		}
  }

  // 一阶低通滤波代替斜波作为底盘速度输入，滤波输出结果保存在 chassis_cmd_slow_set_vx.out 和 chassis_cmd_slow_set_vy.out
  first_order_filter_cali(&chassis_move_rc_to_vector->chassis_cmd_slow_set_vx, vx_set_channel);
  first_order_filter_cali(&chassis_move_rc_to_vector->chassis_cmd_slow_set_vy, vy_set_channel);
  // 停止信号，不需要缓慢加速，直接减速到零
  if (vx_set_channel < CHASSIS_RC_DEADLINE * CHASSIS_VX_RC_SEN && vx_set_channel > -CHASSIS_RC_DEADLINE * CHASSIS_VX_RC_SEN)
  {
    chassis_move_rc_to_vector->chassis_cmd_slow_set_vx.out = 0.0f;
  }

  if (vy_set_channel < CHASSIS_RC_DEADLINE * CHASSIS_VY_RC_SEN && vy_set_channel > -CHASSIS_RC_DEADLINE * CHASSIS_VY_RC_SEN)
  {
    chassis_move_rc_to_vector->chassis_cmd_slow_set_vy.out = 0.0f;
  }

  *vx_set = chassis_move_rc_to_vector->chassis_cmd_slow_set_vx.out; // 赋值单位为电流
  *vy_set = chassis_move_rc_to_vector->chassis_cmd_slow_set_vy.out; // 赋值单位为电流
}

static void chassis_set_contorl(chassis_move_t *chassis_move_control)
{

  if (chassis_move_control == NULL)
  {
    return;
  }
  // 单位m/s
  fp32 vx_set = 0.0f, vy_set = 0.0f, angle_set = 0.0f, gimbal_yaw_set = 0.0f, gimbal_pitch_set = 0.0f;
  // 获取三个控制设置值
  chassis_behaviour_control_set(&vx_set, &vy_set, &angle_set, chassis_move_control, &gimbal_yaw_set, &gimbal_pitch_set);
  test_angle_set = angle_set;
  // 跟随云台模式
  if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW)
  {
    fp32 sin_yaw = 0.0f, cos_yaw = 0.0f;
    // 旋转控制底盘速度方向，保证前进方向是云台方向，有利于运动平稳
    sin_yaw = arm_sin_f32(-chassis_move_control->chassis_yaw_motor->relative_angle); // 输入的角度为弧度
    cos_yaw = arm_cos_f32(-chassis_move_control->chassis_yaw_motor->relative_angle); // 输入的角度为弧度
    chassis_move_control->vx_set = cos_yaw * vx_set + sin_yaw * vy_set;
    chassis_move_control->vy_set = -sin_yaw * vx_set + cos_yaw * vy_set;
    // 设置控制相对云台角度
    chassis_move_control->chassis_relative_angle_set = rad_format(angle_set);
    // 计算旋转pid角速度
    chassis_move_control->wz_set = -PID_calc(&chassis_move_control->chassis_angle_pid, chassis_move_control->chassis_yaw_motor->relative_angle, chassis_move_control->chassis_relative_angle_set);
    // 速度限幅
    chassis_move_control->vx_set = fp32_constrain(chassis_move_control->vx_set, chassis_move_control->vx_min_speed, chassis_move_control->vx_max_speed);
    chassis_move_control->vy_set = fp32_constrain(chassis_move_control->vy_set, chassis_move_control->vy_min_speed, chassis_move_control->vy_max_speed);
  }
  // 自定义模式
  else if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_DEFINED)
  {
    // 设置底盘控制的角度
    chassis_move_control->chassis_yaw_set = rad_format(angle_set);
    // 计算旋转的角速度
    chassis_move_control->wz_set = angle_set;
    // 速度限幅
    chassis_move_control->vx_set = fp32_constrain(vx_set, chassis_move_control->vx_min_speed, chassis_move_control->vx_max_speed);
    chassis_move_control->vy_set = fp32_constrain(vy_set, chassis_move_control->vy_min_speed, chassis_move_control->vy_max_speed);
  }
  // // 跟随底盘模式
  // else if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW)
  // {
  //     fp32 delat_angle = 0.0f;
  //     // 设置底盘控制的角度
  //     chassis_move_control->chassis_yaw_set = rad_format(angle_set);
  //     delat_angle = rad_format(chassis_move_control->chassis_yaw_set - chassis_move_control->chassis_yaw);
  //     // 计算旋转的角速度
  //     chassis_move_control->wz_set = PID_calc(&chassis_move_control->chassis_angle_pid, 0.0f, delat_angle);
  //     // 速度限幅
  //     chassis_move_control->vx_set = fp32_constrain(vx_set, chassis_move_control->vx_min_speed, chassis_move_control->vx_max_speed);
  //     chassis_move_control->vy_set = fp32_constrain(vy_set, chassis_move_control->vy_min_speed, chassis_move_control->vy_max_speed);
  // }
  // 不跟随云台模式
  else if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_NO_FOLLOW_YAW)
  {
    // angle_set是旋转速度控制，单位m/s
    chassis_move_control->wz_set = angle_set;
    chassis_move_control->vx_set = fp32_constrain(vx_set, chassis_move_control->vx_min_speed, chassis_move_control->vx_max_speed);
    chassis_move_control->vy_set = fp32_constrain(vy_set, chassis_move_control->vy_min_speed, chassis_move_control->vy_max_speed);
  }

  // 原始控制模式，开环控制
  else if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_RAW)
  {
    // 在原始模式，设置值发送到CAN总线
    chassis_move_control->vx_set = vx_set;
    chassis_move_control->vy_set = vy_set;
    chassis_move_control->wz_set = angle_set;
    chassis_move_control->chassis_cmd_slow_set_vx.out = 0.0f;
    chassis_move_control->chassis_cmd_slow_set_vy.out = 0.0f;
  }
}

static void chassis_vector_to_mecanum_wheel_speed(const fp32 vx_set, const fp32 vy_set, const fp32 wz_set, fp32 wheel_speed[4])
{
  // 数组中四个轮子按顺序分别为左前、右前、左后、右后

  // 旋转的时候，由于云台靠前，所以是前面两轮0、1旋转的速度变慢，后面两轮2、3旋转的速度变快
  vy_set_tem = +vy_set * arm_cos_f32(gimbal_control_1.gimbal_yaw_motor.relative_angle - INIT_YAW_SET) + vx_set * arm_sin_f32(gimbal_control_1.gimbal_yaw_motor.relative_angle - INIT_YAW_SET);
  vx_set_tem = -vy_set * arm_sin_f32(gimbal_control_1.gimbal_yaw_motor.relative_angle - INIT_YAW_SET) + vx_set * arm_cos_f32(gimbal_control_1.gimbal_yaw_motor.relative_angle - INIT_YAW_SET);
  wheel_speed[0] = -vx_set_tem - vy_set_tem + (CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
  wheel_speed[1] = vx_set_tem - vy_set_tem + (CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
  wheel_speed[2] = vx_set_tem + vy_set_tem + (-CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
  wheel_speed[3] = -vx_set_tem + vy_set_tem + (-CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
}

static void chassis_vector_to_omnidirectional_wheel_speed(const fp32 vx_set, const fp32 vy_set, const fp32 wz_set, fp32 wheel_speed[4])
{
  // 数组中四个轮子按顺序分别为左前、右前、左后、右后

  // 计算云台和底盘的相对转角
  offset_angle = gimbal_control_1.gimbal_yaw_motor.relative_angle - INIT_YAW_SET;
  // 根据云台朝向，计算前向和侧向速度
  vx_set_tem = vx_set * arm_cos_f32(offset_angle) - vy_set * arm_sin_f32(offset_angle);
  vy_set_tem = vx_set * arm_sin_f32(offset_angle) + vy_set * arm_cos_f32(offset_angle);
  // 根据前向和侧向速度，计算四边全向轮的速度
  wheel_speed[0] = vx_set_tem - vy_set_tem +
                   (CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
  wheel_speed[1] = -vx_set_tem - vy_set_tem +
                   (CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
  wheel_speed[2] = vx_set_tem + vy_set_tem +
                   (CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
  wheel_speed[3] = -vx_set_tem + vy_set_tem +
                   (CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
}

static void chassis_control_loop(chassis_move_t *chassis_move_control_loop)
{
  fp32 max_vector = 0.0f, vector_rate = 0.0f;
  fp32 temp = 0.0f;
  fp32 wheel_speed[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  uint8_t i = 0;

  if (chassis_wheel == CHASSIS_WHEEL_MECANUM)
  {
    // 麦轮运动分解
    chassis_vector_to_mecanum_wheel_speed(chassis_move_control_loop->vx_set, chassis_move_control_loop->vy_set, chassis_move_control_loop->wz_set, wheel_speed);
  }
  else if (chassis_wheel == CHASSIS_WHEEL_OMNIDIRECTIONAL)
  {
    // 全向轮运动分解
    chassis_vector_to_omnidirectional_wheel_speed(chassis_move_control_loop->vx_set, chassis_move_control_loop->vy_set, chassis_move_control_loop->wz_set, wheel_speed);
  }

  if (chassis_move_control_loop->chassis_mode == CHASSIS_VECTOR_RAW)
  {

    for (i = 0; i < 4; i++)
    {
      chassis_move_control_loop->motor_chassis[i].give_current = (int16_t)(wheel_speed[i]);
    }
    // raw原始模式直接返回
    return;
  }

  // 计算轮子的最大控制速度，并进行限制
  for (i = 0; i < 4; i++)
  {
    chassis_move_control_loop->motor_chassis[i].speed_set = wheel_speed[i];
    temp = fabs(chassis_move_control_loop->motor_chassis[i].speed_set);
    if (max_vector < temp)
    {
      max_vector = temp;
    }
  }

  if (max_vector > MAX_WHEEL_SPEED)
  {
    vector_rate = MAX_WHEEL_SPEED / max_vector;
    for (i = 0; i < 4; i++)
    {
      chassis_move_control_loop->motor_chassis[i].speed_set *= vector_rate;
    }
  }

  // 计算pid
  for (i = 0; i < 4; i++)
  {
    PID_calc(&chassis_move_control_loop->motor_speed_pid[i], chassis_move_control_loop->motor_chassis[i].speed, chassis_move_control_loop->motor_chassis[i].speed_set);
  }

  // 功率控制
//  chassis_power_control(chassis_move_control_loop);

  // 赋值电流值
  for (i = 0; i < 4; i++)
  {
    chassis_move_control_loop->motor_chassis[i].give_current = (int16_t)(chassis_move_control_loop->motor_speed_pid[i].out);
  }
}

uint16_t get_chassis_current(void)
{
  return (uint16_t)(fabs(chassis_move.motor_chassis[0].chassis_motor_measure->given_current) + fabs(chassis_move.motor_chassis[1].chassis_motor_measure->given_current) + fabs(chassis_move.motor_chassis[2].chassis_motor_measure->given_current) + fabs(chassis_move.motor_chassis[3].chassis_motor_measure->given_current));
}
