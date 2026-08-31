#include "fft.h"

float FFT_FS,MAX_F;
uint16_t AD_Flag;//定时器触发ADC采集中断标志位
uint16_t MAX_F_POINT;
fft_typedef fft_Struct;
MMA_typedef MMA_ADC_Struct;

/**
* @brief  ADC采集部分
* @param  无
* @retval 无
**/
void fft_Init(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1);//ADC启动校准电压
	HAL_TIM_Base_Start(&htim3);         //开启TIM
	HAL_ADC_Start_DMA(&hadc1,(uint32_t *)fft_Struct.ADC_Digital_Value,1);//开启ADC转换以及DMA传输，1是DMA传输的次数
}

/**
* @brief  ADC电压采集
* @param  无
* @retval 无
**/
void fft_get_adc_value(void)
{
	uint16_t i=0;
	while(i<SAMPLING)//通过ADC转换完成事件触发TIM3定时器中断 while循环读取SAMPLING个ADC值
	{
		if(AD_Flag==1)
		{
			i++;
			AD_Flag=0;//清除ADC中断标志位
			fft_Struct.ADC_Digital_Value[i]=HAL_ADC_GetValue(&hadc1);//获取ADC值
		}
	}
	for(uint16_t i=0;i<SAMPLING;i++)//将获取到的ADC值转换为模拟量电压值
	{
		fft_Struct.ADC_Analog_Value[i]=(float)(fft_Struct.ADC_Digital_Value[i])*3.3/4096;
		printf("%02f\r\n",fft_Struct.ADC_Analog_Value[i]);
	}
}

/**
* @brief  傅里叶变换
* @param  无
* @retval 无
**/
void fft_calculation(void)
{
	for(uint16_t i=0;i<SAMPLING;i++)
	{
		fft_Struct.testInput[i*2]=fft_Struct.ADC_Analog_Value[i];//实部为电压值
		fft_Struct.testInput[i*2+1]=0;//虚部补零
	}
	arm_cfft_f32(arm_cfft_sR_f32_lenxxx, fft_Struct.testInput, 0, 1);//FFT运算
	arm_cmplx_mag_f32(fft_Struct.testInput, fft_Struct.testOutput, SAMPLING);//把计算结果复数求模得幅值
	fft_Struct.testOutput[0]/=SAMPLING;//信号的直流分量
	for(int i=1;i<SAMPLING;i++)
		fft_Struct.testOutput[i]/=(SAMPLING/2);//信号各频域的幅值
//	for(int i=0;i<SAMPLING;i++)
//		printf("%02f\r\n",fft_Struct.testOutput[i]);
}

/**
* @brief  算出频率（最大值）
* @param  无
* @retval 无
**/
void GetPowerMag(void)
{
	float Mag;
	FFT_FS=System_Clock/(FFT_PSC*FFT_ARR);//采样率即频谱分辨率
	for(uint16_t i=1;i<SAMPLING/2;i++)
	{
		if(Mag<fft_Struct.testOutput[i]){Mag=fft_Struct.testOutput[i];MAX_F_POINT =i;}
	}
	MAX_F=((FFT_FS/SAMPLING)*MAX_F_POINT);
//	printf("Freq:%02fHZ  Erro:%02f  Sample_Freq:%02f\n\r",MAX_F,(FFT_FS/SAMPLING),FFT_FS);
}

/**
* @brief  算出电压（最大值、最小值、幅值）
* @param  无
* @retval 无
**/
void GetVoltageMMA(void)
{
	MMA_ADC_Struct.MAX_Value=fft_Struct.ADC_Analog_Value[0];
	MMA_ADC_Struct.MIN_Value=fft_Struct.ADC_Analog_Value[0];
	for(uint16_t i=0;i<SAMPLING;i++)
	{
		if(fft_Struct.ADC_Analog_Value[i] >= MMA_ADC_Struct.MAX_Value)
		{	    
			MMA_ADC_Struct.MAX_Value = fft_Struct.ADC_Analog_Value[i];
		}			
		if(fft_Struct.ADC_Analog_Value[i] <= MMA_ADC_Struct.MIN_Value)
		{		    
			MMA_ADC_Struct.MIN_Value = fft_Struct.ADC_Analog_Value[i];
		}						
	}
	MMA_ADC_Struct.MIN_Value=MMA_ADC_Struct.MIN_Value;//最大值、最小值、幅值
	MMA_ADC_Struct.MAX_Value=MMA_ADC_Struct.MAX_Value;
	MMA_ADC_Struct.AMPLITUDE_Value=MMA_ADC_Struct.MAX_Value - MMA_ADC_Struct.MIN_Value ;
	
//	printf("MAX:%f02V  MIN:%f02V  AMP:%f02Vpp\n\r",MMA_ADC_Struct.MAX_Value,MMA_ADC_Struct.MIN_Value,MMA_ADC_Struct.AMPLITUDE_Value);
}


