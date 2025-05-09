/**
 * @file shoot.c
 * @brief
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

#include "shoot.h"
#include "main.h"
#include "cmsis_os.h"
#include "bsp_laser.h"
#include "bsp_fric.h"
#include "arm_math.h"
#include "user_lib.h"
#include "referee.h"
#include "CAN_receive.h"
#include "gimbal_behaviour.h"
#include "detect_task.h"
#include "pid.h"

/*-----------------------------------宏定义-----------------------------------*/

#define shoot_fric1_on(pwm) fric1_on((pwm)) // 摩擦轮1的pwm
#define shoot_fric2_on(pwm) fric2_on((pwm)) // 摩擦轮2的pwm
#define shoot_fric_off() fric_off()         // 关闭两个摩擦轮

#define shoot_laser_on() laser_on()   // 开启激光
#define shoot_laser_off() laser_off() // 关闭激光
// 微动开关IO
#define BUTTEN_TRIG_PIN HAL_GPIO_ReadPin(BUTTON_TRIG_GPIO_Port, BUTTON_TRIG_Pin)

/*-----------------------------------变量声明-----------------------------------*/

// 射击数据
shoot_control_t shoot_control;
uint8_t fric_flag = 0;
extern ext_bullet_remaining_t bullet_remaining_t; // 剩余弹药量
extern ext_game_robot_state_t robot_state;        // 机器人状态
extern uint32_t uwTick;
uint32_t repowerTick=0;	// 发射机构重新上电计时
uint8_t last_mains_power_shooter_output = 0;

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 射击状态机设置，遥控器上拨一次开启，再上拨关闭，下拨一次发射一颗弹丸，长时间下拨则连续发射，用于3min准备时间清理子弹
 * @param none
 * @retval none
 */
static void shoot_set_mode(void);

/**
 * @brief 射击数据更新
 * @param none
 * @retval none
 */
static void shoot_feedback_update(void);

/**
 * @brief 堵转倒转处理
 * @param none
 * @retval none
 */
static void trigger_motor_turn_back(void);

/**
 * @brief 射击控制，控制拨弹电机角度，完成一次发射
 * @param none
 * @retval none
 */
static void shoot_bullet_control(void);

/*-----------------------------------函数实现-----------------------------------*/

void shoot_init(void)
{
    static const fp32 Trigger_speed_pid[3] = {TRIGGER_ANGLE_PID_KP, TRIGGER_ANGLE_PID_KI, TRIGGER_ANGLE_PID_KD};
    // 初始模式为停止射击
    shoot_control.shoot_mode = SHOOT_STOP;
    // 遥控器指针
    shoot_control.shoot_rc = get_remote_control_point();
    // 电机指针设置为拨弹电机
    shoot_control.shoot_motor_measure = get_trigger_motor_measure_point();
    // 初始化PID
    PID_init(&shoot_control.trigger_motor_pid, PID_POSITION, Trigger_speed_pid, TRIGGER_READY_PID_MAX_OUT, TRIGGER_READY_PID_MAX_IOUT);
    // 更新数据
    shoot_feedback_update();
    ramp_init(&shoot_control.fric1_ramp, SHOOT_CONTROL_TIME * 0.001f, FRIC_DOWN, FRIC_OFF);
    ramp_init(&shoot_control.fric2_ramp, SHOOT_CONTROL_TIME * 0.001f, FRIC_DOWN, FRIC_OFF);
    shoot_control.fric_pwm1 = FRIC_OFF;
    shoot_control.fric_pwm2 = FRIC_OFF;
    shoot_control.ecd_count = 0;
    shoot_control.angle = shoot_control.shoot_motor_measure->ecd * MOTOR_ECD_TO_ANGLE;
    shoot_control.given_current = 0;
    shoot_control.move_flag = 0;
    shoot_control.set_angle = shoot_control.angle;
    shoot_control.speed = 0.0f;
    shoot_control.speed_set = 0.0f;
    shoot_control.key_time = 0;
}

