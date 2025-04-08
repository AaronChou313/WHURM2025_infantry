/**
 * @file test_task.c
 * @brief 蜂鸣器报警任务
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

#include "test_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "bsp_buzzer.h"
#include "detect_task.h"

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 测试任务
 * @param[in] pvParameters NULL
 * @retval none
 */
void test_task(void const *argument);

/**
 * @brief 蜂鸣器响
 * @param[in] num 响声次数
 * @retval none
 */
static void buzzer_warn_error(uint8_t num);

/*-----------------------------------变量声明-----------------------------------*/

const error_t *error_list_test_local;

/*-----------------------------------函数实现-----------------------------------*/

void test_task(void const *argument)
{
    static uint8_t error, last_error;
    static uint8_t error_num;
    error_list_test_local = get_error_list_point();

    while (1)
    {
        error = 0;

        // 发现错误
        for (error_num = 0; error_num < REFEREE_TOE; error_num++)
        {
            if (error_list_test_local[error_num].error_exist)
            {
                error = 1;
                break;
            }
        }

        // 没有错误，停止蜂鸣器
        if (error == 0 && last_error != 0)
        {
            buzzer_off();
        }
        // 有错误，蜂鸣器响
        if (error)
        {
            buzzer_warn_error(error_num + 1);
        }

        last_error = error;
        osDelay(10);
    }
}

static void buzzer_warn_error(uint8_t num)
{
    static uint8_t show_num = 0;
    static uint8_t stop_num = 100;
    if (show_num == 0 && stop_num == 0)
    {
        show_num = num;
        stop_num = 100;
    }
    else if (show_num == 0)
    {
        stop_num--;
        buzzer_off();
    }
    else
    {
        static uint8_t tick = 0;
        tick++;
        if (tick < 50)
        {
            buzzer_off();
        }
        else if (tick < 100)
        {
            buzzer_on(1, 0);
        }
        else
        {
            tick = 0;
            show_num--;
        }
    }
}
