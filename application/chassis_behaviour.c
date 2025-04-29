/**
 * @file chassis_behaviour.c
 * @brief 根据遥控器的值，决定底盘行为
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

#include "chassis_behaviour.h"
#include "cmsis_os.h"
#include "chassis_task.h"
#include "arm_math.h"
#include "gimbal_behaviour.h"
#include "referee.h"

/*-----------------------------------变量声明-----------------------------------*/

// 底盘行为模式变量
chassis_behaviour_e chassis_behaviour_mode = CHASSIS_ZERO_FORCE;

float test_swing_angle;

// 小陀螺标识符，0-关闭，1-顺时针转，2-逆时针转
uint8_t rotate_flag = 0;

// ctrl键防抖
uint32_t ctrl_pressed_tick = 0;

// 小陀螺速度
float rotate_speed = 0;
extern uint32_t uwTick;
extern ext_game_robot_state_t robot_state;

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 底盘无力的行为状态机下，底盘模式是raw，故而设定值会直接发送到can总线上，故而将设定值都设为0
 */
static void chassis_zero_force_control(fp32 *vx_can_set, fp32 *vy_can_set, fp32 *wz_can_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 底盘不移动的行为状态机下，底盘模式是不跟随角度
 * @param vx_set 前后速度，正值前进，负值后退
 * @param vy_set 左右速度，正值左移，负值右移
 * @param wz_set 旋转角速度
 * @param chassis_move_rc_to_vector 底盘数据
 */
static void chassis_no_move_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 底盘跟随角度的行为状态机下，底盘模式是跟随云台角度，底盘旋转速度会根据角度查计算底盘旋转的角速度
 * @param vx_set 前后速度，正值前进，负值后退
 * @param vy_set 左右速度，正值左移，负值右移
 * @param wz_set 底盘与云台控制到的相对角度
 * @param chassis_move_rc_to_vector 底盘数据
 */
static void chassis_infantry_follow_gimbal_yaw_control(fp32 *vx_set, fp32 *vy_set, fp32 *angle_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 底盘跟随底盘yaw的行为状态机下，底盘模式是跟随底盘角度，底盘旋转速度会根据角度差计算底盘旋转的角速度
 * @param vx_set 前后速度，正值前进，负值后退
 * @param vy_set 左右速度，正值左移，负值右移
 * @param wz_set 底盘设置的yaw，范围-PI到PI
 * @param chassis_move_rc_to_vector 底盘数据
 */
static void chassis_engineer_follow_chassis_yaw_control(fp32 *vx_set, fp32 *vy_set, fp32 *angle_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 底盘不跟随角度的行为状态机下，底盘模式是不跟随角度，底盘旋转速度由参数直接设定
 * @param vx_set 前后速度，正值前进，负值后退
 * @param vy_set 左右速度，正值左移，负值右移
 * @param wz_set 底盘设置的旋转速度，正值逆时针，负值顺时针
 * @param chassis_move_rc_to_vector 底盘数据
 */
static void chassis_no_follow_yaw_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 底盘开环的行为状态机下，底盘模式是raw原生状态，故而设定值会直接发送到can总线上
 * @param vx_set 前后速度，正值前进，负值后退
 * @param vy_set 左右速度，正值左移，负值右移
 * @param wz_set 底盘设置的旋转速度，正值逆时针，负值顺时针
 * @param chassis_move_rc_to_vector 底盘数据
 */
static void chassis_open_set_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector);

/**
 * @brief 自定义模式运动控制，底盘与云台分开来进行
 */
static void chassis_my_defined_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector, fp32 *gimbal_yaw, fp32 *gimbal_pitch);

void level_to_rotate_speed(chassis_move_t *chassis_move_level, float* rotate_speed, uint8_t robot_level);

/*-----------------------------------函数实现-----------------------------------*/

void chassis_behaviour_mode_set(chassis_move_t *chassis_move_mode)
{
  if (chassis_move_mode == NULL)
  {
    return;
  }

    // 设置遥控器模式
    if (switch_is_mid(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_CHANNEL]))
    {
      // 当右边拨杆拨到中间时，切换为自定义模式（跟随云台模式）
      chassis_behaviour_mode = CHASSIS_MY_DEFINED;
    }
    else if (switch_is_down(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_CHANNEL]))
    {
      // 当右边拨杆拨到下面时，切换为底盘静止模式
      chassis_behaviour_mode = CHASSIS_NO_MOVE;
    }
    else if (switch_is_up(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_CHANNEL]))
    {
      // 当右边拨杆拨到上面时，切换为不跟随云台模式
      chassis_behaviour_mode = CHASSIS_NO_FOLLOW_YAW;
    }

  // 当云台在某些模式下（如初始化模式），底盘不动
  // if (gimbal_cmd_to_chassis_stop())
  // {
  //     chassis_behaviour_mode = CHASSIS_NO_MOVE;
  // }

  // 添加自己的逻辑判断进入新模式

  // 根据行为模式选择一个底盘控制模式
  // 底盘行为无力
  if (chassis_behaviour_mode == CHASSIS_ZERO_FORCE)
  {
    // 控制模式为遥控器值成比例发送电流（无PID）
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_RAW;
  }
  // 底盘不动，down模式
  else if (chassis_behaviour_mode == CHASSIS_NO_MOVE)
  {
    // 底盘有旋转速度控制
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_NO_FOLLOW_YAW;
  }
  // 行为模式为跟随云台
  else if (chassis_behaviour_mode == CHASSIS_INFANTRY_FOLLOW_GIMBAL_YAW)
  {
    // 控制模式为跟随云台
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW;
  }
  // 行为模式为跟随底盘
  else if (chassis_behaviour_mode == CHASSIS_ENGINEER_FOLLOW_CHASSIS_YAW)
  {
    // 底盘有底盘角度控制闭环
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_FOLLOW_CHASSIS_YAW;
  }
  // 不跟随云台，mid模式
  else if (chassis_behaviour_mode == CHASSIS_NO_FOLLOW_YAW)
  {
    // 底盘不跟随
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_NO_FOLLOW_YAW;
  }
  // 底盘开环
  else if (chassis_behaviour_mode == CHASSIS_OPEN)
  {
    // 底盘控制模式为遥控器值成比例发送电流（无PID）
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_RAW;
  }
  // 自定义模式
  else if (chassis_behaviour_mode == CHASSIS_MY_DEFINED)
  {
    // 自定义模式
    chassis_move_mode->chassis_mode = CHASSIS_VECTOR_DEFINED;
  }
}

