/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "fft.h"
#include "oled_driver.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)//串口发送重定向
{
	HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,0xffff);
	return ch;
}

u8g2_t u8g2; // 显示器初始化结构体

uint32_t prt = 200;	    //量化显示的比例
uint8_t fall_pot[128];	//记录下落点的坐标

void display1(void);
void display2(void);
void display3(void);
void display4(void);
void display5(void);
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
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
	TIM3->PSC=FFT_PSC-1; //ADC采样 预分频值
	TIM3->ARR=FFT_ARR-1; //ADC采样 自动重装载值
	fft_Init();

  MD_OLED_RST_Set(); //显示器复位拉高
  u8g2Init(&u8g2);   //显示器调用初始化函数
	
	//初始化下落点 把下落的点 初始化为最底部显示
	for(uint8_t i=0;i<128;i++)
		fall_pot[i] = 63;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		fft_get_adc_value();
		fft_calculation();
		GetPowerMag();
		u8g2_FirstPage(&u8g2);
		do
		{
//			draw(&u8g2);
//			u8g2DrawTest(&u8g2);
//			u8g2_DrawBox(&u8g2,0,10,3,10);
//			testDrawProcess(&u8g2);
			display3();
			display4();
			
			display5();
		} while (u8g2_NextPage(&u8g2));
		
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*64柱状显示*/
void display1(void)
{
	uint16_t i = 0;
	uint8_t x = 0;
	uint8_t y = 0;
	
	/*******************显示*******************/
	for(i = 0; i < 32; i++)	//间隔的取32个频率出来显示
	{
		x = (i<<2);	//i*4
		y = 63-(fft_Struct.testOutput[x+1]*prt)-2;	//加1是为了丢掉第一个直流分量
		if(y>63) y = 63;
		
		u8g2_DrawBox(&u8g2,x,y,3,y);//画柱状图
		
		if(fall_pot[i]>y) fall_pot[i] = y;
		else
		{
			if(fall_pot[i]>63) fall_pot[i]=63;
			u8g2_DrawBox(&u8g2,x,fall_pot[i]+3,3,3);//画下落的点
			fall_pot[i] += 2 ;
		}
	}
}

/*单柱状显示*/
void display2(void)
{
	uint16_t i = 0;
	uint8_t y = 0;
	
	/*******************显示*******************/
	for(i = 1; i < 128; i++)	
	{
		y = 63-(fft_Struct.testOutput[i+1]*prt)-1;
		if(y>63) y = 63;
		
		u8g2_DrawBox(&u8g2,i,y,1,y);//画柱状图
		//画下落的点
		if(fall_pot[i]>y) fall_pot[i] = y;
		else
		{
				if(fall_pot[i]>63) fall_pot[i]=63;
				u8g2_DrawBox(&u8g2,i,fall_pot[i]+1,1,1);//画下落的点
				fall_pot[i] += 2 ;
		}
	}
}

/*32柱状显示*/
void display3(void)
{
	uint16_t i = 0;
	uint8_t x = 0;
	uint8_t y = 0;
	GetPowerMag();
	/*******************显示*******************/
	for(i = 0; i < 32; i++)	//间隔的取32个频率出来显示
	{
		x = (i<<2);	//i*4
		y = 31-(fft_Struct.testOutput[x+1]*prt)-1;	//加1是为了丢掉第一个直流分量
		if(y>31) y = 31;

		u8g2_DrawBox(&u8g2,x,y+32,3,32-y);//画柱状图

		//画下落的点
		if(fall_pot[i]>y) fall_pot[i] = y;
		else
		{
			if(fall_pot[i]>31) fall_pot[i]=31;
			u8g2_DrawBox(&u8g2,x,fall_pot[i]+32+3,3,3);//画柱状图
			fall_pot[i] += 2 ;
		}
	}
}

/*显示频率 误差 采样率*/
void display4(void)
{
	char buff[20];
	sprintf(buff,"F:%5d E:%d SF:%d",(uint16_t)MAX_F,(uint16_t)(FFT_FS/SAMPLING),(uint16_t)FFT_FS);
	u8g2_SetFont(&u8g2,u8g2_font_courB08_tf);
	u8g2_DrawStr(&u8g2,0,12,buff);
	u8g2_SendBuffer(&u8g2);
}

/*波形图*/
void display5(void)
{
	uint16_t i = 0;
	uint8_t y = 0;
	uint16_t cur_adc =0;
	uint16_t last_adc =15;
	/*******************显示*******************/
	for(i = 1; i < 128; i++)
	{
		cur_adc=fft_Struct.ADC_Digital_Value[i*2]/128;
		u8g2_DrawLine(&u8g2,i-1,last_adc,i,cur_adc);
		last_adc = cur_adc;
		cur_adc = 0;
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)//ADC转换完成回调函数
{
  AD_Flag=1;//AD_Flag标志位置
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
