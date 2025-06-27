/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_NUM 37 //number of LEDS
#define LED_LOGICAL_ONE 57 //high
#define LED_LOGICAL_ZERO 28 //low
#define BRIGHTNESS 1 //preset brightness 1-45

#define DS3231_ADDRESS 0x68 // 7-bit I2C DS3231 address
#define BIRTH_MONTH 6
#define BIRTH_DAY 29
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim2_ch1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
void set_digit(int digit_pos, uint8_t digit_number);
void set_brightness(int brightness, int led_pos);
void set_LEDS(uint32_t R, uint32_t G, uint32_t B, int led_pos);
void send_LEDS(void);
void set_all_white(void);
void reset_all_LEDS(void);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void set_digital_clock(uint8_t delimiter);
void set_time(int seconds, int minutes, int hours, int week_day, int month_day, int month, int year, int AMPM);
uint8_t dectoBCD(int num);
int BCDtodec(uint8_t bin);
void initiate_time(void);
void set_birthday_alarm(int day_of_month);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t pulse_arr[24*LED_NUM + 40]; //add bits for reset timing later
uint32_t my_LEDS[LED_NUM][3]; //0 = Green, 1 = Red, 2 = Blue -> buffer for debugging & easier brightness settings
uint8_t digit_table[12] = {
	0b1111101, //0
	0b0000101, //1
	0b1101110, //2
	0b1001111, //3
	0b0010111, //4
	0b1011011, //5
	0b1111011, //6
	0b0001101, //7
	0b1111111, //8
	0b1011111, //9
	0b0111110, //'P' (for PM)
	0b0111111 //'A' (for AM)
};
volatile int data_sent = 0;
volatile uint8_t alarm_status = 0; //boolean: 1 = bday 0 = end of day

typedef struct{
	int minutes;
	int hours;
	int AMPM;
	int day_month;
	int month;
} Time;
volatile Time curr_time;

//-------------------------------------------------------------------> LED control
void set_digit(int digit_pos, uint8_t digit_number){ //digit position 0-4, digit_number 0-9
	int position_increment = digit_pos*7;

	if(digit_pos > 1){ //taking into account colon LED indexes
		position_increment += 2;
	}

	for(int led_index = 0; led_index < 7; led_index++){
		if(digit_table[digit_number] & (0b1000000 >> led_index)){
			set_LEDS(255, 0, 0, led_index + position_increment); //red
		}
		else{
			set_LEDS(0, 0, 0, led_index + position_increment); //off
		}
	}
}

//configure the brightness w/ brightness constant at top
void set_brightness(int brightness, int led_pos){
	float angle = 90-brightness; //degrees
	angle *= M_PI/180; //radians
	my_LEDS[led_pos][0] /= tan(angle);
	my_LEDS[led_pos][1] /= tan(angle);
	my_LEDS[led_pos][2] /= tan(angle);
}

//set LED buffer to certain RGB value
void set_LEDS(uint32_t R, uint32_t G, uint32_t B, int led_pos){
	my_LEDS[led_pos][0] = G;
	my_LEDS[led_pos][1] = R;
	my_LEDS[led_pos][2] = B;
	set_brightness(BRIGHTNESS, led_pos);
}

//Sets LED pulse values for an LED at a specific index 0 to # of LEDS - 1.
//Able to edit all 24 pulses (or all 3 leds at once) in 8 loops -> # of bits for each RGB
void send_LEDS(){
	for(int j = 0; j < LED_NUM; j++){
		uint32_t RGB_bits = my_LEDS[j][0] << 16 | my_LEDS[j][1] << 8 | my_LEDS[j][2]; //creates a 24 bit number that fits all 3 LEDS (8 bits each)
		for(int i = 0; i < 8; i++){
			if(RGB_bits & 1 << i){ //for all statements below, bitmasks 8 different iterations to isolate one bit
				pulse_arr[i + 24*j] = LED_LOGICAL_ONE;
			}
			else{
				pulse_arr[i + 24*j] = LED_LOGICAL_ZERO;
			}

			if(RGB_bits & 1 << (i + 8)){
				pulse_arr[i + 8 + 24*j] = LED_LOGICAL_ONE;
			}
			else{
				pulse_arr[i + 8 + 24*j] = LED_LOGICAL_ZERO;
			}

			if(RGB_bits & 1 << (i + 16)){
				pulse_arr[i + 16 + 24*j] = LED_LOGICAL_ONE;
			}
			else{
				pulse_arr[i + 16 + 24*j] = LED_LOGICAL_ZERO;
			}
		}
	}

	data_sent = 0;
	HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *) pulse_arr, 24*LED_NUM + 40);
}

//Sets every LED on strip to white color
void set_all_white(){
	for(int i = 0; i < LED_NUM; i++){
		set_LEDS(255, 255, 255, i);
	}
}
//Turns every LED off (logical low)
void reset_all_LEDS(){
	for(int i = 0; i < LED_NUM; i++){
		set_LEDS(0, 0, 0, i);
	}
}

