/**
 * @file referee_usart_task.c
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

/*-----------------------------------头文件引用-----------------------------------*/

#include "referee_usart_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "bsp_usart.h"
#include "detect_task.h"
#include "CRC8_CRC16.h"
#include "fifo.h"
#include "protocol.h"
#include "referee.h"
#include "gimbal_task.h"
#include "chassis_task.h"
#include "string.h"
#include "ui.h"
#include "usart.h"

/*-----------------------------------变量声明-----------------------------------*/

// 串口6
extern UART_HandleTypeDef huart6;
// 串口消息接收缓冲区
uint8_t usart6_buf[2][USART_RX_BUF_LENGTH];

// 裁判系统数据
fifo_s_t referee_fifo;
// 裁判系统数据缓冲区
uint8_t referee_fifo_buf[REFEREE_FIFO_BUF_LENGTH];
// 裁判系统消息标准结构，用于解包裁判系统消息
unpack_data_t referee_unpack_obj;

uint32_t ui_fpsTick; // 刷新计数
uint32_t ui_period = 100;
uint32_t ui_initTick_1; // 重新初始化绘制计数（解决丢包导致静态图案丢失）
uint32_t ui_initTick_2;
uint32_t ui_initTick_3;
extern uint32_t uwTick;

// 动态图形数据指针
extern ui_interface_round_t *ui_default_left_dynamic_rotate_state;
extern ui_interface_round_t *ui_default_left_dynamic_fric_state;
extern ui_interface_round_t *ui_default_left_dynamic_autoaim_state;
extern ui_interface_round_t *ui_default_left_dynamic_fire_state;
extern ui_interface_number_t *ui_default_left_dynamic_hp_value_i;
extern ui_interface_number_t *ui_default_left_dynamic_now_pitch_value_f;
extern ui_interface_number_t *ui_default_left_dynamic_now_yaw_value_f;

extern ui_interface_number_t *ui_default_right_dynamic_bullet_value_i;
extern ui_interface_number_t *ui_default_right_dynamic_heat_value_f;
extern ui_interface_number_t *ui_default_right_dynamic_power_value_f;
extern ui_interface_round_t *ui_default_right_dynamic_occupied_state;
extern ui_interface_number_t *ui_default_right_dynamic_aim_pitch_value_f;
extern ui_interface_number_t *ui_default_right_dynamic_aim_yaw_value_f;
extern ui_interface_round_t *ui_default_right_dynamic_rfid_state;

extern ui_interface_number_t *ui_default_velocity_dynamic_velocity_x_value_f;
extern ui_interface_number_t *ui_default_velocity_dynamic_velocity_y_value_f;

// 底盘任务控制的小陀螺标识符：0-关闭，1-正转，2-反转
extern uint8_t rotate_flag;
// 摩擦轮状态标识符：0-关闭，1-开启中，2-弹药准备就绪，3-射击准备就绪，4-射击中
extern uint8_t fric_flag;
// 自瞄标识符：0-关闭，1-开启
extern uint8_t autoaim_flag;
// 自瞄上位机数据，其中有fire标识符、自瞄pitch角度、自瞄yaw角度
extern InputData inputdata;
// 云台数据，其中有当前pitch角度、当前yaw角度
extern gimbal_control_t gimbal_control_1;
// 底盘数据
extern chassis_move_t chassis_move;
extern float rotate_speed;
// 裁判系统数据
extern ext_game_robot_state_t robot_state;        // 机器人状态
extern ext_power_heat_data_t power_heat_data_t;   // 功率与热量
extern ext_bullet_remaining_t bullet_remaining_t; // 剩余弹药量
extern ext_event_data_t field_event;              // 场地事件，其中有中心增益点占领信息
extern ext_rfid_status_t rfid_status;

extern int ui_self_id;

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 单字节解包
 * @param none
 * @retval none
 */
static void referee_unpack_fifo_data(void);

/*-----------------------------------函数实现-----------------------------------*/

