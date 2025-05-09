/**
 * @file oled_task.c
 * @brief oled屏幕显示错误码
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

#include "oled_task.h"
#include "main.h"
#include "oled.h"
#include "cmsis_os.h"
#include "detect_task.h"
#include "voltage_task.h"
#include "chassis_task.h"
#include "referee.h"

/*-----------------------------------宏定义-----------------------------------*/

#define OLED_CONTROL_TIME 10
#define REFRESH_RATE 10

/*-----------------------------------变量声明-----------------------------------*/

const error_t *error_list_local;
uint8_t other_toe_name[4][4] = {"GYR\0", "ACC\0", "MAG\0", "REF\0"};
uint8_t last_oled_error = 0;
uint8_t now_oled_errror = 0;
static uint8_t refresh_tick = 0;
float chassis_power = 0;

/*-----------------------------------内部函数声明-----------------------------------*/

// 空

/*-----------------------------------函数实现-----------------------------------*/

void oled_task(void const *argument)
{
    uint8_t i;
    uint8_t show_col, show_row;
    // 获取错误列表
    error_list_local = get_error_list_point();
    osDelay(1000);
    OLED_init();
    OLED_LOGO();
    i = 100;
    while (i--)
    {
        if (OLED_check_ack())
        {
            detect_hook(OLED_TOE);
        }
        osDelay(10);
    }
    while (1)
    {
        // 使用i2c ack检查oled
        if (OLED_check_ack())
        {
            detect_hook(OLED_TOE);
        }

        now_oled_errror = toe_is_error(OLED_TOE);
        // 首次连接时需要初始化oled
        if (last_oled_error == 1 && now_oled_errror == 0)
        {
            OLED_init();
        }

        if (now_oled_errror == 0)
        {
            refresh_tick++;
            // 10Hz频率刷新
            if (refresh_tick > configTICK_RATE_HZ / (OLED_CONTROL_TIME * REFRESH_RATE))
            {
                refresh_tick = 0;
                OLED_operate_gram(PEN_CLEAR);
                OLED_show_graphic(0, 1, &battery_box);

                if (get_battery_percentage() < 10)
                {
                    OLED_printf(9, 2, "%d", get_battery_percentage());
                }
                else if (get_battery_percentage() < 100)
                {
                    OLED_printf(6, 2, "%d", get_battery_percentage());
                }
                else
                {
                    OLED_printf(3, 2, "%d", get_battery_percentage());
                }
                chassis_power = get_chassis_current() * Get_battery_voltage() * 20 / 16384.0f;
                char buffer[15];

                sprintf(buffer, "%.3f", power_heat_data_t.chassis_power);
                OLED_show_string(80, 2, (uint8_t *)buffer);

                OLED_show_string(90, 27, "DBUS");
                OLED_show_graphic(115, 27, &check_box[error_list_local[DBUS_TOE].error_exist]);
                for (i = CHASSIS_MOTOR1_TOE; i < TRIGGER_MOTOR_TOE + 1; i++)
                {
                    show_col = ((i - 1) * 32) % 128;
                    show_row = 15 + (i - 1) / 4 * 12;
                    OLED_show_char(show_col, show_row, 'M');
                    OLED_show_char(show_col + 6, show_row, '0' + i);
                    OLED_show_graphic(show_col + 12, show_row, &check_box[error_list_local[i].error_exist]);
                }

                for (i = BOARD_GYRO_TOE; i < REFEREE_TOE + 1; i++)
                {
                    show_col = (i * 32) % 128;
                    show_row = 15 + i / 4 * 12;
                    OLED_show_string(show_col, show_row, other_toe_name[i - BOARD_GYRO_TOE]);
                    OLED_show_graphic(show_col + 18, show_row, &check_box[error_list_local[i].error_exist]);
                }

                OLED_refresh_gram();
            }
        }

        last_oled_error = now_oled_errror;
        osDelay(OLED_CONTROL_TIME);
    }
}
