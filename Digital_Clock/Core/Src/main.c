/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *x
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
typedef struct{
	int minutes;
	int hours;
	int AMPM;
	int day_month;
	int month;
} Time;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_NUM 30 //number of LEDS
#define LED_LOGICAL_ONE 57 //high
#define LED_LOGICAL_ZERO 28 //low
#define BRIGHTNESS 1 //preset brightness 1-45
#define DELIMITER_LED1 14
#define DELIMITER_LED2 15

#define DS3231_ADDRESS 0x68 // 7-bit I2C DS3231 address
#define BIRTH_MONTH 6
#define BIRTH_DAY 29

#define DHT11_PORT GPIOB
#define DHT11_PIN 10
#define DHT11_INTERVAL 300000 //ms
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim2_ch1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static uint16_t pulse_arr[24*LED_NUM + 40]; // 24 bits per LED + reset pulse (> 50us = 40 slots at 1.25 µs per bit)
static uint8_t digit_table[12] = {
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
	0b1111000, //C
	0b0111010, //F
	0b0011110 //° degree symbol
};
volatile int data_sent = 0;
volatile uint8_t alarm_status = 0; //boolean: 1 = bday 0 = end of day
static volatile Time curr_time;
static uint8_t ToggleLED[LED_NUM];
static volatile uint8_t dht11_data[5];
static volatile uint8_t clock_temp_toggle = 1; //0 for clock, 1 for temperature
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void set_digit(int digit_pos, uint8_t digit_number);
void adjust_brightness(int brightness, uint8_t *RGB, int led_pos);
void set_LEDS(uint8_t R, uint8_t G, uint8_t B, int led_pos);
void send_LEDS(void);
void set_all_white(void);
void reset_all_LEDS(void);
void set_Toggle(int led_pos, uint8_t toggle);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void set_digital_clock(uint8_t delimiter);
void set_time(int seconds, int minutes, int hours, int week_day, int month_day, int month, int year, int AMPM);
uint8_t dectoBCD(int num);
int BCDtodec(uint8_t bin);
void initiate_time(void);
void set_birthday_alarm(int day_of_month);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void set_gpio_outin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t status);
void delay_us(uint16_t us);
void dht11_initialization();
uint8_t dht11_response();
uint8_t dht11_byte_read();
void calculate_feelslike_temp(uint8_t temp, uint8_t humidity);
float celsius_to_fahrenheit(uint8_t temp);
float fahrenheit_to_celsius(uint8_t temp);
void set_temp(uint8_t temp);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//-------------------------------------------------------------------> LED control
void set_digit(int digit_pos, uint8_t digit_number){ //digit position 0-4, digit_number 0-9
	int position_increment = digit_pos*7;

	if(digit_pos > 1){ //taking into account colon LED indexes
		position_increment += 2;
	}

	for(int led_index = 0; led_index < 7; led_index++){
		if(digit_table[digit_number] & (0b1000000 >> led_index)){
			set_Toggle(led_index + position_increment, 1); //on
		}
		else{
			set_Toggle(led_index + position_increment, 0); //off
		}
	}
}

//configure the brightness w/ brightness constant at top
void adjust_brightness(int brightness, uint8_t *RGB, int led_pos){
	if(brightness > 44){
			brightness = 44;
	} else if(brightness < 1){
		brightness = 1;
	}

	float angle = 90-brightness; //degrees
	angle *= M_PI/180; //radians

	RGB[0] /= tan(angle);
	RGB[1] /= tan(angle);
	RGB[2] /= tan(angle);
}

//Able to edit all 24 pulses (or all 3 RGB at once) in 8 loops -> # of bits for each RGB
void set_LEDS(uint8_t R, uint8_t G, uint8_t B, int led_pos){
	uint8_t RGB[3] = {R, G, B};
	adjust_brightness(BRIGHTNESS, RGB, led_pos);

	if (!ToggleLED[led_pos]) {//only activates LEDS if LED is toggled in toggle array
		RGB[0] = 0;
		RGB[1] = 0;
		RGB[2] = 0;
	}

	for(int i = 0; i < 8; i++){
		if(((RGB[1] << i) >> 7) & 1){ //for all statements below, bitmasks 8 different iterations to isolate one bit
			pulse_arr[i + 24*led_pos] = LED_LOGICAL_ONE;
		}
		else{
			pulse_arr[i + 24*led_pos] = LED_LOGICAL_ZERO;
		}

		if(((RGB[0] << i) >> 7) & 1){
			pulse_arr[i + 8 + 24*led_pos] = LED_LOGICAL_ONE;
		}
		else{
			pulse_arr[i + 8 + 24*led_pos] = LED_LOGICAL_ZERO;
		}

		if(((RGB[2] << i) >> 7) & 1){
			pulse_arr[i + 16 + 24*led_pos] = LED_LOGICAL_ONE;
		}
		else{
			pulse_arr[i + 16 + 24*led_pos] = LED_LOGICAL_ZERO;
		}
	}
}

