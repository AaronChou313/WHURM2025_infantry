/**
 * @file bsp_fric.c
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

#include "bsp_fric.h"
#include "main.h"
extern TIM_HandleTypeDef htim1;
void fric_off(void)
{
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, FRIC_OFF);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, FRIC_OFF);
}
void fric1_on(uint16_t cmd)
{
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, cmd);
}
void fric2_on(uint16_t cmd)
{
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, cmd);
}


