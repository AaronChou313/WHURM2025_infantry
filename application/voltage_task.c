/**
 * @file voltage_task.c
 * @brief 24v电源电压ADC任务，获取电压并计算电量百分比。如果电源不直连开发板，请修改VOLTAGE_DROP
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

#include "voltage_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "bsp_adc.h"
#include "user_lib.h"

/*-----------------------------------宏定义-----------------------------------*/

#define FULL_BATTER_VOLTAGE 25.2f
#define LOW_BATTER_VOLTAGE 22.2f // about 20%

#define VOLTAGE_DROP 0.00f

/*-----------------------------------变量声明-----------------------------------*/

fp32 battery_voltage;
fp32 electricity_percentage;

/*-----------------------------------内部函数声明-----------------------------------*/

// 计算电量百分比
static fp32 calc_battery_percentage(float voltage);

/*-----------------------------------函数实现-----------------------------------*/

void battery_voltage_task(void const *argument)
{
    osDelay(1000);
    // 使用内部 1.2v 电压进行校准
    init_vrefint_reciprocal();
    while (1)
    {
        battery_voltage = get_battery_voltage() + VOLTAGE_DROP;
        electricity_percentage = calc_battery_percentage(battery_voltage);
        osDelay(100);
    }
}

static fp32 calc_battery_percentage(float voltage)
{
    fp32 percentage;
    fp32 voltage_2 = voltage * voltage;
    fp32 voltage_3 = voltage_2 * voltage;

    if (voltage < 19.5f)
    {
        percentage = 0.0f;
    }
    else if (voltage < 21.9f)
    {
        percentage = 0.005664f * voltage_3 - 0.3386f * voltage_2 + 6.765f * voltage - 45.17f;
    }
    else if (voltage < 25.5f)
    {
        percentage = 0.02269f * voltage_3 - 1.654f * voltage_2 + 40.34f * voltage - 328.4f;
    }
    else
    {
        percentage = 1.0f;
    }
    if (percentage < 0.0f)
    {
        percentage = 0.0f;
    }
    else if (percentage > 1.0f)
    {
        percentage = 1.0f;
    }

    // 另一套公式
    //    if(voltage < 19.5f)
    //    {
    //        percentage = 0.0f;
    //    }
    //    else if(voltage < 22.5f)
    //    {
    // //        percentage = 0.05776f * (voltage - 22.5f) * (voltage_2 - 39.0f * voltage + 383.4f) + 0.5f;
    //        percentage = 0.05021f * voltage_3 - 3.075f * voltage_2 + 62.77f * voltage - 427.02953125f;
    //    }
    //    else if(voltage < 25.5f)
    //    {
    // //        percentage = 0.01822f * (voltage - 22.5f) * (voltage_2 - 52.05f * voltage + 637.0f) + 0.5f;
    //        percentage = 0.0178f * voltage_3 - 1.292f * voltage_2 + 31.41f * voltage - 254.903125f;
    //    }
    //    else
    //    {
    //        percentage = 1.0f;
    //    }

    return percentage;
}

uint16_t get_battery_percentage(void)
{
    return (uint16_t)(electricity_percentage * 100.0f);
}

float Get_battery_voltage(void)
{
    return battery_voltage;
}