void chassis_behaviour_control_set(fp32 *vx_set, fp32 *vy_set, fp32 *angle_set, chassis_move_t *chassis_move_rc_to_vector, fp32 *gimbal_yaw_set, fp32 *gimbal_pitch_set)
{

  if (vx_set == NULL || vy_set == NULL || angle_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }

  if (chassis_behaviour_mode == CHASSIS_ZERO_FORCE)
  {
    chassis_zero_force_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
  }
  else if (chassis_behaviour_mode == CHASSIS_NO_MOVE) // down，静止，电机锁住
  {
    chassis_no_move_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
  }
  else if (chassis_behaviour_mode == CHASSIS_INFANTRY_FOLLOW_GIMBAL_YAW)
  {
    chassis_infantry_follow_gimbal_yaw_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
  }
  else if (chassis_behaviour_mode == CHASSIS_ENGINEER_FOLLOW_CHASSIS_YAW)
  {
    chassis_engineer_follow_chassis_yaw_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
  }
  else if (chassis_behaviour_mode == CHASSIS_NO_FOLLOW_YAW) // up，不跟随云台
  {
    chassis_no_follow_yaw_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
  }
  else if (chassis_behaviour_mode == CHASSIS_OPEN)
  {
    chassis_open_set_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector);
  }
  else if (chassis_behaviour_mode == CHASSIS_MY_DEFINED) // mid，自定义模式
  {
    chassis_my_defined_control(vx_set, vy_set, angle_set, chassis_move_rc_to_vector, gimbal_yaw_set, gimbal_pitch_set);
  }
}

static void chassis_zero_force_control(fp32 *vx_can_set, fp32 *vy_can_set, fp32 *wz_can_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (vx_can_set == NULL || vy_can_set == NULL || wz_can_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }
  *vx_can_set = 0.0f;
  *vy_can_set = 0.0f;
  *wz_can_set = 0.0f;
}

static void chassis_no_move_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }
  *vx_set = 0.0f;
  *vy_set = 0.0f;
  *wz_set = 0.0f;
}

static void chassis_infantry_follow_gimbal_yaw_control(fp32 *vx_set, fp32 *vy_set, fp32 *angle_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (vx_set == NULL || vy_set == NULL || angle_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }

  // 通过遥控器的通道值以及键盘按键值得出一般情况下的速度设定值
  chassis_rc_to_control_vector(vx_set, vy_set, chassis_move_rc_to_vector);

  // 摇摆角度是利用sin函数生成的，swing_time是sin函数的输入值
  static fp32 swing_time = 0.0f;
  static fp32 swing_angle = 0.0f;
  // max_angle是sin函数的幅值
  static fp32 max_angle = SWING_NO_MOVE_ANGLE;
  // swing_time在一个控制周期内，加上add_time
  static fp32 const add_time = PI * 0.5f * configTICK_RATE_HZ / CHASSIS_CONTROL_TIME_MS;

  static uint8_t swing_flag = 0;

  // 判断是否要摇摆
  if (chassis_move_rc_to_vector->chassis_RC->key.v & SWING_KEY)
  {
    if (swing_flag == 0)
    {
      swing_flag = 1;
      swing_time = 0.0f;
    }
  }
  else
  {
    swing_flag = 0;
  }

  // 判断键盘输入是不是在控制底盘运动，如果是，则减小摇摆角度
  if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_FRONT_KEY || chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_BACK_KEY ||
      chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_LEFT_KEY || chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_RIGHT_KEY)
  {
    max_angle = SWING_MOVE_ANGLE;
  }
  else
  {
    max_angle = SWING_NO_MOVE_ANGLE;
  }

  if (swing_flag)
  {
    swing_angle = max_angle * arm_sin_f32(swing_time);
    swing_time += add_time;
    test_swing_angle = swing_angle;
  }
  else
  {
    swing_angle = 0.0f;
  }
  // swing_time取值范围[0,2pi]
  if (swing_time > 2 * PI)
  {
    swing_time -= 2 * PI;
  }

  *angle_set = swing_angle * 0.1;
}