void set_digital_clock(uint8_t delimiter){
	if(curr_time.AMPM){ //1st digit from right, AM/PM value
		set_digit(4, 10); //index of 'P' in digital_table = 10
	}
	else{
		set_digit(4, 11); //index of 'A' in digital_table = 11
	}
	set_digit(3, curr_time.minutes%10); //2nd digit from right, one's minute value
	set_digit(2, curr_time.minutes/10); //3nd digit from right, ten's minute value
	set_digit(1, curr_time.hours%10); //4nd digit from right, one's hour value
	set_digit(0, curr_time.hours/10); //5nd digit from right, ten's hour value

	if(delimiter){
		set_LEDS(255, 0, 0, 14); //temp color red
		set_LEDS(255, 0, 0, 15);
	}
	else{
		set_LEDS(0, 0, 0, 14);
		set_LEDS(0, 0, 0, 15);
	}
}

//-------------------------------------------------------------------> DS3231 setup
void set_time(int seconds, int minutes, int hours, int week_day, int month_day, int month, int year, int AMPM){ //initialize the ds3231 registers
	uint8_t buffer[] = {
		dectoBCD(seconds), //seconds
		dectoBCD(minutes), //minutes
		(1 << 6) | ((AMPM & 1) << 5) | dectoBCD(hours), //hours & AM/PM activation, AM = 0 PM = 1
		dectoBCD(week_day), //days in week 1-7
		dectoBCD(month_day), //days in month 1-31
		dectoBCD(month), //month
		dectoBCD(year), //year
	};

	//initialize time registers in DS3231
	HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS << 1, 0x00, 1, buffer, 7, 1000);

	//clear OSF flag & 32k SQW pin
    uint8_t status;
    HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS << 1, 0x0F, 1, &status, 1, 1000);
    status &= ~((1 << 7) | (1 << 3));
    HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS << 1, 0x0F, 1, &status, 1, 1000);

}

void update_time(){
	uint8_t buffer[4];
    HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS << 1, 0x01, 1, &buffer[0], 2, 1000); //read minutes & hours
    HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS << 1, 0x04, 1, &buffer[2], 2, 1000); //read day in month & current month

    curr_time.minutes = BCDtodec(buffer[0]);
    curr_time.hours = BCDtodec(buffer[1] & 0x1F);
    curr_time.AMPM = (buffer[1] >> 5) & 0x01;
    curr_time.day_month = BCDtodec(buffer[2]);
    curr_time.month = BCDtodec(buffer[3] & 0x1F);
}

uint8_t dectoBCD(int num){
	return (uint8_t)(((num/10) << 4) + num%10);
}

int BCDtodec(uint8_t bin){
	return (int)((bin>>4)*10 + (bin & 0x0F));
}

void initiate_time(){
	set_time(59, 0, 1, 1, 1, 1, 0, 0); //edit or add API
}

void set_birthday_alarm(int day_of_month){
	//alarm control register setting
	uint8_t ctrl;
	HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS << 1, 0x0E, 1, &ctrl, 1, 1000);
	ctrl |= 0b00000111; // A1IE, A2IE, INTCN bits turned on
	HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS << 1, 0x0E, 1, &ctrl, 1, 1000);

	//create buffer used to fill & represent both alarm registers
	uint8_t alarm_buffer[] = {
		0x00, //alarm 1 seconds, set 3 seconds into the day just in case
		0x00, //a1 minutes
		0x00, //a1 hours
		dectoBCD(day_of_month), //a1 month-day
		0x00, //a2 minutes, set 1 minute at the end of the day just in case
		0x00, //a2 hours
		dectoBCD(day_of_month+1) //a2 month-day
	};

	HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS << 1, 0x07, 1, alarm_buffer, 7, 1000);
};

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
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  initiate_time();
  HAL_Delay(100);
  update_time();

  uint8_t delimiter_toggle = 1;
  uint8_t was_alarm = 0;

  set_birthday_alarm(BIRTH_DAY);
  HAL_Delay(100);

  uint32_t onboard_timer = 0; //timer for onboard LED
  uint32_t clock_timer = 0;
  set_digital_clock(delimiter_toggle);

  send_LEDS();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint32_t now_tick = HAL_GetTick();

	  if(now_tick - onboard_timer >= 500){
		  onboard_timer = now_tick;
		  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	  }

	  if(data_sent && now_tick - clock_timer >= 1000){
		  delimiter_toggle = !delimiter_toggle;
		  clock_timer = now_tick;
		  update_time();
		  set_digital_clock(delimiter_toggle);
		  send_LEDS();
	  }

	  if(alarm_status){
		  was_alarm = 1;
		  //change color every tick like rainbow
	  } else if(was_alarm){
		  //set color back to red
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 89;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  huart1.Init.BaudRate = 115200;
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

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : Alarm_Trigger_Pin */
  GPIO_InitStruct.Pin = Alarm_Trigger_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Alarm_Trigger_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == GPIO_PIN_12) {
		uint8_t ctrl_status;
		HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS << 1, 0x0F, 1, &ctrl_status, 1, 1000);
		ctrl_status &= ~(0b00000011); //clears both alarm bits
		HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS << 1, 0x0F, 1, &ctrl_status, 1, 1000);

		update_time();
		if(curr_time.month == BIRTH_MONTH && curr_time.day_month == BIRTH_DAY){ //WIP
			alarm_status = 1;
		}
		else{
			alarm_status = 0;
		}
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_1);
	data_sent = 1;
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
	//while (1); //halt program and keep it running
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
