/**
 * @file detect_task.c
 * @brief 检测错误任务，通过接收数据时间来判断，提供检测钩子函数、错误存在函数
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

#include "detect_task.h"
#include "cmsis_os.h"

/*-----------------------------------变量声明-----------------------------------*/

error_t error_list[ERROR_LIST_LENGTH + 1];

#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t detect_task_stack;
#endif

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 初始化error_list，赋值offline_time，online_time，priority
 * @param[in] time 系统时间
 * @retval none
 */
static void detect_init(uint32_t time);

extern void OLED_com_reset(void);

/*-----------------------------------函数实现-----------------------------------*/

void detect_task(void const *pvParameters)
{
    static uint32_t system_time;
    system_time = xTaskGetTickCount();
    // 初始化
    detect_init(system_time);
    // 空闲一段时间
    vTaskDelay(DETECT_TASK_INIT_TIME);

    while (1)
    {
        static uint8_t error_num_display = 0;
        system_time = xTaskGetTickCount();

        error_num_display = ERROR_LIST_LENGTH;
        error_list[ERROR_LIST_LENGTH].is_lost = 0;
        error_list[ERROR_LIST_LENGTH].error_exist = 0;

        for (int i = 0; i < ERROR_LIST_LENGTH; i++)
        {
            // 未使能，跳过
            if (error_list[i].enable == 0)
            {
                continue;
            }

            // 判断掉线
            if (system_time - error_list[i].new_time > error_list[i].set_offline_time)
            {
                if (error_list[i].error_exist == 0)
                {
                    // 记录错误以及掉线时间
                    error_list[i].is_lost = 1;
                    error_list[i].error_exist = 1;
                    error_list[i].lost_time = system_time;
                }
                // 判断错误优先级，保存优先级最高的错误码
                if (error_list[i].priority > error_list[error_num_display].priority)
                {
                    error_num_display = i;
                }

                error_list[ERROR_LIST_LENGTH].is_lost = 1;
                error_list[ERROR_LIST_LENGTH].error_exist = 1;
                // 如果提供解决函数，运行解决函数
                if (error_list[i].solve_lost_fun != NULL)
                {
                    error_list[i].solve_lost_fun();
                }
            }
            else if (system_time - error_list[i].work_time < error_list[i].set_online_time)
            {
                // 刚刚上线，可能存在数据不稳定，只记录不丢失
                error_list[i].is_lost = 0;
                error_list[i].error_exist = 1;
            }
            else
            {
                error_list[i].is_lost = 0;
                // 判断是否存在数据错误
                if (error_list[i].data_is_error != NULL)
                {
                    error_list[i].error_exist = 1;
                }
                else
                {
                    error_list[i].error_exist = 0;
                }
                // 计算频率
                if (error_list[i].new_time > error_list[i].last_time)
                {
                    error_list[i].frequency = configTICK_RATE_HZ / (fp32)(error_list[i].new_time - error_list[i].last_time);
                }
            }
        }

        vTaskDelay(DETECT_CONTROL_TIME);
#if INCLUDE_uxTaskGetStackHighWaterMark
        detect_task_stack = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
}

bool_t toe_is_error(uint8_t toe)
{
    return (error_list[toe].error_exist == 1);
}

void detect_hook(uint8_t toe)
{
    error_list[toe].last_time = error_list[toe].new_time;
    error_list[toe].new_time = xTaskGetTickCount();

    if (error_list[toe].is_lost)
    {
        error_list[toe].is_lost = 0;
        error_list[toe].work_time = error_list[toe].new_time;
    }

    if (error_list[toe].data_is_error_fun != NULL)
    {
        if (error_list[toe].data_is_error_fun())
        {
            error_list[toe].error_exist = 1;
            error_list[toe].data_is_error = 1;

            if (error_list[toe].solve_data_error_fun != NULL)
            {
                error_list[toe].solve_data_error_fun();
            }
        }
        else
        {
            error_list[toe].data_is_error = 0;
        }
    }
    else
    {
        error_list[toe].data_is_error = 0;
    }
}

const error_t *get_error_list_point(void)
{
    return error_list;
}

static void detect_init(uint32_t time)
{
    // 设置离线时间，上线稳定工作时间，优先级为offlineTime -> onlineTime -> priority
    uint16_t set_item[ERROR_LIST_LENGTH][3] =
        {
            {30, 40, 15},  // SBUS
            {10, 10, 11},  // motor1
            {10, 10, 10},  // motor2
            {10, 10, 9},   // motor3
            {10, 10, 8},   // motor4
            {2, 3, 14},    // yaw
            {2, 3, 13},    // pitch
            {10, 10, 12},  // trigger
            {2, 3, 7},     // board gyro
            {5, 5, 7},     // board accel
            {40, 200, 7},  // board mag
            {100, 100, 5}, // referee
            {10, 10, 7},   // rm imu
            {100, 100, 1}, // oled
        };

    for (uint8_t i = 0; i < ERROR_LIST_LENGTH; i++)
    {
        error_list[i].set_offline_time = set_item[i][0];
        error_list[i].set_online_time = set_item[i][1];
        error_list[i].priority = set_item[i][2];
        error_list[i].data_is_error_fun = NULL;
        error_list[i].solve_lost_fun = NULL;
        error_list[i].solve_data_error_fun = NULL;

        error_list[i].enable = 1;
        error_list[i].error_exist = 1;
        error_list[i].is_lost = 1;
        error_list[i].data_is_error = 1;
        error_list[i].frequency = 0.0f;
        error_list[i].new_time = time;
        error_list[i].last_time = time;
        error_list[i].lost_time = time;
        error_list[i].work_time = time;
    }

    error_list[OLED_TOE].data_is_error_fun = NULL;
    error_list[OLED_TOE].solve_lost_fun = OLED_com_reset;
    error_list[OLED_TOE].solve_data_error_fun = NULL;

    //    error_list[DBUSTOE].dataIsErrorFun = RC_data_is_error;
    //    error_list[DBUSTOE].solveLostFun = slove_RC_lost;
    //    error_list[DBUSTOE].solveDataErrorFun = slove_data_error;
}
