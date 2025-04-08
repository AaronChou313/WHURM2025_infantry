/**
 * @file shoot.h
 * @brief 射击功能
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

#ifndef SHOOT_H
#define SHOOT_H

#include "struct_typedef.h"
#include "CAN_receive.h"
#include "gimbal_task.h"
#include "remote_control.h"
#include "user_lib.h"

/*-----------------------------------宏定义-----------------------------------*/

// 射击发射开关通道
#define SHOOT_RC_MODE_CHANNEL 1

#define SHOOT_CONTROL_TIME GIMBAL_CONTROL_TIME

#define SHOOT_FRIC_PWM_ADD_VALUE 100.0f

// 射击摩擦轮激光打开 关闭
#define SHOOT_ON_KEYBOARD KEY_PRESSED_OFFSET_R
#define SHOOT_OFF_KEYBOARD KEY_PRESSED_OFFSET_F

// 射击完成后，子弹弹出去后，判断时间，以防误触发
#define SHOOT_DONE_KEY_OFF_TIME 15
// 鼠标长按判断
#define PRESS_LONG_TIME 400
// 遥控器射击开关打下档一段时间后，连续发射子弹
#define RC_S_LONG_TIME 800
// 摩擦轮加速时间
#define UP_ADD_TIME 80
// 电机反馈码盘值范围
#define HALF_ECD_RANGE 4096
#define ECD_RANGE 8191
// 电机rmp变化成旋转速度的比例
#define MOTOR_RPM_TO_SPEED 0.00290888208665721596153948461415f
#define MOTOR_ECD_TO_ANGLE 0.000021305288720633905968306772076277f
#define FULL_COUNT 18
// 拨弹速度
#define TRIGGER_SPEED 10.0f
#define CONTINUE_TRIGGER_SPEED 10.0f // 15.0f
#define READY_TRIGGER_SPEED 3.0f

#define KEY_OFF_JUGUE_TIME 500
#define SWITCH_TRIGGER_ON 0
#define SWITCH_TRIGGER_OFF 1

// 卡弹时间及反转时间
#define BLOCK_TRIGGER_SPEED 1.0f
#define BLOCK_TIME 700
#define REVERSE_TIME 500
#define REVERSE_SPEED_LIMIT 13.0f

#define PI_FOUR 0.78539816339744830961566084581988f
#define PI_TEN 0.314f
#define PI_16 0.19625

// 拨弹轮电机pid
#define TRIGGER_ANGLE_PID_KP 800.0f
#define TRIGGER_ANGLE_PID_KI 0.5f
#define TRIGGER_ANGLE_PID_KD 0.0f

#define TRIGGER_BULLET_PID_MAX_OUT 10000.0f
#define TRIGGER_BULLET_PID_MAX_IOUT 9000.0f

#define TRIGGER_READY_PID_MAX_OUT 10000.0f
#define TRIGGER_READY_PID_MAX_IOUT 7000.0f

#define SHOOT_HEAT_REMAIN_VALUE 80

/*-----------------------------------数据结构定义-----------------------------------*/

typedef enum
{
    SHOOT_STOP = 0,        // 停止射击
    SHOOT_READY_FRIC,      // 开启摩擦轮阶段
    SHOOT_READY_BULLET,    // 摩擦轮已开启，转动拨弹轮阶段
    SHOOT_READY,           // 设计准备就绪
    SHOOT_BULLET,          // 进行单发射击
    SHOOT_CONTINUE_BULLET, // 进行多发射击
    SHOOT_DONE,            // 射击完成
} shoot_mode_e;

typedef struct
{
    shoot_mode_e shoot_mode;
    const RC_ctrl_t *shoot_rc;                  // 遥控器指针
    const motor_measure_t *shoot_motor_measure; // 编码器返回数据指针
    ramp_function_source_t fric1_ramp;          // 斜波函数结构体
    uint16_t fric_pwm1;
    ramp_function_source_t fric2_ramp; // 斜波函数结构体
    uint16_t fric_pwm2;
    pid_type_def trigger_motor_pid; // 拨弹电机pid
    fp32 trigger_speed_set;
    fp32 speed;
    fp32 speed_set;
    fp32 angle;
    fp32 set_angle;
    int16_t given_current;
    int8_t ecd_count;

    bool_t press_l;
    bool_t press_r;
    bool_t last_press_l;
    bool_t last_press_r;
    uint16_t press_l_time;
    uint16_t press_r_time;
    uint16_t rc_s_time;

    uint16_t block_time;
    uint16_t reverse_time;
    bool_t move_flag;

    bool_t key;
    uint8_t key_time;

    uint16_t heat_limit;
    uint16_t heat;
} shoot_control_t;

/*-----------------------------------外部函数声明-----------------------------------*/

// 由于射击和云台使用同一个can的id，故也将射击任务放在云台任务中执行
extern void shoot_init(void);
extern int16_t shoot_control_loop(void);

#endif