void referee_usart_task(void const *argument)
{
  // 初始化裁判系统数据结构体
  init_referee_struct_data();
  // 初始化fifo序列
  fifo_s_init(&referee_fifo, referee_fifo_buf, REFEREE_FIFO_BUF_LENGTH);
  // 初始化usart6
  usart6_init(usart6_buf[0], usart6_buf[1], USART_RX_BUF_LENGTH);
  osDelay(10);
  // 初始化图像数据
  ui_init_default_aim_cross_stable();
  ui_init_default_camera_scope_stable();
  ui_init_default_left_text_stable();
  ui_init_default_right_text_stable();
  ui_init_default_left_dynamic();
  ui_init_default_right_dynamic();

  while (1)
  {
    ui_self_id = robot_state.robot_id;
    referee_unpack_fifo_data();
    osDelay(10);
    // 重新初始化绘制，避免丢包导致图形未绘制
    if ((uwTick - ui_initTick_1) > 2 * ui_period)
    {
      ui_initTick_1 = uwTick;
      ui_init_default_left_text_stable();
      ui_init_default_left_dynamic();
    }
		if ((uwTick - ui_initTick_2) > (2 * ui_period)+ui_period)
    {
      ui_initTick_2 = uwTick;
      ui_init_default_right_text_stable();
      ui_init_default_right_dynamic();
    }
		if ((uwTick - ui_initTick_3) > 5.5 * ui_period)
    {
      ui_initTick_3 = uwTick;
      ui_init_default_aim_cross_stable();
      ui_init_default_camera_scope_stable();
    }

    // 更新动态数据
		// 血量
    ui_default_left_dynamic_hp_value_i->number = (int32_t)robot_state.remain_HP;
    // 小陀螺标识符：OK
    if (rotate_flag == 0)
    {
      ui_default_left_dynamic_rotate_state->color = UI_Color_Purplish_red;
    }
    else if (rotate_flag == 1)
    {
      ui_default_left_dynamic_rotate_state->color = UI_Color_Green;
    }
    else if (rotate_flag == 2)
    {
      ui_default_left_dynamic_rotate_state->color = UI_Color_Yellow;
    }
    // 摩擦轮标识符：OK
    if (fric_flag == 0)
    {
      ui_default_left_dynamic_fric_state->color = UI_Color_Purplish_red;
    }
    else if (fric_flag == 1)
    {
      ui_default_left_dynamic_fric_state->color = UI_Color_Orange;
    }
    else if (fric_flag == 2)
    {
      ui_default_left_dynamic_fric_state->color = UI_Color_Yellow;
    }
    else if (fric_flag == 3)
    {
      ui_default_left_dynamic_fric_state->color = UI_Color_Green;
    }
    else if (fric_flag == 4)
    {
      ui_default_left_dynamic_fric_state->color = UI_Color_Cyan;
    }
    // 自瞄标识符
    if (autoaim_flag == 0)
    {
      ui_default_left_dynamic_autoaim_state->color = UI_Color_Purplish_red;
    }
    else if (autoaim_flag == 1)
    {
      ui_default_left_dynamic_autoaim_state->color = UI_Color_Green;
    }
    // 开火标识符
    if (inputdata.fire == 0)
    {
      ui_default_left_dynamic_fire_state->color = UI_Color_Purplish_red;
    }
    else if (inputdata.fire == 1)
    {
      ui_default_left_dynamic_fire_state->color = UI_Color_Green;
    }

    // 小陀螺速度
    ui_default_right_dynamic_rotate_speed_value_f->number = (int32_t)(rotate_speed*1000);

    // 占领中心点标识符
    if (field_event.center_point_occupation_status == 0)
    {
      ui_default_right_dynamic_occupied_state->color = UI_Color_Orange;
    }
    else if (field_event.center_point_occupation_status == 1)
    {
      ui_default_right_dynamic_occupied_state->color = UI_Color_Green;
    }
    else if (field_event.center_point_occupation_status == 2)
    {
      ui_default_right_dynamic_occupied_state->color = UI_Color_Purplish_red;
    }
    else if (field_event.center_point_occupation_status == 3)
    {
      ui_default_right_dynamic_occupied_state->color = UI_Color_Cyan;
    }

    // 剩余可发射弹药量：OK
    ui_default_right_dynamic_bullet_value_i->number = (int32_t)bullet_remaining_t.projectile_allowance_17mm;

    // 剩余安全枪口热量
    ui_default_right_dynamic_heat_value_f->number = (int32_t)((robot_state.shooter_heat0_limit - power_heat_data_t.shooter_heat0) * 1000);
    // 剩余安全底盘功率
    ui_default_right_dynamic_power_value_f->number = (int32_t)((robot_state.chassis_power_limit - power_heat_data_t.chassis_power) * 1000);

    // 运动速度
    ui_default_velocity_dynamic_velocity_x_value_f->number = (int32_t)(chassis_move.vx * 1000);
    ui_default_velocity_dynamic_velocity_y_value_f->number = (int32_t)(chassis_move.vy * 1000);

    // 当前pitch角：OK
    ui_default_left_dynamic_now_pitch_value_f->number = (int32_t)(gimbal_control_1.gimbal_pitch_motor.absolute_angle * 1000);
    // 当前yaw角：OK
    ui_default_left_dynamic_now_yaw_value_f->number = (int32_t)(gimbal_control_1.gimbal_yaw_motor.absolute_angle * 1000);
    // 自瞄pitch角
    ui_default_right_dynamic_aim_pitch_value_f->number = (int32_t)(inputdata.shoot_pitch * 1000);
    // 自瞄yaw角
    ui_default_right_dynamic_aim_yaw_value_f->number = (int32_t)(inputdata.shoot_yaw * 1000);

    // 刷新绘制动态数据
    if ((uwTick - ui_fpsTick) > ui_period)
    {
      ui_fpsTick = uwTick;
      ui_update_default_left_dynamic();
      ui_update_default_right_dynamic();
    }
  }
}

