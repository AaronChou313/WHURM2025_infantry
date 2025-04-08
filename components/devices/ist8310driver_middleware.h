/**
 * @file ist8310driver_middleware.h
 * @brief IST8310磁力计中间层，完成IST8310的IIC通信（IST8310只支持IIC读取），设置的是通过MPU6500的IIC_SLV0完成读取，IIC_SLV4完成写入
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

#ifndef IST8310DRIVER_MIDDLEWARE_H
#define IST8310DRIVER_MIDDLEWARE_H

#include "struct_typedef.h"

#define IST8310_IIC_ADDRESS (0x0E << 1)  //IST8310��IIC��ַ
#define IST8310_IIC_READ_MSB (0x80) //IST8310��SPI��ȡ���͵�һ��bitΪ1

extern void ist8310_GPIO_init(void); //ist8310��io��ʼ��
extern void ist8310_com_init(void);  //ist8310��ͨѶ��ʼ��
extern uint8_t ist8310_IIC_read_single_reg(uint8_t reg);
extern void ist8310_IIC_write_single_reg(uint8_t reg, uint8_t data);
extern void ist8310_IIC_read_muli_reg(uint8_t reg, uint8_t *buf, uint8_t len);
extern void ist8310_IIC_write_muli_reg(uint8_t reg, uint8_t *data, uint8_t len);
extern void ist8310_delay_ms(uint16_t ms);
extern void ist8310_delay_us(uint16_t us);
extern void ist8310_RST_H(void); //��λIO �ø�
extern void ist8310_RST_L(void); //��λIO �õ� �õػ�����ist8310����

#endif
