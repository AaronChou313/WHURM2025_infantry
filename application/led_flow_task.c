/**
 * @file led_flow_task.c
 * @brief led RGB灯效
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

#include "led_flow_task.h"
#include "bsp_led.h"
#include "cmsis_os.h"
#include "main.h"

/*-----------------------------------宏定义-----------------------------------*/

#define RGB_FLOW_COLOR_CHANGE_TIME 1000
#define RGB_FLOW_COLOR_LENGTH 6

/*-----------------------------------变量声明-----------------------------------*/

// 蓝 -> 绿（灭） -> 红 -> 蓝（灭） -> 绿 -> 红（灭） -> 蓝
uint32_t RGB_flow_color[RGB_FLOW_COLOR_LENGTH + 1] = {0xFF0000FF, 0x0000FF00, 0xFFFF0000, 0x000000FF, 0xFF00FF00, 0x00FF0000, 0xFF0000FF};

/*-----------------------------------内部函数声明-----------------------------------*/

// 空

/*-----------------------------------函数实现-----------------------------------*/

void led_RGB_flow_task(void const *argument)
{
  uint16_t i, j;
  fp32 delta_alpha, delta_red, delta_green, delta_blue;
  fp32 alpha, red, green, blue;
  uint32_t aRGB;

  while (1)
  {

    for (i = 0; i < RGB_FLOW_COLOR_LENGTH; i++)
    {
      alpha = (RGB_flow_color[i] & 0xFF000000) >> 24;
      red = ((RGB_flow_color[i] & 0x00FF0000) >> 16);
      green = ((RGB_flow_color[i] & 0x0000FF00) >> 8);
      blue = ((RGB_flow_color[i] & 0x000000FF) >> 0);

      delta_alpha = (fp32)((RGB_flow_color[i + 1] & 0xFF000000) >> 24) - (fp32)((RGB_flow_color[i] & 0xFF000000) >> 24);
      delta_red = (fp32)((RGB_flow_color[i + 1] & 0x00FF0000) >> 16) - (fp32)((RGB_flow_color[i] & 0x00FF0000) >> 16);
      delta_green = (fp32)((RGB_flow_color[i + 1] & 0x0000FF00) >> 8) - (fp32)((RGB_flow_color[i] & 0x0000FF00) >> 8);
      delta_blue = (fp32)((RGB_flow_color[i + 1] & 0x000000FF) >> 0) - (fp32)((RGB_flow_color[i] & 0x000000FF) >> 0);

      delta_alpha /= RGB_FLOW_COLOR_CHANGE_TIME;
      delta_red /= RGB_FLOW_COLOR_CHANGE_TIME;
      delta_green /= RGB_FLOW_COLOR_CHANGE_TIME;
      delta_blue /= RGB_FLOW_COLOR_CHANGE_TIME;
      for (j = 0; j < RGB_FLOW_COLOR_CHANGE_TIME; j++)
      {
        alpha += delta_alpha;
        red += delta_red;
        green += delta_green;
        blue += delta_blue;

        aRGB = ((uint32_t)(alpha)) << 24 | ((uint32_t)(red)) << 16 | ((uint32_t)(green)) << 8 | ((uint32_t)(blue)) << 0;
        aRGB_led_show(aRGB);
        osDelay(1);
      }
    }
  }
}