void referee_unpack_fifo_data(void)
{
  uint8_t byte = 0;
  uint8_t sof = HEADER_SOF;
  unpack_data_t *p_obj = &referee_unpack_obj;

  while (fifo_s_used(&referee_fifo))
  {
    byte = fifo_s_get(&referee_fifo);
    switch (p_obj->unpack_step)
    {
    case STEP_HEADER_SOF:
    {
      if (byte == sof)
      {
        p_obj->unpack_step = STEP_LENGTH_LOW;
        p_obj->protocol_packet[p_obj->index++] = byte;
      }
      else
      {
        p_obj->index = 0;
      }
    }
    break;

    case STEP_LENGTH_LOW:
    {
      p_obj->data_len = byte;
      p_obj->protocol_packet[p_obj->index++] = byte;
      p_obj->unpack_step = STEP_LENGTH_HIGH;
    }
    break;

    case STEP_LENGTH_HIGH:
    {
      p_obj->data_len |= (byte << 8);
      p_obj->protocol_packet[p_obj->index++] = byte;

      if (p_obj->data_len < (REF_PROTOCOL_FRAME_MAX_SIZE - REF_HEADER_CRC_CMDID_LEN))
      {
        p_obj->unpack_step = STEP_FRAME_SEQ;
      }
      else
      {
        p_obj->unpack_step = STEP_HEADER_SOF;
        p_obj->index = 0;
      }
    }
    break;
    case STEP_FRAME_SEQ:
    {
      p_obj->protocol_packet[p_obj->index++] = byte;
      p_obj->unpack_step = STEP_HEADER_CRC8;
    }
    break;

    case STEP_HEADER_CRC8:
    {
      p_obj->protocol_packet[p_obj->index++] = byte;

      if (p_obj->index == REF_PROTOCOL_HEADER_SIZE)
      {
        if (verify_CRC8_check_sum(p_obj->protocol_packet, REF_PROTOCOL_HEADER_SIZE))
        {
          p_obj->unpack_step = STEP_DATA_CRC16;
        }
        else
        {
          p_obj->unpack_step = STEP_HEADER_SOF;
          p_obj->index = 0;
        }
      }
    }
    break;

    case STEP_DATA_CRC16:
    {
      if (p_obj->index < (REF_HEADER_CRC_CMDID_LEN + p_obj->data_len))
      {
        p_obj->protocol_packet[p_obj->index++] = byte;
      }
      if (p_obj->index >= (REF_HEADER_CRC_CMDID_LEN + p_obj->data_len))
      {
        p_obj->unpack_step = STEP_HEADER_SOF;
        p_obj->index = 0;

        if (verify_CRC16_check_sum(p_obj->protocol_packet, REF_HEADER_CRC_CMDID_LEN + p_obj->data_len))
        {
          referee_data_solve(p_obj->protocol_packet);
        }
      }
    }
    break;

    default:
    {
      p_obj->unpack_step = STEP_HEADER_SOF;
      p_obj->index = 0;
    }
    break;
    }
  }
}

void USART6_IRQHandler(void)
{
  static volatile uint8_t res;
  if (USART6->SR & UART_FLAG_IDLE)
  {
    __HAL_UART_CLEAR_PEFLAG(&huart6);

    static uint16_t this_time_rx_len = 0;

    if ((huart6.hdmarx->Instance->CR & DMA_SxCR_CT) == RESET)
    {
      __HAL_DMA_DISABLE(huart6.hdmarx);
      this_time_rx_len = USART_RX_BUF_LENGTH - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
      __HAL_DMA_SET_COUNTER(huart6.hdmarx, USART_RX_BUF_LENGTH);
      huart6.hdmarx->Instance->CR |= DMA_SxCR_CT;
      __HAL_DMA_ENABLE(huart6.hdmarx);
      fifo_s_puts(&referee_fifo, (char *)usart6_buf[0], this_time_rx_len);
      detect_hook(REFEREE_TOE);
    }
    else
    {
      __HAL_DMA_DISABLE(huart6.hdmarx);
      this_time_rx_len = USART_RX_BUF_LENGTH - __HAL_DMA_GET_COUNTER(huart6.hdmarx);
      __HAL_DMA_SET_COUNTER(huart6.hdmarx, USART_RX_BUF_LENGTH);
      huart6.hdmarx->Instance->CR &= ~(DMA_SxCR_CT);
      __HAL_DMA_ENABLE(huart6.hdmarx);
      fifo_s_puts(&referee_fifo, (char *)usart6_buf[1], this_time_rx_len);
      detect_hook(REFEREE_TOE);
    }
  }
}