int16_t shoot_control_loop(void)
{
    // 设置状态机
    shoot_set_mode();
    // 更新数据
    shoot_feedback_update();

    if (shoot_control.shoot_mode == SHOOT_STOP)
    {
        fric_flag = 0;
        // 设置拨弹轮的速度
        shoot_control.speed_set = 0.0f;
    }
    else if (shoot_control.shoot_mode == SHOOT_READY_FRIC)
    {
        fric_flag = 1;
        // 设置拨弹轮的速度
        shoot_control.speed_set = 0.0f;
    }
    else if (shoot_control.shoot_mode == SHOOT_READY_BULLET)
    {
        fric_flag = 2;
        if (shoot_control.key == SWITCH_TRIGGER_OFF)
        {
            // 设置拨弹轮的拨动速度，并开启堵转反转处理
            shoot_control.trigger_speed_set = READY_TRIGGER_SPEED; // 5f
            trigger_motor_turn_back();
        }
        else
        {
            shoot_control.trigger_speed_set = 0.0f;
            shoot_control.speed_set = 0.0f;
        }
        shoot_control.trigger_motor_pid.max_out = TRIGGER_READY_PID_MAX_OUT;
        shoot_control.trigger_motor_pid.max_iout = TRIGGER_READY_PID_MAX_IOUT;
    }
    else if (shoot_control.shoot_mode == SHOOT_READY)
    {
        fric_flag = 3;
        // 设置拨弹轮的速度
        shoot_control.speed_set = 0.0f;
    }
    else if (shoot_control.shoot_mode == SHOOT_BULLET)
    {
        fric_flag = 4;
        shoot_control.trigger_motor_pid.max_out = TRIGGER_BULLET_PID_MAX_OUT;
        shoot_control.trigger_motor_pid.max_iout = TRIGGER_BULLET_PID_MAX_IOUT;
        shoot_bullet_control();
    }
    else if (shoot_control.shoot_mode == SHOOT_CONTINUE_BULLET)
    {
        fric_flag = 4;
        // 设置拨弹轮的拨动速度，并开启堵转反转处理
        shoot_control.trigger_speed_set = CONTINUE_TRIGGER_SPEED;
        trigger_motor_turn_back();
    }
    else if (shoot_control.shoot_mode == SHOOT_DONE)
    {
        fric_flag = 0;
        shoot_control.speed_set = 0.0f;
    }

    if (shoot_control.shoot_mode == SHOOT_STOP)
    {
        shoot_laser_off();
        shoot_control.given_current = 0;
        // 摩擦轮需要一个一个斜波开启，不能同时直接开启，否则可能电机不转
        ramp_calc(&shoot_control.fric1_ramp, -SHOOT_FRIC_PWM_ADD_VALUE);
        ramp_calc(&shoot_control.fric2_ramp, -SHOOT_FRIC_PWM_ADD_VALUE);
    }
    else
    {
        // 激光开启
        shoot_laser_on();
        // 计算拨弹轮电机pid
        PID_calc(&shoot_control.trigger_motor_pid, shoot_control.speed, shoot_control.speed_set);
        shoot_control.given_current = (int16_t)(shoot_control.trigger_motor_pid.out);
        if (shoot_control.shoot_mode < SHOOT_READY_BULLET)
        {
            shoot_control.given_current = 0;
        }
        // 摩擦轮需要一个一个斜波开启，不能同时直接开启，否则可能电机不转
        ramp_calc(&shoot_control.fric1_ramp, SHOOT_FRIC_PWM_ADD_VALUE);
        ramp_calc(&shoot_control.fric2_ramp, SHOOT_FRIC_PWM_ADD_VALUE);
    }

    shoot_control.fric_pwm1 = (uint16_t)(shoot_control.fric1_ramp.out);
    shoot_control.fric_pwm2 = (uint16_t)(shoot_control.fric2_ramp.out);
    shoot_fric1_on(shoot_control.fric_pwm1);
    shoot_fric2_on(shoot_control.fric_pwm2);
    return shoot_control.given_current;
}

