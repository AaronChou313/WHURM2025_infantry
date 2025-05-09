/**
 * @file bsp_i2c.h
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

#ifndef BSP_I2C_H
#define BSP_I2C_H
#include "struct_typedef.h"
#include "main.h"

#define I2C_ACK 1
#define I2C_NO_ACK  0


extern void bsp_I2C_reset(I2C_TypeDef *I2C);
extern void bsp_I2C_master_transmit(I2C_TypeDef *I2C, uint16_t I2C_address, uint8_t *data, uint16_t len);
extern bool_t bsp_I2C_check_ack(I2C_TypeDef *I2C, uint16_t I2C_address);

extern void I2C2_tx_DMA_init(void);
extern void I2C2_DMA_transmit(uint16_t DevAddress, uint8_t *pData, uint16_t Size);



#endif
