/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "can.h"
#include "crc.h"
#include "dma.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_can.h"
#include "bsp_delay.h"
#include "bsp_usart.h"
#include "remote_control.h"

#include "calibrate_task.h"
#include "chassis_task.h"
#include "detect_task.h"
#include "gimbal_task.h"
#include "INS_task.h"
#include "led_flow_task.h"
#include "oled_task.h"
#include "referee_usart_task.h"
#include "referee.h"
#include "ui_interface.h"
#include "usb_task.h"
#include "voltage_task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
float X_set, Y_set, W_set, X_real, Y_real, W_real;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
extern uint16_t get_CRC16_check_sum(uint8_t *pchMessage, uint32_t dwLength, uint16_t wCRC); // 生成CRC16校验位
extern uint32_t verify_CRC16_check_sum(uint8_t *pchMessage, uint32_t dwLength);             // 检验CRC16校验位
extern ext_game_robot_state_t robot_state; // 机器人数据

#define RX_BUFFER_SIZE 14 * 10
InputData inputdata;

SEND_DATA Send_Data;
bool_t rec_flag = 0;

extern chassis_move_t chassis_move;       // 底盘控制结构体
extern gimbal_control_t gimbal_control_1; // 云台控制结构体
extern __IO uint32_t uwTick;              // 系统时钟（持续递增）

uint8_t buffer[sizeof(RECEIVE_DATA)]; // 用于存储完整的数据包
uint8_t Usart_Receive[sizeof(RECEIVE_DATA)];             // 用于接收单个字节的数据
uint16_t received = 0;                // 当前接收到的数据长度
uint8_t Count = 0;                    // 接收状态标签
uint32_t sendTick = 0;                // 发送时钟（用于检验发送周期）
uint8_t enemy_color = 0;

void robot_id_to_enemy_color(uint8_t robot_id, uint8_t *enemy_color)
{
	switch(robot_id)
	{
		case UI_Data_RobotID_RHero:         
		case UI_Data_RobotID_REngineer:
		case UI_Data_RobotID_RStandard1:
		case UI_Data_RobotID_RStandard2:
		case UI_Data_RobotID_RStandard3:
		case UI_Data_RobotID_RAerial:
		case UI_Data_RobotID_RSentry:
		case UI_Data_RobotID_RRadar:
			*enemy_color = 0;
			break;
		case UI_Data_RobotID_BHero:
		case UI_Data_RobotID_BEngineer:
		case UI_Data_RobotID_BStandard1:
		case UI_Data_RobotID_BStandard2:
		case UI_Data_RobotID_BStandard3:
		case UI_Data_RobotID_BAerial:
		case UI_Data_RobotID_BSentry:
		case UI_Data_RobotID_BRadar:
			*enemy_color = 1;
			break;
		default:
			break;
	}
}

void rx_frame_handler()
{
  uint8_t received_byte = Usart_Receive[0]; // 读取接收到的字节
  // 读取该字节，检查是否为帧头的sof
  if (Count == 0)
  {
    if (received_byte == FRAME_HEADER_SOF)
    {
      buffer[received++] = received_byte; // 存储帧头起始标志
      Count = 1;                          // 进入帧头接受阶段
    }
    else
    {
      received = 0; // 不是帧头标志则丢弃数据
    }
  }
  else if (Count == 1) // 接收帧头的CRC8字段
  {
    buffer[received++] = received_byte;

    // 检查帧头接收是否完成
    if (received == sizeof(FrameHeader))
    {
      FrameHeader *header = (FrameHeader *)buffer;

      // 检查sof和CRC8是否符合固定值
      if (header->sof == FRAME_HEADER_SOF && header->crc8 == 0x00)
      {
        Count = 2; // 帧头校验通过，准备接受数据部分
      }
      else
      {
        received = 0; // 帧头不匹配，丢弃数据重置接收
        Count = 0;
      }
    }
  }
  else if (Count == 2) // 接收数据部分以及帧尾
  {
    buffer[received++] = received_byte;

    // 检查数据包是否接收完整
    if (received == sizeof(FrameHeader) + sizeof(InputData) + sizeof(FrameTailer))
    {
      FrameTailer *tail = (FrameTailer *)(buffer + sizeof(FrameHeader) + sizeof(InputData));

      // 检查帧尾
      if (verify_CRC16_check_sum((uint8_t *)buffer, sizeof(RECEIVE_DATA)))
      {

        // 数据包检验成功，解析数据部分
        memcpy(&inputdata, buffer + sizeof(FrameHeader), sizeof(InputData));
        // memset(buffer, 0, sizeof(RECEIVE_DATA));
      }

      // 无论是否校验成功，一次信息接收已结束，重置接受状态
      received = 0;
      Count = 0;
    }
  }
  // 重新开启帧接收
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	 if (huart->Instance == USART1)
	 {
		 if(Usart_Receive[0]==FRAME_HEADER_SOF&&Usart_Receive[1]== 0x00)
		 {
			   memcpy(&inputdata, Usart_Receive + sizeof(FrameHeader), sizeof(InputData));
		 }
		 
		  HAL_UART_Receive_IT(&huart1, (uint8_t *)Usart_Receive, sizeof(RECEIVE_DATA));
	 }
	 
 }

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        
        if (__HAL_UART_GET_FLAG(huart, UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE)) {
            __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_ORE);
        }
        huart->RxState = HAL_UART_STATE_READY;
        HAL_UART_Receive_IT(huart, (uint8_t *)Usart_Receive,  sizeof(RECEIVE_DATA));
				//HAL_UART_Receive_IT(huart, (uint8_t *)Usart_Receive, 14);
    }
}