static void shoot_set_mode(void)
{
    // 静态变量，初始化声明后不会清除，也不会再初始化
    static int8_t last_s = RC_SW_UP;

    // 上拨判断，一次开启，再次关闭
    if ((switch_is_up(shoot_control.shoot_rc->rc.s[SHOOT_RC_MODE_CHANNEL]) && !switch_is_up(last_s) && shoot_control.shoot_mode == SHOOT_STOP))
    {
        shoot_control.shoot_mode = SHOOT_READY_FRIC;
    }
    else if ((switch_is_up(shoot_control.shoot_rc->rc.s[SHOOT_RC_MODE_CHANNEL]) && !switch_is_up(last_s) && shoot_control.shoot_mode != SHOOT_STOP))
    {
        shoot_control.shoot_mode = SHOOT_STOP;
    }

    // 使用键盘开启摩擦轮
    if ((shoot_control.shoot_rc->key.v & SHOOT_ON_KEYBOARD) && shoot_control.shoot_mode == SHOOT_STOP)
    {
        shoot_control.shoot_mode = SHOOT_READY_FRIC;
    }
    // 使用键盘关闭摩擦轮
    else if ((shoot_control.shoot_rc->key.v & SHOOT_OFF_KEYBOARD) && shoot_control.shoot_mode != SHOOT_STOP)
    {
        shoot_control.shoot_mode = SHOOT_STOP;
    }

    // 如果当前尝试开启摩擦轮，且两个摩擦轮速度已达最大值，可认为摩擦轮开启完成，进入准备拨弹轮的状态
    if (shoot_control.shoot_mode == SHOOT_READY_FRIC && shoot_control.fric1_ramp.out == shoot_control.fric1_ramp.max_value && shoot_control.fric2_ramp.out == shoot_control.fric2_ramp.max_value)
    {
        shoot_control.shoot_mode = SHOOT_READY_BULLET;
    }
    // 如果当前正在准备拨弹轮，且弹丸被送入炮管触发微动开关，可认为送弹完成，可以发射
    else if (shoot_control.shoot_mode == SHOOT_READY_BULLET && shoot_control.key == SWITCH_TRIGGER_ON)
    {
        shoot_control.shoot_mode = SHOOT_READY;
    }
    // 如果当前可以发射，而微动开关又失活，可认为弹丸已发射，需要补弹，于是重新进入拨弹轮状态
    else if (shoot_control.shoot_mode == SHOOT_READY && shoot_control.key == SWITCH_TRIGGER_OFF)
    {
        shoot_control.shoot_mode = SHOOT_READY_BULLET;
    }
    // 如果为正常的可发射状态，判断遥控器或鼠标控制逻辑
    else if (shoot_control.shoot_mode == SHOOT_READY)
    {
        // 下拨一次或者鼠标按下一次，进入单发射击状态
        if ((switch_is_down(shoot_control.shoot_rc->rc.s[SHOOT_RC_MODE_CHANNEL]) && !switch_is_down(last_s)) || (shoot_control.press_l && shoot_control.last_press_l == 0))
        {
            shoot_control.shoot_mode = SHOOT_BULLET;
        }
    }
    // 如果处于射击状态，而微动开关失活，会在feedback_update中切换为射击完毕状态，此时如果微动开关仍然失活，则记录射击完毕时间，经过一定时间后再切入补弹状态，防止误触发；而如果在此期间微动开关又激活，直接返回单发射击状态
    else if (shoot_control.shoot_mode == SHOOT_DONE)
    {
        if (shoot_control.key == SWITCH_TRIGGER_OFF)
        {
            shoot_control.key_time++;
            if (shoot_control.key_time > SHOOT_DONE_KEY_OFF_TIME)
            {
                shoot_control.key_time = 0;
                shoot_control.shoot_mode = SHOOT_READY_BULLET;
            }
        }
        else
        {
            shoot_control.key_time = 0;
            shoot_control.shoot_mode = SHOOT_BULLET;
        }
    }

    // 如果为摩擦轮启动完毕后的任意状态
    if (shoot_control.shoot_mode > SHOOT_READY_FRIC)
    {
        // 鼠标长按一直进入射击状态，保持连发
        if ((shoot_control.press_l_time == PRESS_LONG_TIME) || (shoot_control.press_r_time == PRESS_LONG_TIME) || (shoot_control.rc_s_time == RC_S_LONG_TIME))
        {
            shoot_control.shoot_mode = SHOOT_CONTINUE_BULLET;
        }
        // 如果没有长按，且当前已经是连发状态，停止发射，切换为拨弹轮状态
        else if (shoot_control.shoot_mode == SHOOT_CONTINUE_BULLET)
        {
            shoot_control.shoot_mode = SHOOT_READY_BULLET;
        }
    }

    get_shoot_heat0_limit_and_heat0(&shoot_control.heat_limit, &shoot_control.heat);
    // 裁判系统温度过高
    if (!toe_is_error(REFEREE_TOE) && (shoot_control.heat + SHOOT_HEAT_REMAIN_VALUE > shoot_control.heat_limit))
    {
        if (shoot_control.shoot_mode == SHOOT_BULLET || shoot_control.shoot_mode == SHOOT_CONTINUE_BULLET)
        {
            shoot_control.shoot_mode = SHOOT_READY_BULLET;
        }
    }

    last_s = shoot_control.shoot_rc->rc.s[SHOOT_RC_MODE_CHANNEL];

    // if (bullet_remaining_t.projectile_allowance_17mm <= 5)
    // {
	// 		shoot_control.shoot_mode = SHOOT_STOP;
    // }
		
//		if(!robot_state.mains_power_shooter_output)
//		{
//			shoot_control.shoot_mode = SHOOT_STOP;
//    }
//		
//		if(last_mains_power_shooter_output==0&&robot_state.mains_power_shooter_output==1)
//		{
//			repowerTick = uwTick;
//		}
//		if(uwTick-repowerTick<5000)
//		{
//			shoot_control.shoot_mode = SHOOT_STOP;
//		}
//		last_mains_power_shooter_output = robot_state.mains_power_shooter_output;
}

