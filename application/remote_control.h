/**
 * @file referee_control.h
 * @brief 遥控器处理，遥控器是通过类似SBUS的协议传输，利用DMA传输方式节约CPU资源，利用串口空闲中断来拉起处理函数，同时提供一些掉线重启DMA串口的方法保证热插拔的稳定性。
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

/*-----------------------------------预处理-----------------------------------*/

#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

#include "struct_typedef.h"
#include "bsp_rc.h"

/*-----------------------------------宏定义-----------------------------------*/

#define SBUS_RX_BUF_NUM 36u
#define RC_FRAME_LENGTH 18u

#define RC_CH_VALUE_MIN ((uint16_t)364)
#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
#define RC_CH_VALUE_MAX ((uint16_t)1684)

// 遥控器拨杆定义
#define RC_SW_UP ((uint16_t)1)
#define RC_SW_MID ((uint16_t)3)
#define RC_SW_DOWN ((uint16_t)2)
#define switch_is_down(s) (s == RC_SW_DOWN)
#define switch_is_mid(s) (s == RC_SW_MID)
#define switch_is_up(s) (s == RC_SW_UP)
// 电脑键盘按键定义
#define KEY_PRESSED_OFFSET_W ((uint16_t)1 << 0)
#define KEY_PRESSED_OFFSET_S ((uint16_t)1 << 1)
#define KEY_PRESSED_OFFSET_A ((uint16_t)1 << 2)
#define KEY_PRESSED_OFFSET_D ((uint16_t)1 << 3)
#define KEY_PRESSED_OFFSET_SHIFT ((uint16_t)1 << 4)
#define KEY_PRESSED_OFFSET_CTRL ((uint16_t)1 << 5)
#define KEY_PRESSED_OFFSET_Q ((uint16_t)1 << 6)
#define KEY_PRESSED_OFFSET_E ((uint16_t)1 << 7)

#define KEY_PRESSED_OFFSET_R ((uint16_t)1 << 8)
#define KEY_PRESSED_OFFSET_F ((uint16_t)1 << 9)
#define KEY_PRESSED_OFFSET_G ((uint16_t)1 << 10)
#define KEY_PRESSED_OFFSET_Z ((uint16_t)1 << 11)
#define KEY_PRESSED_OFFSET_X ((uint16_t)1 << 12)
#define KEY_PRESSED_OFFSET_C ((uint16_t)1 << 13)
#define KEY_PRESSED_OFFSET_V ((uint16_t)1 << 14)
#define KEY_PRESSED_OFFSET_B ((uint16_t)1 << 15)

/*-----------------------------------数据结构定义-----------------------------------*/

typedef __packed struct
{
        __packed struct
        {
                int16_t ch[5];
                char s[2];
        } rc;
        __packed struct
        {
                int16_t x;
                int16_t y;
                int16_t z;
                uint8_t press_l;
                uint8_t press_r;
        } mouse;
        __packed struct
        {
                uint16_t v;
        } key;

} RC_ctrl_t;

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 遥控器初始化
 * @param none
 * @retval none
 */
extern void remote_control_init(void);

/**
 * @brief 获取遥控器数据指针
 * @param none
 * @retval 遥控器数据指针
 */
extern const RC_ctrl_t *get_remote_control_point(void);

/**
 * @brief 判断遥控器数据是否出错
 * @param none
 * @retval 1-出错，0-未出错
 */
extern uint8_t RC_data_is_error(void);
extern void slove_RC_lost(void);
extern void slove_data_error(void);

/**
 * @brief 通过usart1发送sbus数据，在usart3_IRQHandle中被调用
 * @param[in] sbus sbus数据，18字节
 * @retval none
 */
extern void sbus_to_usart1(uint8_t *sbus);

#endif
