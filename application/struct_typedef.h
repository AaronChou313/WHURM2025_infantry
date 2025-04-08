/**
 * @file struct_typedef.h
 * @brief 基本数据类型定义
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

#ifndef STRUCT_TYPEDEF_H
#define STRUCT_TYPEDEF_H

// 精确宽度有符号整数类型
typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

// 精确宽度无符号整数类型
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned char bool_t;

typedef float fp32;
typedef double fp64;

#endif
