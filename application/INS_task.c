/**
 * @file INS_task.c
 * @brief 使用陀螺仪bmi088计算欧拉角，不使用磁力计ist8310，因此只启用data ready引脚以节省CPU时间
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

#include "INS_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "bsp_imu_pwm.h"
#include "bsp_spi.h"
#include "bmi088driver.h"
#include "ist8310driver.h"
#include "pid.h"
#include "ahrs.h"
#include "calibrate_task.h"
#include "detect_task.h"

/*-----------------------------------宏定义-----------------------------------*/

// PWM给定
#define IMU_temp_PWM(pwm) imu_pwm_set(pwm)

#define BMI088_BOARD_INSTALL_SPIN_MATRIX \
  {0.0f, 1.0f, 0.0f},                    \
      {-1.0f, 0.0f, 0.0f},               \
      {0.0f, 0.0f, 1.0f}

#define IST8310_BOARD_INSTALL_SPIN_MATRIX \
  {1.0f, 0.0f, 0.0f},                     \
      {0.0f, 1.0f, 0.0f},                 \
      {0.0f, 0.0f, 1.0f}

/*-----------------------------------内部函数声明-----------------------------------*/

/**
 * @brief 旋转陀螺仪、加速度计、磁力计，并计算零漂，因为设备有不同安装方式
 * @param[out] gyro 加上零漂和旋转
 * @param[out] accel 加上零漂和旋转
 * @param[out] mag 加上零漂和旋转
 * @param[in] bmi088 陀螺仪和加速度计数据
 * @param[in] ist8310 磁力计数据
 * @retval none
 */
static void imu_cali_slove(fp32 gyro[3], fp32 accel[3], fp32 mag[3], bmi088_real_data_t *bmi088, ist8310_real_data_t *ist8310);

/**
 * @brief 控制bmi088的温度
 * @param[in] temp bmi088的温度
 * @retval none
 */
static void imu_temp_control(fp32 temp);

/**
 * @brief 根据imu_update_flag的值，开启SPI DMA
 * @param[in] none
 * @retval none
 */
static void imu_cmd_spi_dma(void);

/*-----------------------------------变量声明-----------------------------------*/

extern SPI_HandleTypeDef hspi1;

static TaskHandle_t INS_task_local_handler;

