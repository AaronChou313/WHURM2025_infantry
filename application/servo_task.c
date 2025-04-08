/**
 * @file servo_task.c
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

/*-----------------------------------头文件引用-----------------------------------*/

#include "servo_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "bsp_servo_pwm.h"
#include "remote_control.h"

/*-----------------------------------宏定义-----------------------------------*/

#define SERVO_MIN_PWM 500
#define SERVO_MAX_PWM 2500

#define PWM_DETAL_VALUE 10

#define SERVO1_ADD_PWM_KEY KEY_PRESSED_OFFSET_Z
#define SERVO2_ADD_PWM_KEY KEY_PRESSED_OFFSET_X
#define SERVO3_ADD_PWM_KEY KEY_PRESSED_OFFSET_C
#define SERVO4_ADD_PWM_KEY KEY_PRESSED_OFFSET_V

#define SERVO_MINUS_PWM_KEY KEY_PRESSED_OFFSET_SHIFT

/*-----------------------------------内部函数声明-----------------------------------*/

// 空

/*-----------------------------------变量声明-----------------------------------*/

const RC_ctrl_t *servo_rc;
const static uint16_t servo_key[4] = {SERVO1_ADD_PWM_KEY, SERVO2_ADD_PWM_KEY, SERVO3_ADD_PWM_KEY, SERVO4_ADD_PWM_KEY};
uint16_t servo_pwm[4] = {SERVO_MIN_PWM, SERVO_MIN_PWM, SERVO_MIN_PWM, SERVO_MIN_PWM};

/*-----------------------------------函数实现-----------------------------------*/

void servo_task(void const *argument)
{
    servo_rc = get_remote_control_point();

    while (1)
    {
        for (uint8_t i = 0; i < 4; i++)
        {

            if ((servo_rc->key.v & SERVO_MINUS_PWM_KEY) && (servo_rc->key.v & servo_key[i]))
            {
                servo_pwm[i] -= PWM_DETAL_VALUE;
            }
            else if (servo_rc->key.v & servo_key[i])
            {
                servo_pwm[i] += PWM_DETAL_VALUE;
            }

            // 限制pwm
            if (servo_pwm[i] < SERVO_MIN_PWM)
            {
                servo_pwm[i] = SERVO_MIN_PWM;
            }
            else if (servo_pwm[i] > SERVO_MAX_PWM)
            {
                servo_pwm[i] = SERVO_MAX_PWM;
            }

            servo_pwm_set(servo_pwm[i], i);
        }
        osDelay(10);
    }
}