//Sets on/off for an LED at a specific index 0 to # of LEDS - 1.
void set_Toggle(int led_pos, uint8_t toggle){
    ToggleLED[led_pos] = toggle;
}

void send_LEDS(){
	HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *) pulse_arr, 24*LED_NUM + 40);
	data_sent = 0;
}

//Sets all LED on strip to white color
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

void hsv_to_rgb(float h, float s, float v, uint8_t* r, uint8_t* g, uint8_t* b) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf = 0, gf = 0, bf = 0;

    if (h < 1.0f/6.0f)       { rf = c; gf = x; bf = 0; }
    else if (h < 2.0f/6.0f)  { rf = x; gf = c; bf = 0; }
    else if (h < 3.0f/6.0f)  { rf = 0; gf = c; bf = x; }
    else if (h < 4.0f/6.0f)  { rf = 0; gf = x; bf = c; }
    else if (h < 5.0f/6.0f)  { rf = x; gf = 0; bf = c; }
    else                    { rf = c; gf = 0; bf = x; }

    *r = (uint8_t)((rf + m) * 255);
    *g = (uint8_t)((gf + m) * 255);
    *b = (uint8_t)((bf + m) * 255);
}

void rainbow_effect(uint32_t now){
    static float base_hue = 0.0f;
    static uint32_t last_ms = 0;

    if (now - last_ms < 20) return;
    last_ms = now;

    for (int i = 0; i < LED_NUM; i++) {
        float hue = base_hue + ((float)i / LED_NUM);
        if (hue >= 1.0f) hue -= 1.0f;

        uint8_t r, g, b;
        hsv_to_rgb(hue, 1.0f, 0.3f, &r, &g, &b);

        set_LEDS(r, g, b, i);
    }

    base_hue += 0.01f;
    if (base_hue >= 1.0f) base_hue -= 1.0f;
}

void set_digital_clock(uint8_t delimiter){
	set_digit(3, curr_time.minutes%10); //2nd digit from right, one's minute value
	set_digit(2, curr_time.minutes/10); //3nd digit from right, ten's minute value
	set_digit(1, curr_time.hours%10); //4nd digit from right, one's hour value
	set_digit(0, curr_time.hours/10); //5nd digit from right, ten's hour value

	if(delimiter){
		set_Toggle(DELIMITER_LED1, 1);
		set_Toggle(DELIMITER_LED2, 1);
	}
	else{
		set_Toggle(DELIMITER_LED1, 0);
		set_Toggle(DELIMITER_LED2, 0);
	}

	for(int i = 0; i < LED_NUM; i++){
		set_LEDS(255, 0, 0, i);
	}
}

//-------------------------------------------------------------------> DS3231 setup
void set_time(int seconds, int minutes, int hours, int week_day, int month_day, int month, int year){ //initialize the ds3231 registers
	uint8_t buffer[] = {
		dectoBCD(seconds), //seconds
		dectoBCD(minutes), //minutes
		dectoBCD(hours), //hours
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
	set_time(50, 59, 9, 1, 1, 1, 0); //edit or add API
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

//-------------------------------------------------------------------> DHT11 setup
void set_gpio_outin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t status){ //1 for output, 0 for input
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if(status == 1){
		GPIO_InitStruct.Pin = GPIO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	} else{
		GPIO_InitStruct.Pin = GPIO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	}
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void delay_us(uint16_t us){
	__HAL_TIM_SET_COUNTER(&htim3, 0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim3) < us);  // wait for the counter to reach the us input in the parameter
}

void dht11_initialization(){
	uint8_t confirm_check = 0; //bool to check if dht11 return signal is correct
	while(!confirm_check){
		set_gpio_outin(DHT11_PORT, DHT11_PIN, 1);
		HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 0);
		delay_us(18000);
		HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 1);
		delay_us(30);
		set_gpio_outin(DHT11_PORT, DHT11_PIN, 0);

		confirm_check = dht11_response();
		if(!confirm_check){
			HAL_Delay(1000);
		}
	}
}

uint8_t dht11_response(){
	uint8_t res = 0;
	delay_us(40);
	if(!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
		delay_us(80);
		if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
			res = 1;
		} else{
			res = -1;
		}
	} else{
		res = -1;
	}
	while ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)));   // wait for pin to go low
	return res;
}

