/**
 * @file bsp_rng.c
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

#include "bsp_rng.h"
#include "main.h"

extern RNG_HandleTypeDef hrng;

uint32_t RNG_get_random_num(void)
{
    static uint32_t rng;
    HAL_RNG_GenerateRandomNumber(&hrng, &rng);
    return rng;
}

int32_t RNG_get_random_rangle(int min, int max)
{
    static int32_t random;
    random = (RNG_get_random_num() % (max - min + 1)) + min;
    return random;
}



