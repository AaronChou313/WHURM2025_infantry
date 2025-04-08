/**
 * @file AHRS_middleware.c
 * @brief 姿态解算中间层
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

#include "AHRS_MiddleWare.h"
#include "AHRS.h"
#include "arm_math.h"
#include "main.h"

/**
 * @brief          获取当前高度
 * @author         RM
 * @param[in]      high 高度的指针，fp32
 * @retval         空
 */
void AHRS_get_height(fp32* high)
{
    if (high != NULL)
    {
        *high = 0.0f;
    }
}

/**
 * @brief          获取当前纬度
 * @author         RM
 * @param[in]      latitude 纬度的指针，fp32
 * @retval         空
 */
void AHRS_get_latitude(fp32* latitude)
{
    if (latitude != NULL)
    {
        *latitude = 22.0f;
    }
}

/**
 * @brief          快速开方函数
 * @author         RM
 * @param[in]      num 需要开方的浮点数，fp32
 * @retval         返回1/sqrt，即开方后的倒数
 */
fp32 AHRS_invSqrt(fp32 num)
{
    return 1/sqrtf(num);

//    fp32 halfnum = 0.5f * num;
//    fp32 y = num;
//    long i = *(long*)&y;
//    i = 0x5f3759df - (i >> 1);
//    y = *(fp32*)&i;
//    y = y * (1.5f - (halfnum * y * y));
//    y = y * (1.5f - (halfnum * y * y));
//    return y;
}

/**
 * @brief          sin函数
 * @author         RM
 * @param[in]      angle 角度，单位rad
 * @retval         返回对应角度的sin值
 */
fp32 AHRS_sinf(fp32 angle)
{
    return arm_sin_f32(angle);
}

/**
 * @brief          cos函数
 * @author         RM
 * @param[in]      angle 角度，单位rad
 * @retval         返回对应角度的cos值
 */
fp32 AHRS_cosf(fp32 angle)
{
    return arm_cos_f32(angle);
}

/**
 * @brief          tan函数
 * @author         RM
 * @param[in]      angle 角度，单位rad
 * @retval         返回对应角度的tan值
 */
fp32 AHRS_tanf(fp32 angle)
{
    return tanf(angle);
}

/**
 * @brief          arcsin函数
 * @author         RM
 * @param[in]      sin 输入sin值，最大1.0f，最小-1.0f
 * @retval         返回角度，单位rad
 */
fp32 AHRS_asinf(fp32 sin)
{

    return asinf(sin);
}

/**
 * @brief          arccos函数
 * @author         RM
 * @param[in]      cos 输入cos值，最大1.0f，最小-1.0f
 * @retval         返回角度，单位rad
 */
fp32 AHRS_acosf(fp32 cos)
{

    return acosf(cos);
}

/**
 * @brief          arctan函数
 * @author         RM
 * @param[in]      y 输入tan值中的y值，最大正无穷，最小负无穷
 * @param[in]      x 输入tan值中的x值，最大正无穷，最小负无穷
 * @retval         返回角度，单位rad
 */
fp32 AHRS_atan2f(fp32 y, fp32 x)
{
    return atan2f(y, x);
}