void data_transition(void)
{
  Send_Data.frame_header.sof = FRAME_HEADER_SOF; //Frame_header 
	Send_Data.frame_header.crc8= FRAME_HEADER_CRC8;
	//Send_Data.frame_tailer.crc16 = FRAME_TAILER_CRC16; //Frame_tail 
	
  Send_Data.output_data.curr_yaw = gimbal_control_1.gimbal_yaw_motor.absolute_angle;        
  Send_Data.output_data.curr_pitch = gimbal_control_1.gimbal_pitch_motor.absolute_angle;     
  Send_Data.output_data.curr_omega = gimbal_control_1.gimbal_yaw_motor.motor_gyro;//0.0000015f;//-0.000006f;      
  Send_Data.output_data.state = 0;              
  Send_Data.output_data.autoaim = 4;            
//	robot_id_to_enemy_color(robot_state.robot_id, &enemy_color);
  Send_Data.output_data.enemy_color = 1;   
	
	uint8_t *data_ptr = (uint8_t *)&Send_Data;//½«Send_Data×÷Îª×Ö½ÚÊý×é
	uint32_t data_length = sizeof(SEND_DATA)-sizeof(Send_Data.frame_tailer.crc16); // ¼ÆËãCRCµÄ³¤¶È£¬ÅÅ³ýcrc16×Ö¶Î
	Send_Data.frame_tailer.crc16 = get_CRC16_check_sum(data_ptr,data_length,FRAME_TAILER_CRC16_init);
	 //uint8_t *data_ptr = (uint8_t *)&Send_Data.output_data;
   //uint32_t data_length = sizeof(Send_Data.output_data);
   //Send_Data.frame_tailer.crc16 = get_CRC16_check_sum(data_ptr, data_length, FRAME_TAILER_CRC16_init);
}

void USART1_SEND(void)
{
  // 如果发送间隔小于30ms，则不发送，目的是使信息发送周期固定为30ms
  if (uwTick - sendTick < 15)
    return;
  // 达到30ms，则将发送时间同步为当前时间，并开始发消息
  sendTick = uwTick;
  uint8_t *data_ptr = (uint8_t *)&Send_Data;
  uint16_t data_length = sizeof(SEND_DATA);
  uint8_t newline = '\n';

  // 使用 HAL_UART_Transmit_IT 使用非阻塞模式发送整个数据包
  HAL_UART_Transmit_IT(&huart1, data_ptr, data_length);
  while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET)
    ;
  HAL_UART_Transmit_IT(&huart1, &newline, 1);
  // HAL_Delay(100);
}

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC3_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_USART3_UART_Init();
  MX_TIM8_Init();
  MX_CRC_Init();
  MX_RNG_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_RTC_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM10_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  can_filter_init();
  delay_init();
  cali_param_init();
  remote_control_init();
  // usart1_tx_dma_init(); // 如果出现问题，删去该句试试
//  HAL_UART_Receive_IT(&huart1, (uint8_t *)Usart_Receive, 1);
HAL_UART_Receive_IT(&huart1,(uint8_t*)Usart_Receive,sizeof(RECEIVE_DATA));
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    //		chassis_move.vx_set=vx_set;
    //		chassis_move.vy_set=vy_set;
    //		chassis_move.wz_set=wz_set;
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV30;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