static void shoot_feedback_update(void)
{

    static fp32 speed_fliter_1 = 0.0f;
    static fp32 speed_fliter_2 = 0.0f;
    static fp32 speed_fliter_3 = 0.0f;

    // 拨弹轮电机速度滤波一下
    static const fp32 fliter_num[3] = {1.725709860247969f, -0.75594777109163436f, 0.030237910843665373f};

    // 二阶低通滤波
    speed_fliter_1 = speed_fliter_2;
    speed_fliter_2 = speed_fliter_3;
    speed_fliter_3 = speed_fliter_2 * fliter_num[0] + speed_fliter_1 * fliter_num[1] + (shoot_control.shoot_motor_measure->speed_rpm * MOTOR_RPM_TO_SPEED) * fliter_num[2];
    shoot_control.speed = speed_fliter_3;

    // 电机圈数重置，因为输出轴旋转一圈，电机轴旋转36圈，将电机轴数据处理成输出轴数据，用于控制输出轴角度
    if (shoot_control.shoot_motor_measure->ecd - shoot_control.shoot_motor_measure->last_ecd > HALF_ECD_RANGE)
    {
        shoot_control.ecd_count--;
    }
    else if (shoot_control.shoot_motor_measure->ecd - shoot_control.shoot_motor_measure->last_ecd < -HALF_ECD_RANGE)
    {
        shoot_control.ecd_count++;
    }

    if (shoot_control.ecd_count == FULL_COUNT)
    {
        shoot_control.ecd_count = -(FULL_COUNT - 1);
    }
    else if (shoot_control.ecd_count == -FULL_COUNT)
    {
        shoot_control.ecd_count = FULL_COUNT - 1;
    }

    // 计算输出轴角度
    shoot_control.angle = (shoot_control.ecd_count * ECD_RANGE + shoot_control.shoot_motor_measure->ecd) * MOTOR_ECD_TO_ANGLE;

    // 微动开关
    shoot_control.key = BUTTEN_TRIG_PIN;

    // 鼠标按键
    shoot_control.last_press_l = shoot_control.press_l;
    shoot_control.last_press_r = shoot_control.press_r;
    shoot_control.press_l = shoot_control.shoot_rc->mouse.press_l;
    shoot_control.press_r = shoot_control.shoot_rc->mouse.press_r;
    // 长按计时
    if (shoot_control.press_l)
    {
        if (shoot_control.press_l_time < PRESS_LONG_TIME)
        {
            shoot_control.press_l_time++;
        }
    }
    else
    {
        shoot_control.press_l_time = 0;
    }

    // if (shoot_control.press_r)
    // {
    //     if (shoot_control.press_r_time < PRESS_LONG_TIME)
    //     {
    //         shoot_control.press_r_time++;
    //     }
    // }
    // else
    // {
    //     shoot_control.press_r_time = 0;
    // }

    // 射击开关下档计时
    if (shoot_control.shoot_mode != SHOOT_STOP && switch_is_down(shoot_control.shoot_rc->rc.s[SHOOT_RC_MODE_CHANNEL]))
    {

        if (shoot_control.rc_s_time < RC_S_LONG_TIME)
        {
            shoot_control.rc_s_time++;
        }
    }
    else
    {
        shoot_control.rc_s_time = 0;
    }

    // 鼠标右键按下加速摩擦轮，使得左键低速射击，右键高速射击
    static uint16_t up_time = 0;
    // if (shoot_control.press_r)
    // {
    //     up_time = UP_ADD_TIME;
    // }

    if (up_time > 0)
    {
        shoot_control.fric1_ramp.max_value = FRIC_UP;
        shoot_control.fric2_ramp.max_value = FRIC_UP;
        up_time--;
    }
    else
    {
        shoot_control.fric1_ramp.max_value = FRIC_DOWN;
        shoot_control.fric2_ramp.max_value = FRIC_DOWN;
    }
}