uint8_t gyro_dma_rx_buf[SPI_DMA_GYRO_LENGTH];
uint8_t gyro_dma_tx_buf[SPI_DMA_GYRO_LENGTH] = {0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t accel_dma_rx_buf[SPI_DMA_ACCEL_LENGTH];
uint8_t accel_dma_tx_buf[SPI_DMA_ACCEL_LENGTH] = {0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t accel_temp_dma_rx_buf[SPI_DMA_ACCEL_TEMP_LENGTH];
uint8_t accel_temp_dma_tx_buf[SPI_DMA_ACCEL_TEMP_LENGTH] = {0xA2, 0xFF, 0xFF, 0xFF};

volatile uint8_t gyro_update_flag = 0;
volatile uint8_t accel_update_flag = 0;
volatile uint8_t accel_temp_update_flag = 0;
volatile uint8_t mag_update_flag = 0;
volatile uint8_t imu_start_dma_flag = 0;

bmi088_real_data_t bmi088_real_data;
fp32 gyro_scale_factor[3][3] = {BMI088_BOARD_INSTALL_SPIN_MATRIX};
fp32 gyro_offset[3];
fp32 gyro_cali_offset[3];

fp32 accel_scale_factor[3][3] = {BMI088_BOARD_INSTALL_SPIN_MATRIX};
fp32 accel_offset[3];
fp32 accel_cali_offset[3];

ist8310_real_data_t ist8310_real_data;
fp32 mag_scale_factor[3][3] = {IST8310_BOARD_INSTALL_SPIN_MATRIX};
fp32 mag_offset[3];
fp32 mag_cali_offset[3];

static uint8_t first_temperate;
static const fp32 imu_temp_PID[3] = {TEMPERATURE_PID_KP, TEMPERATURE_PID_KI, TEMPERATURE_PID_KD};
static pid_type_def imu_temp_pid;

// 任务运行的时间，单位s
static const float timing_time = 0.001f;

// 加速度计低通滤波
static fp32 accel_fliter_1[3] = {0.0f, 0.0f, 0.0f};
static fp32 accel_fliter_2[3] = {0.0f, 0.0f, 0.0f};
static fp32 accel_fliter_3[3] = {0.0f, 0.0f, 0.0f};
static const fp32 fliter_num[3] = {1.929454039488895f, -0.93178349823448126f, 0.002329458745586203f};

static fp32 INS_gyro[3] = {0.0f, 0.0f, 0.0f};
static fp32 INS_accel[3] = {0.0f, 0.0f, 0.0f};
static fp32 INS_mag[3] = {0.0f, 0.0f, 0.0f};
static fp32 INS_quat[4] = {0.0f, 0.0f, 0.0f, 0.0f};
// 欧拉角，单位rad
fp32 INS_angle[3] = {0.0f, 0.0f, 0.0f};

// 标定系数
float cali_scale[3]={1.0f,1.0f,1.0f};

int it=0;

/*-----------------------------------函数实现-----------------------------------*/

void INS_task(void const *pvParameters)
{
  // 延迟一段时间，确保系统稳定
  osDelay(INS_TASK_INIT_TIME);
  // 初始化BMI088传感器，如果初始化失败就等待100ms后重新初始化，直至初始化成功（返回零值）
  while (BMI088_init())
  {
    osDelay(100);
  }
  // 初始化ist8310传感器，如果初始化失败就等待100ms后重新初始化，直至初始化成功（返回零值）
  while (ist8310_init())
  {
    osDelay(100);
  }

  // 从BMI088传感器读取陀螺仪、加速度计、温度数据
  BMI088_read(bmi088_real_data.gyro, bmi088_real_data.accel, &bmi088_real_data.temp);
  // 对获取到的陀螺仪、加速度计、磁力计数据进行校准处理（利用bmi和ist的数据计算gyro、accel、mag）
  imu_cali_slove(INS_gyro, INS_accel, INS_mag, &bmi088_real_data, &ist8310_real_data);
  // 初始化温度PID控制结构体
  PID_init(&imu_temp_pid, PID_POSITION, imu_temp_PID, TEMPERATURE_PID_MAX_OUT, TEMPERATURE_PID_MAX_IOUT);
  // 初始化四元数quat、加速度计accel、磁力计mag
  AHRS_init(INS_quat, INS_accel, INS_mag);

  // 初始化加速度滤波器
  accel_fliter_1[0] = accel_fliter_2[0] = accel_fliter_3[0] = INS_accel[0];
  accel_fliter_1[1] = accel_fliter_2[1] = accel_fliter_3[1] = INS_accel[1];
  accel_fliter_1[2] = accel_fliter_2[2] = accel_fliter_3[2] = INS_accel[2];
  // 获取当前任务的任务句柄
  INS_task_local_handler = xTaskGetHandle(pcTaskGetName(NULL));

  // 设置SPI1通信的时钟频率为系统时钟频率的1/8
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;

  // 初始化SPI1接口，如果初始化失败就进入错误处理
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

  // 初始化SPI1的DMA传输，配置发送缓冲区tx_buf、接收缓冲区rx_buf，以及传输长度
  SPI1_DMA_init((uint32_t)gyro_dma_tx_buf, (uint32_t)gyro_dma_rx_buf, SPI_DMA_GYRO_LENGTH);

  // 将start dma的标志符设为真，表示开启DMA（直接内存访问）传输
  imu_start_dma_flag = 1;

  while (1)
  {
    // 等待SPI的DMA传输
    while (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != pdPASS)
    {
    }

    // 检查陀螺仪更新的标志位
    if (gyro_update_flag & (1 << IMU_NOTIFY_SHFITS))
    {
      gyro_update_flag &= ~(1 << IMU_NOTIFY_SHFITS);
      BMI088_gyro_read_over(gyro_dma_rx_buf + BMI088_GYRO_RX_BUF_DATA_OFFSET, bmi088_real_data.gyro);
    }
    // 检查加速度计更新的标志位
    if (accel_update_flag & (1 << IMU_UPDATE_SHFITS))
    {
      accel_update_flag &= ~(1 << IMU_UPDATE_SHFITS);
      BMI088_accel_read_over(accel_dma_rx_buf + BMI088_ACCEL_RX_BUF_DATA_OFFSET, bmi088_real_data.accel, &bmi088_real_data.time);
    }
    // 检查加速度计温度更新的标志位
    if (accel_temp_update_flag & (1 << IMU_UPDATE_SHFITS))
    {
      accel_temp_update_flag &= ~(1 << IMU_UPDATE_SHFITS);
      BMI088_temperature_read_over(accel_temp_dma_rx_buf + BMI088_ACCEL_RX_BUF_DATA_OFFSET, &bmi088_real_data.temp);
      imu_temp_control(bmi088_real_data.temp);
    }

    // // 陀螺仪校准
    // if (it <= 150)
    // {
    //   static uint16_t offset_time_count = 0;
    //   INS_cali_gyro(cali_scale, gyro_offset, &offset_time_count);
    //   it++;
    // }

    // 对获取到的陀螺仪、加速度计、磁力计数据进行校准处理（利用bmi和ist的数据计算gyro、accel、mag）
    imu_cali_slove(INS_gyro, INS_accel, INS_mag, &bmi088_real_data, &ist8310_real_data);

    // 加速度计低通滤波
    accel_fliter_1[0] = accel_fliter_2[0];
    accel_fliter_2[0] = accel_fliter_3[0];
    accel_fliter_3[0] = accel_fliter_2[0] * fliter_num[0] + accel_fliter_1[0] * fliter_num[1] + INS_accel[0] * fliter_num[2];

    accel_fliter_1[1] = accel_fliter_2[1];
    accel_fliter_2[1] = accel_fliter_3[1];
    accel_fliter_3[1] = accel_fliter_2[1] * fliter_num[0] + accel_fliter_1[1] * fliter_num[1] + INS_accel[1] * fliter_num[2];

    accel_fliter_1[2] = accel_fliter_2[2];
    accel_fliter_2[2] = accel_fliter_3[2];
    accel_fliter_3[2] = accel_fliter_2[2] * fliter_num[0] + accel_fliter_1[2] * fliter_num[1] + INS_accel[2] * fliter_num[2];

    // 更新AHRS（姿态和航向参考系统）四元数
    AHRS_update(INS_quat, timing_time, INS_gyro, accel_fliter_3, INS_mag);
    // 根据AHRS四元数计算欧拉角yaw、pitch、roll
    get_angle(INS_quat, INS_angle + INS_YAW_ADDRESS_OFFSET, INS_angle + INS_PITCH_ADDRESS_OFFSET, INS_angle + INS_ROLL_ADDRESS_OFFSET);

    // because no use ist8310 and save time, no use
    if (mag_update_flag &= 1 << IMU_DR_SHFITS)
    {
      mag_update_flag &= ~(1 << IMU_DR_SHFITS);
      mag_update_flag |= (1 << IMU_SPI_SHFITS);
      //            ist8310_read_mag(ist8310_real_data.mag);
    }
  }
}

static void imu_cali_slove(fp32 gyro[3], fp32 accel[3], fp32 mag[3], bmi088_real_data_t *bmi088, ist8310_real_data_t *ist8310)
{
  for (uint8_t i = 0; i < 3; i++)
  {
    gyro[i] = bmi088->gyro[0] * gyro_scale_factor[i][0] + bmi088->gyro[1] * gyro_scale_factor[i][1] + bmi088->gyro[2] * gyro_scale_factor[i][2] + gyro_offset[i];
    accel[i] = bmi088->accel[0] * accel_scale_factor[i][0] + bmi088->accel[1] * accel_scale_factor[i][1] + bmi088->accel[2] * accel_scale_factor[i][2] + accel_offset[i];
    mag[i] = ist8310->mag[0] * mag_scale_factor[i][0] + ist8310->mag[1] * mag_scale_factor[i][1] + ist8310->mag[2] * mag_scale_factor[i][2] + mag_offset[i];
  }
}

static void imu_temp_control(fp32 temp)
{
  uint16_t tempPWM;
  static uint8_t temp_constant_time = 0;
  if (first_temperate)
  {
    PID_calc(&imu_temp_pid, temp, get_control_temperature());
    if (imu_temp_pid.out < 0.0f)
    {
      imu_temp_pid.out = 0.0f;
    }
    tempPWM = (uint16_t)imu_temp_pid.out;
    IMU_temp_PWM(tempPWM);
  }
  else
  {
    // 没有达到设置的温度，一直最大功率加热
    if (temp > get_control_temperature())
    {
      temp_constant_time++;
      if (temp_constant_time > 200)
      {
        // 达到设置温度，将积分项设置为一般最大功率，加速收敛
        first_temperate = 1;
        imu_temp_pid.Iout = MPU6500_TEMP_PWM_MAX / 2.0f;
      }
    }

    IMU_temp_PWM(MPU6500_TEMP_PWM_MAX - 1);
  }
}

void gyro_offset_calc(fp32 gyro_offset[3], fp32 gyro[3], uint16_t *offset_time_count)
{
  if (gyro_offset == NULL || gyro == NULL || offset_time_count == NULL)
  {
    return;
  }

  // gyro_offset[0] = gyro_offset[0] - 0.0003f * gyro[0];
  // gyro_offset[1] = gyro_offset[1] - 0.0003f * gyro[1];
  // gyro_offset[2] = gyro_offset[2] - 0.0003f * gyro[2];

  // 设置学习率为0.03
  gyro_offset[0] = gyro_offset[0] - 0.03f * gyro[0];
  gyro_offset[1] = gyro_offset[1] - 0.03f * gyro[1];
  gyro_offset[2] = gyro_offset[2] - 0.03f * gyro[2];
  (*offset_time_count)++;
}

void INS_cali_gyro(fp32 cali_scale[3], fp32 cali_offset[3], uint16_t *time_count)
{
  if (*time_count == 0)
  {
    gyro_offset[0] = gyro_cali_offset[0];
    gyro_offset[1] = gyro_cali_offset[1];
    gyro_offset[2] = gyro_cali_offset[2];
  }
  gyro_offset_calc(gyro_offset, INS_gyro, time_count);

  cali_offset[0] = gyro_offset[0];
  cali_offset[1] = gyro_offset[1];
  cali_offset[2] = gyro_offset[2];
  cali_scale[0] = 1.0f;
  cali_scale[1] = 1.0f;
  cali_scale[2] = 1.0f;
}

void INS_set_cali_gyro(fp32 cali_scale[3], fp32 cali_offset[3])
{
  gyro_cali_offset[0] = cali_offset[0];
  gyro_cali_offset[1] = cali_offset[1];
  gyro_cali_offset[2] = cali_offset[2];
  gyro_offset[0] = gyro_cali_offset[0];
  gyro_offset[1] = gyro_cali_offset[1];
  gyro_offset[2] = gyro_cali_offset[2];
}

const fp32 *get_INS_quat_point(void)
{
  return INS_quat;
}

const fp32 *get_INS_angle_point(void)
{
  return INS_angle;
}

extern const fp32 *get_gyro_data_point(void)
{
  return INS_gyro;
}

extern const fp32 *get_accel_data_point(void)
{
  return INS_accel;
}

extern const fp32 *get_mag_data_point(void)
{
  return INS_mag;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == INT1_ACCEL_Pin)
  {
    detect_hook(BOARD_ACCEL_TOE);
    accel_update_flag |= 1 << IMU_DR_SHFITS;
    accel_temp_update_flag |= 1 << IMU_DR_SHFITS;
    if (imu_start_dma_flag)
    {
      imu_cmd_spi_dma();
    }
  }
  else if (GPIO_Pin == INT1_GYRO_Pin)
  {
    detect_hook(BOARD_GYRO_TOE);
    gyro_update_flag |= 1 << IMU_DR_SHFITS;
    if (imu_start_dma_flag)
    {
      imu_cmd_spi_dma();
    }
  }
  else if (GPIO_Pin == DRDY_IST8310_Pin)
  {
    detect_hook(BOARD_MAG_TOE);
    mag_update_flag |= 1 << IMU_DR_SHFITS;
  }
  else if (GPIO_Pin == GPIO_PIN_0)
  {

    // 唤醒任务
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
      static BaseType_t xHigherPriorityTaskWoken;
      vTaskNotifyGiveFromISR(INS_task_local_handler, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}

static void imu_cmd_spi_dma(void)
{
  UBaseType_t uxSavedInterruptStatus;
  uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

  // 开启陀螺仪的DMA传输
  if ((gyro_update_flag & (1 << IMU_DR_SHFITS)) && !(hspi1.hdmatx->Instance->CR & DMA_SxCR_EN) && !(hspi1.hdmarx->Instance->CR & DMA_SxCR_EN) && !(accel_update_flag & (1 << IMU_SPI_SHFITS)) && !(accel_temp_update_flag & (1 << IMU_SPI_SHFITS)))
  {
    gyro_update_flag &= ~(1 << IMU_DR_SHFITS);
    gyro_update_flag |= (1 << IMU_SPI_SHFITS);

    HAL_GPIO_WritePin(CS1_GYRO_GPIO_Port, CS1_GYRO_Pin, GPIO_PIN_RESET);
    SPI1_DMA_enable((uint32_t)gyro_dma_tx_buf, (uint32_t)gyro_dma_rx_buf, SPI_DMA_GYRO_LENGTH);
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    return;
  }
  // 开启加速度计的DMA传输
  if ((accel_update_flag & (1 << IMU_DR_SHFITS)) && !(hspi1.hdmatx->Instance->CR & DMA_SxCR_EN) && !(hspi1.hdmarx->Instance->CR & DMA_SxCR_EN) && !(gyro_update_flag & (1 << IMU_SPI_SHFITS)) && !(accel_temp_update_flag & (1 << IMU_SPI_SHFITS)))
  {
    accel_update_flag &= ~(1 << IMU_DR_SHFITS);
    accel_update_flag |= (1 << IMU_SPI_SHFITS);

    HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_RESET);
    SPI1_DMA_enable((uint32_t)accel_dma_tx_buf, (uint32_t)accel_dma_rx_buf, SPI_DMA_ACCEL_LENGTH);
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    return;
  }

  if ((accel_temp_update_flag & (1 << IMU_DR_SHFITS)) && !(hspi1.hdmatx->Instance->CR & DMA_SxCR_EN) && !(hspi1.hdmarx->Instance->CR & DMA_SxCR_EN) && !(gyro_update_flag & (1 << IMU_SPI_SHFITS)) && !(accel_update_flag & (1 << IMU_SPI_SHFITS)))
  {
    accel_temp_update_flag &= ~(1 << IMU_DR_SHFITS);
    accel_temp_update_flag |= (1 << IMU_SPI_SHFITS);

    HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_RESET);
    SPI1_DMA_enable((uint32_t)accel_temp_dma_tx_buf, (uint32_t)accel_temp_dma_rx_buf, SPI_DMA_ACCEL_TEMP_LENGTH);
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    return;
  }
  taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

void DMA2_Stream2_IRQHandler(void)
{

  if (__HAL_DMA_GET_FLAG(hspi1.hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi1.hdmarx)) != RESET)
  {
    __HAL_DMA_CLEAR_FLAG(hspi1.hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi1.hdmarx));

    // 陀螺仪读取完毕
    if (gyro_update_flag & (1 << IMU_SPI_SHFITS))
    {
      gyro_update_flag &= ~(1 << IMU_SPI_SHFITS);
      gyro_update_flag |= (1 << IMU_UPDATE_SHFITS);

      HAL_GPIO_WritePin(CS1_GYRO_GPIO_Port, CS1_GYRO_Pin, GPIO_PIN_SET);
    }

    // 加速度计读取完毕
    if (accel_update_flag & (1 << IMU_SPI_SHFITS))
    {
      accel_update_flag &= ~(1 << IMU_SPI_SHFITS);
      accel_update_flag |= (1 << IMU_UPDATE_SHFITS);

      HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_SET);
    }
    // 温度读取完毕
    if (accel_temp_update_flag & (1 << IMU_SPI_SHFITS))
    {
      accel_temp_update_flag &= ~(1 << IMU_SPI_SHFITS);
      accel_temp_update_flag |= (1 << IMU_UPDATE_SHFITS);

      HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_SET);
    }

    imu_cmd_spi_dma();

    if (gyro_update_flag & (1 << IMU_UPDATE_SHFITS))
    {
      gyro_update_flag &= ~(1 << IMU_UPDATE_SHFITS);
      gyro_update_flag |= (1 << IMU_NOTIFY_SHFITS);
      __HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_0);
    }
  }
}
