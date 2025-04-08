/**
 * @file referee_usart_task.h
 * @brief RM裁判系统数据处理
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

#ifndef REFEREE_USART_TASK_H
#define REFEREE_USART_TASK_H

#include "main.h"

/*-----------------------------------宏定义-----------------------------------*/

#define USART_RX_BUF_LENGTH 512
#define REFEREE_FIFO_BUF_LENGTH 1024

/*-----------------------------------外部函数声明-----------------------------------*/

/**
 * @brief 裁判系统任务
 * @param[in] pvParameters NULL
 * @retval none
 */
extern void referee_usart_task(void const *argument);

#endif