static void trigger_motor_turn_back(void)
{
    if (shoot_control.block_time < BLOCK_TIME)
    {
        shoot_control.speed_set = shoot_control.trigger_speed_set;
    }
    else
    {
        shoot_control.speed_set = -shoot_control.trigger_speed_set;
    }

    if (fabs(shoot_control.speed) < BLOCK_TRIGGER_SPEED && shoot_control.block_time < BLOCK_TIME)
    {
        shoot_control.block_time++;
        shoot_control.reverse_time = 0;
    }
    else if (shoot_control.block_time == BLOCK_TIME && shoot_control.reverse_time < REVERSE_TIME)
    {
        shoot_control.reverse_time++;
    }
    else
    {
        shoot_control.block_time = 0;
    }
}

static void shoot_bullet_control(void)
{
    // 每次拨动1/10 PI的角度
    if (shoot_control.move_flag == 0)
    {
        shoot_control.set_angle = rad_format(shoot_control.angle + PI_TEN);
        shoot_control.move_flag = 1;
    }
    // 如果微动开关关闭，表明炮口处无弹丸，认为发射完毕，进入发射完毕状态
    if (shoot_control.key == SWITCH_TRIGGER_OFF)
    {
        shoot_control.shoot_mode = SHOOT_DONE;
    }
    // 到达角度判断
    if (rad_format(shoot_control.set_angle - shoot_control.angle) > 0.05f)
    {
        // 没达到一直设置旋转速度
        shoot_control.trigger_speed_set = TRIGGER_SPEED;
        trigger_motor_turn_back();
    }
    else
    {
        shoot_control.move_flag = 0;
    }
}
