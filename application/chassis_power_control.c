/**
 * @file chassis_power_control.c
 * @brief 底盘功率控制，只控制80w的功率，主要限制电机电流设定值，如果功率限制是40w，就降低JUDGE_TOTAL_CURRENT_LIMIT、POWER_CURRENT_LIMIT和底盘最大速度
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

#include "chassis_power_control.h"
#include "referee.h"
#include "arm_math.h"
#include "detect_task.h"

/*-----------------------------------宏定义-----------------------------------*/

#define POWER_LIMIT 80.0f
#define WARNING_POWER 40.0f
#define WARNING_POWER_BUFF 50.0f

#define NO_JUDGE_TOTAL_CURRENT_LIMIT 64000.0f // 16000 * 4,
#define BUFFER_TOTAL_CURRENT_LIMIT 16000.0f
#define POWER_TOTAL_CURRENT_LIMIT 20000.0f

/*-----------------------------------变量声明-----------------------------------*/

fp32 current_scale = 0;

/*-----------------------------------内部函数声明-----------------------------------*/

// 空

/*-----------------------------------函数实现-----------------------------------*/

void chassis_power_control(chassis_move_t *chassis_power_control)
{
    fp32 chassis_power = 0.0f;
    fp32 chassis_power_buffer = 0.0f;
    fp32 total_current_limit = 0.0f;
    fp32 total_current = 0.0f;
    uint8_t robot_id = get_robot_id();
    if (toe_is_error(REFEREE_TOE))
    {
        total_current_limit = NO_JUDGE_TOTAL_CURRENT_LIMIT;
    }
    else if (robot_id == RED_ENGINEER || robot_id == BLUE_ENGINEER || robot_id == 0)
    {
        total_current_limit = NO_JUDGE_TOTAL_CURRENT_LIMIT;
    }
    else
    {
        get_chassis_power_and_buffer(&chassis_power, &chassis_power_buffer);
        /*缓冲能量占比环-总体约束*/
        if (chassis_power_buffer < 60 && chassis_power_buffer >= 50)
            current_scale = 0.95; // 15
        else if (chassis_power_buffer < 50 && chassis_power_buffer >= 40)
            current_scale = 0.9;
        else if (chassis_power_buffer < 40 && chassis_power_buffer >= 35)
            current_scale = 0.75;
        else if (chassis_power_buffer < 35 && chassis_power_buffer >= 30)
            current_scale = 0.5;
        else if (chassis_power_buffer < 30 && chassis_power_buffer >= 20)
            current_scale = 0.25;
        else if (chassis_power_buffer < 20 && chassis_power_buffer >= 10)
            current_scale = 0.125;
        else if (chassis_power_buffer < 10 && chassis_power_buffer >= 0)
            current_scale = 0.05;
        else if (chassis_power_buffer == 60)
            current_scale = 1;
    }
    chassis_power_control->motor_speed_pid[0].out *= current_scale;
    chassis_power_control->motor_speed_pid[1].out *= current_scale;
    chassis_power_control->motor_speed_pid[2].out *= current_scale;
    chassis_power_control->motor_speed_pid[3].out *= current_scale;
}
