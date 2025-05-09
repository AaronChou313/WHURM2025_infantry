/**
 * @file bsp_servo_pwm.c
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

#include "bsp_servo_pwm.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim8;

void servo_pwm_set(uint16_t pwm, uint8_t i)
{
    switch(i)
    {
        case 0:
        {
            __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwm);
        }break;
        case 1:
        {
            __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwm);
        }break;
        case 2:
        {
            __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, pwm);
        }break;
        case 3:
        {
            __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_3, pwm);
        }break;
    }
}
