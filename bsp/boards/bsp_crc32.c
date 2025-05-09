/**
 * @file bsp_crc32.c
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

#include "bsp_crc32.h"
#include "main.h"
extern CRC_HandleTypeDef hcrc;

uint32_t get_crc32_check_sum(uint32_t *data, uint32_t len)
{
    return HAL_CRC_Calculate(&hcrc, data, len);
}

bool_t verify_crc32_check_sum(uint32_t *data, uint32_t len)
{
    static uint32_t crc32;
    crc32 = get_crc32_check_sum(data, len-1);
    return (crc32 == data[len-1]);
}
void append_crc32_check_sum(uint32_t *data, uint32_t len)
{
    uint32_t crc32;
    crc32 = get_crc32_check_sum(data, len-1);
    data[len-1] = crc32;
}

