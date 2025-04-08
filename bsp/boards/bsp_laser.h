/**
 * @file bsp_laser.h
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

#ifndef BSP_LASER_H
#define BSP_LASER_H
#include "struct_typedef.h"

extern void laser_configuration(void);
extern void laser_on(void);
extern void laser_off(void);
#endif