static void chassis_engineer_follow_chassis_yaw_control(fp32 *vx_set, fp32 *vy_set, fp32 *angle_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (vx_set == NULL || vy_set == NULL || angle_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }

  chassis_rc_to_control_vector(vx_set, vy_set, chassis_move_rc_to_vector);

  *angle_set = rad_format(chassis_move_rc_to_vector->chassis_yaw_set - CHASSIS_ANGLE_Z_RC_SEN * chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_WZ_CHANNEL]);
}

static void chassis_no_follow_yaw_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }

  chassis_rc_to_control_vector(vx_set, vy_set, chassis_move_rc_to_vector);
	
	level_to_rotate_speed(chassis_move_rc_to_vector, &rotate_speed, robot_state.robot_level);
	
  *wz_set = chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_WZ_CHANNEL] * CHASSIS_WZ_RC_SEN + 
	((chassis_move_rc_to_vector->chassis_RC->key.v & KEY_PRESSED_OFFSET_Q) - (chassis_move_rc_to_vector->chassis_RC->key.v & KEY_PRESSED_OFFSET_E)) * CHASSIS_WZ_KEY_SEN;
  
	if ((uwTick - ctrl_pressed_tick) > 1000){
    if (chassis_move_rc_to_vector->chassis_RC->key.v & KEY_PRESSED_OFFSET_CTRL)
		{
      ctrl_pressed_tick = uwTick;
      if(rotate_flag){
				rotate_flag=0;
			}
			else{
				rotate_flag=1;
			}
		}
  }

  if (rotate_flag == 1)
  {
    *wz_set = rotate_speed;
  }
}

static void chassis_open_set_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
  if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }

  *vx_set = chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_X_CHANNEL] * CHASSIS_OPEN_RC_SCALE;
  *vy_set = -chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_Y_CHANNEL] * CHASSIS_OPEN_RC_SCALE;
  *wz_set = -chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_WZ_CHANNEL] * CHASSIS_OPEN_RC_SCALE;
  return;
}

static void chassis_my_defined_control(fp32 *vx_set, fp32 *vy_set, fp32 *wz_set, chassis_move_t *chassis_move_rc_to_vector, fp32 *gimbal_yaw, fp32 *gimbal_pitch)
{
  //    if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
  //    {
  //        return;
  //    }
  //
  //	chassis_rc_to_control_vector(vx_set, vy_set, chassis_move_rc_to_vector);//设定纵向横向速度
  //    *wz_set = -CHASSIS_WZ_RC_SEN * chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_WZ_CHANNEL];
  //	*gimbal_yaw= chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_YAW_CHANNEL] * YAW_RC_SEN;
  //	*gimbal_pitch= chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_PITCH_CHANNEL] * PITCH_RC_SEN;;
  //    return;
  if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
  {
    return;
  }

  chassis_rc_to_control_vector(vx_set, vy_set, chassis_move_rc_to_vector);
  *wz_set = -CHASSIS_WZ_RC_SEN * chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_WZ_CHANNEL] - YAW_MOUSE_SEN * 2000 * chassis_move_rc_to_vector->chassis_RC->mouse.x;
}

void level_to_rotate_speed(chassis_move_t *chassis_move_level, float* rotate_speed, uint8_t robot_level)
{
	switch (robot_level){
		case 1:
		case 2:
		case 3:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 0;
			}
			else
			{
				*rotate_speed = 6;
			}
			break;
		case 4:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 1;
			}
			else
			{
				*rotate_speed = 7;
			}
			break;
		case 5:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 2;
			}
			else
			{
				*rotate_speed = 7;
			}
			break;
		case 6:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 3;
			}
			else
			{
				*rotate_speed = 8;
			}
			break;
		case 7:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 4;
			}
			else
			{
				*rotate_speed = 8;
			}
			break;
		case 8:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 5;
			}
			else
			{
				*rotate_speed = 8;
			}
			break;
		case 9:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 6;
			}
			else
			{
				*rotate_speed = 8;
			}
			break;
		case 10:
			if (chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_W || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_S || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_A || chassis_move_level->chassis_RC->key.v & KEY_PRESSED_OFFSET_D)
			{
				*rotate_speed = 6;
			}
			else
			{
				*rotate_speed = 8;
			}
			break;
		default:
			break;
	}
}