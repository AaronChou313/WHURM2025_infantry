/**
 * @file referee_control.c
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

/*-----------------------------------头文件引用-----------------------------------*/

#include "remote_control.h"
#include "main.h"
#include "bsp_usart.h"
#include "string.h"
#include "detect_task.h"

/*-----------------------------------宏定义-----------------------------------*/

// 遥控器出错数据上限
#define RC_CHANNAL_ERROR_VALUE 700

/*-----------------------------------变量声明-----------------------------------*/

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

// 遥控器控制变量
RC_ctrl_t rc_ctrl;
// 接收原始数据，为18个字节，给了36个字节长度，防止DMA传输越界
static uint8_t sbus_rx_buf[2][SBUS_RX_BUF_NUM];

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 取正函数
 * @param[in] value 输入值
 * @retval 取正返回值
 */
static int16_t RC_abs(int16_t value);

/**
 * @brief 遥控器协议解析
 * @param[in] sbus_buf 原生数据指针
 * @param[out] rc_ctrl 遥控器数据指针
 * @retval none
 */
static void sbus_to_rc(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl);

/*-----------------------------------函数实现-----------------------------------*/

void remote_control_init(void)
{
    RC_Init(sbus_rx_buf[0], sbus_rx_buf[1], SBUS_RX_BUF_NUM);
}

const RC_ctrl_t *get_remote_control_point(void)
{
    return &rc_ctrl;
}

uint8_t RC_data_is_error(void)
{
    // 使用了goto语句，方便出错统一处理遥控器变量数据归零
    if (RC_abs(rc_ctrl.rc.ch[0]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (RC_abs(rc_ctrl.rc.ch[1]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (RC_abs(rc_ctrl.rc.ch[2]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (RC_abs(rc_ctrl.rc.ch[3]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (rc_ctrl.rc.s[0] == 0)
    {
        goto error;
    }
    if (rc_ctrl.rc.s[1] == 0)
    {
        goto error;
    }
    return 0;

error:
    rc_ctrl.rc.ch[0] = 0;
    rc_ctrl.rc.ch[1] = 0;
    rc_ctrl.rc.ch[2] = 0;
    rc_ctrl.rc.ch[3] = 0;
    rc_ctrl.rc.ch[4] = 0;
    rc_ctrl.rc.s[0] = RC_SW_DOWN;
    rc_ctrl.rc.s[1] = RC_SW_DOWN;
    rc_ctrl.mouse.x = 0;
    rc_ctrl.mouse.y = 0;
    rc_ctrl.mouse.z = 0;
    rc_ctrl.mouse.press_l = 0;
    rc_ctrl.mouse.press_r = 0;
    rc_ctrl.key.v = 0;
    return 1;
}

void slove_RC_lost(void)
{
    RC_restart(SBUS_RX_BUF_NUM);
}

void slove_data_error(void)
{
    RC_restart(SBUS_RX_BUF_NUM);
}

// 串口中断
void USART3_IRQHandler(void)
{
    // 接收到数据
    if (huart3.Instance->SR & UART_FLAG_RXNE)
    {
        __HAL_UART_CLEAR_PEFLAG(&huart3);
    }
    else if (USART3->SR & UART_FLAG_IDLE)
    {
        static uint16_t this_time_rx_len = 0;

        __HAL_UART_CLEAR_PEFLAG(&huart3);

        if ((hdma_usart3_rx.Instance->CR & DMA_SxCR_CT) == RESET)
        {
            /* Current memory buffer used is Memory 0 */

            // 失效DMA
            __HAL_DMA_DISABLE(&hdma_usart3_rx);

            // 获取接收数据长度（长度 = 设定长度 - 剩余长度）
            this_time_rx_len = SBUS_RX_BUF_NUM - hdma_usart3_rx.Instance->NDTR;

            // 重新设定数据长度
            hdma_usart3_rx.Instance->NDTR = SBUS_RX_BUF_NUM;

            // 设定缓冲区1
            hdma_usart3_rx.Instance->CR |= DMA_SxCR_CT;

            // 使能DMA
            __HAL_DMA_ENABLE(&hdma_usart3_rx);

            if (this_time_rx_len == RC_FRAME_LENGTH)
            {
                sbus_to_rc(sbus_rx_buf[0], &rc_ctrl);
                // 记录数据接收时间
                detect_hook(DBUS_TOE);
                sbus_to_usart1(sbus_rx_buf[0]);
            }
        }
        else
        {
            /* Current memory buffer used is Memory 1 */
            // 失效DMA
            __HAL_DMA_DISABLE(&hdma_usart3_rx);

            // 接受获取数据长度（长度 = 设定长度 - 剩余长度）
            this_time_rx_len = SBUS_RX_BUF_NUM - hdma_usart3_rx.Instance->NDTR;

            // 重新设定数据长度
            hdma_usart3_rx.Instance->NDTR = SBUS_RX_BUF_NUM;

            // 设定缓冲区1
            DMA1_Stream1->CR &= ~(DMA_SxCR_CT);

            // 使能DMA
            __HAL_DMA_ENABLE(&hdma_usart3_rx);

            if (this_time_rx_len == RC_FRAME_LENGTH)
            {
                // 处理遥控器数据
                sbus_to_rc(sbus_rx_buf[1], &rc_ctrl);
                // 记录数据接收时间
                detect_hook(DBUS_TOE);
                sbus_to_usart1(sbus_rx_buf[1]);
            }
        }
    }
}

// 取正函数
static int16_t RC_abs(int16_t value)
{
    if (value > 0)
    {
        return value;
    }
    else
    {
        return -value;
    }
}

static void sbus_to_rc(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl)
{
    if (sbus_buf == NULL || rc_ctrl == NULL)
    {
        return;
    }

    rc_ctrl->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff;        //!< Channel 0
    rc_ctrl->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff; //!< Channel 1
    rc_ctrl->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) |          //!< Channel 2
                         (sbus_buf[4] << 10)) &
                        0x07ff;
    rc_ctrl->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff; //!< Channel 3
    rc_ctrl->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003);                       //!< Switch left
    rc_ctrl->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2;                  //!< Switch right
    rc_ctrl->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8);                    //!< Mouse X axis
    rc_ctrl->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8);                    //!< Mouse Y axis
    rc_ctrl->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8);                  //!< Mouse Z axis
    rc_ctrl->mouse.press_l = sbus_buf[12];                                  //!< Mouse Left Is Press ?
    rc_ctrl->mouse.press_r = sbus_buf[13];                                  //!< Mouse Right Is Press ?
    rc_ctrl->key.v = sbus_buf[14] | (sbus_buf[15] << 8);                    //!< KeyBoard value
    rc_ctrl->rc.ch[4] = sbus_buf[16] | (sbus_buf[17] << 8);                 // NULL

    rc_ctrl->rc.ch[0] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[1] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[2] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[3] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[4] -= RC_CH_VALUE_OFFSET;
}

void sbus_to_usart1(uint8_t *sbus)
{
    static uint8_t usart_tx_buf[20];
    static uint8_t i = 0;
    usart_tx_buf[0] = 0xA6;
    memcpy(usart_tx_buf + 1, sbus, 18);
    for (i = 0, usart_tx_buf[19] = 0; i < 19; i++)
    {
        usart_tx_buf[19] += usart_tx_buf[i];
    }
    usart1_tx_dma_enable(usart_tx_buf, 20);
}
