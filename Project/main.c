/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "ctype.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t rx_char;
char tx_data[20] = "Hello\n\r";
char nhiet_do[20] = "Nhiet do cao\n\r";
uint8_t led_state[8] = {0};
uint32_t adc_val = 0;
uint8_t system_running = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
float Read_Temperature(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define LCD_RS_Pin 		GPIOA,GPIO_PIN_14
#define LCD_E_Pin 		GPIOA,GPIO_PIN_13
#define LCD_D4_Pin 		GPIOA,GPIO_PIN_6
#define LCD_D5_Pin 		GPIOA,GPIO_PIN_5
#define LCD_D6_Pin 		GPIOA,GPIO_PIN_4
#define LCD_D7_Pin 		GPIOA,GPIO_PIN_3
#define LCD_Set_Pin(LCD_Pin) 			HAL_GPIO_WritePin(LCD_Pin,GPIO_PIN_SET)
#define LCD_Reset_Pin(LCD_Pin) 			HAL_GPIO_WritePin(LCD_Pin,GPIO_PIN_RESET)
#define LCD_Write_Pin(LCD_Pin,value) 	HAL_GPIO_WritePin(LCD_Pin,value)
#define LCD_Delay_ms(time)				HAL_Delay(time)
#define LCD_GetBit(value,bit)			((value>>bit) & 1)

#define LCD_Send_cmd(Value)				LCD_Write_8bit(Value,0)
#define LCD_Send_cmd_4bit(Value)		LCD_Write_4bit(Value,0)
#define LCD_Send_data(Value)			LCD_Write_8bit(Value,1)
#define LCD_SetCursor(row,col)			LCD_Send_cmd(0x80+(row*64)+col)
#define LCD_clear()						LCD_Write_8bit(0x01,0)

void LCD_Write_4bit(uint8_t value, uint8_t cmd_data) {
	LCD_Write_Pin(LCD_RS_Pin, cmd_data);
	LCD_Set_Pin(LCD_E_Pin);
	LCD_Delay_ms(1);
	LCD_Write_Pin(LCD_D4_Pin, LCD_GetBit(value,0));
	LCD_Write_Pin(LCD_D5_Pin, LCD_GetBit(value,1));
	LCD_Write_Pin(LCD_D6_Pin, LCD_GetBit(value,2));
	LCD_Write_Pin(LCD_D7_Pin, LCD_GetBit(value,3));
	LCD_Delay_ms(1);
	LCD_Reset_Pin(LCD_E_Pin);
	LCD_Delay_ms(1);
}

void LCD_Write_8bit(uint8_t value, uint8_t cmd_data) {
	LCD_Write_4bit((value >> 4) & 0x0F, cmd_data);
	LCD_Write_4bit(value & 0x0F, cmd_data);
}

void LCD_Send_string(char *ptr) {
	int i;
	int len;
	len = strlen(ptr);
	for (i = 0; i < len; i++) {
		LCD_Write_8bit(ptr[i], 1);
	}
}

void LCD_Send_number(float number){
	char buffer[8];
	sprintf(buffer, "%.2f", number);
	LCD_Send_string(buffer);
}

void LCD_init() {
	LCD_Send_cmd_4bit(0x03);
	LCD_Delay_ms(5);
	LCD_Send_cmd_4bit(0x03);
	LCD_Delay_ms(1);
	LCD_Send_cmd_4bit(0x03);
	LCD_Delay_ms(1);
	LCD_Send_cmd_4bit(0x02);
	LCD_Delay_ms(1);
	LCD_Send_cmd(0x28);
	LCD_Delay_ms(1);
	LCD_Send_cmd(0x0C);
	LCD_Delay_ms(1);
}

void UART1_SendString(char *str){
	HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 1000);
}

void StartSystem(void)
{
    // Bắt đầu hệ thống
    UART1_SendString("System started\r\n");
    system_running = 1;
    UART1_SendString(tx_data);
    HAL_UART_Receive_IT(&huart1, &rx_char, 1);
}

void StopSystem(void)
{
    // Dừng hệ thống
    UART1_SendString("System stopped\r\n");
    system_running = 0;
}

void led_control(uint8_t pin, uint8_t state, uint8_t index) {
    if (state == 1) {
        HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
        led_state[index] = state;
    } else {
        HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
        for(int i = 0; i < index; i++){
        	led_state[i] = state;
        }
    }
}

void send_led_state(void)
{
	UART1_SendString("\r\nState_LED: ");
    uint8_t led_state_str[9];
    for (int i = 0; i < 8; i++)
    {
    	led_state_str[i] = led_state[i] + '0';
    }
    led_state_str[8] = '\r';
    HAL_UART_Transmit(&huart1, led_state_str, 9, 1000);
}

void send_temperature(void)
{
    float temp = Read_Temperature();
    char temp_msg[30];
    snprintf(temp_msg, sizeof(temp_msg), "\r\nNhiet do: %.2f oC\r\n", temp);
    HAL_UART_Transmit(&huart1, (uint8_t*)temp_msg, strlen(temp_msg), HAL_MAX_DELAY);
}
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
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  LCD_init();
  char msg[30];
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(system_running != 0){
		  static float previous_temp = -1000.0;
		  float temp = Read_Temperature();
		  if(temp != previous_temp){
			  if(temp < 70){
				  snprintf(msg, sizeof(msg), "Nhiet do: %.2f\r\n", temp);
				  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			  }
			  else{
				  UART1_SendString(nhiet_do);
			  }
			  previous_temp = temp;

			  LCD_clear();
			  LCD_Send_string("Nhiet do: ");
			  if(temp >= 70){
				  LCD_Send_string("Cao");
				  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 1);
			  }
			  else{
				  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 0);
				  LCD_Send_number(temp);
			  }
			  HAL_Delay(500);
		  }
	  }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 499;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  HAL_UART_Receive_IT(&huart1, &rx_char, 1);
  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_11|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3 PA4 PA5 PA6
                           PA11 PA13 PA14 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_11|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB15
                           PB3 PB4 PB5 PB6
                           PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

/* USER CODE BEGIN 4 */
float Read_Temperature(void){
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	adc_val = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	float voltage = (adc_val * 5.0) / 4096.0;
	float temperature = (voltage - 0.002) * 100.0;
	return temperature;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