uint8_t dht11_byte_read(){
	uint8_t res = 0;
	for(int i = 0; i < 8; i++){
		while(!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));
		delay_us(40);
		if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
			res |= (1 << (7-i));
			while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)));  // wait for pin to go low
		}
	}
	return res;
}

void calculate_feelslike_temp(uint8_t temp, uint8_t humidity){
	float f_temp = celsius_to_fahrenheit(temp);
	if(f_temp >= 80){
		float heat_index =
			- 42.379
			+ 2.04901523 * f_temp
			+ 10.14333127 * humidity
			- 0.22475541 * f_temp * humidity
			- 0.00683783 * f_temp * f_temp
			- 0.05481717 * humidity * humidity
			+ 0.00122874 * f_temp * f_temp * humidity
			+ 0.00085282 * f_temp * humidity * humidity
			- 0.00000199 * f_temp * f_temp * humidity * humidity;
		return (uint8_t) fahrenheit_to_celsius(heat_index);
	}
	else {
		return temp;
	}
}

float celsius_to_fahrenheit(uint8_t temp){
	return (temp * (9.0/5.0)) + 32;
}

float fahrenheit_to_celsius(uint8_t temp){
	return (temp - 32) * (5.0/9.0);
}

void set_temp(uint8_t temp){ //displays temp on clock, !!!probably only use Celsius for now
	set_digit(3, 10); //1st digit from right, temperature scale (C/F), 10 is index for celsius
	set_digit(2, 12); //2nd digit from right, temp degree symbol, 12 is index for symbol in digital table above
	set_digit(1, temp.hours%10); //3nd digit from right, one's temp value
	set_digit(0, temp.hours/10); //4th digit from right, ten's temp value
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
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  initiate_time();
  HAL_Delay(100);
  update_time();

  set_birthday_alarm(BIRTH_DAY);
  HAL_Delay(100);

  uint32_t onboard_timer = 0; //timer for onboard LED
  uint32_t clock_timer = 0;
  uint32_t rainbow_timer = 0;
  uint32_t dht11_timer = 0;

  uint8_t delimiter_toggle = 1;
  set_digital_clock(delimiter_toggle);

  if(!alarm_status){ //temp, move to BLE receive function
	  for(int i = 0; i < LED_NUM; i++){
		  set_LEDS(255, 0, 0, i);
	  }
  }

  send_LEDS();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uint32_t now_tick = HAL_GetTick();

	  if(now_tick - onboard_timer >= 500){ //onboard led debugging
		  onboard_timer = now_tick;
		  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	  }

	  if(!clock_temp_toggle && data_sent && (now_tick - clock_timer >= 1000)){
		  delimiter_toggle = !delimiter_toggle;
		  clock_timer = now_tick;
		  update_time();
		  set_digital_clock(delimiter_toggle);
		  send_LEDS();
	  }

	  if(alarm_status){
		  if(data_sent && (now_tick - rainbow_timer >= 20)) {
		      rainbow_timer = now_tick;
		      rainbow_effect(now_tick);  // Now runs at RAINBOW_DELAY_MS (~20 ms)
		      send_LEDS();
		  }
	  }

	  if (clock_temp_toggle && data_sent && now_tick - dht11_timer >= DHT11_INTERVAL) {
	      dht11_timer = now_tick;
	      rel_hum_int = 0;
	      rel_hum_dec = 0;
	      temp_int = 0;
	      temp_dec = 0;
	      check_sum = 0;
	      while(1){
			  dht11_initialization();
			  uint8_t response_check = dht11_response();
			  if(response_check == 1){
			      rel_hum_int = dht11_byte_read();
			      rel_hum_dec = dht11_byte_read();
			      temp_int = dht11_byte_read();
			      temp_dec = dht11_byte_read();
			      check_sum = dht11_byte_read();
			      if(rel_hum_int + rel_hum_dec + temp_int + temp_dec == check_sum){
					  break;
			      }
			  }
			  HAL_Delay(10);
	      }
	      uint8_t temperature = calculate_feelslike_temp(temp_int, rel_hum_int);
	      set_temp(temperature);
		  send_LEDS();
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 72-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65534;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_Data_GPIO_Port, DHT11_Data_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT11_Data_Pin */
  GPIO_InitStruct.Pin = DHT11_Data_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_Data_GPIO_Port, &GPIO_InitStruct);

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
	if(GPIO_Pin == Alarm_Trigger_Pin) {
		uint8_t ctrl_status;
		HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS << 1, 0x0F, 1, &ctrl_status, 1, 1000);
		ctrl_status &= ~(0b00000011); //clears both alarm bits
		HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS << 1, 0x0F, 1, &ctrl_status, 1, 1000);

		update_time();
		if(curr_time.month == BIRTH_MONTH && curr_time.day_month == BIRTH_DAY){
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
