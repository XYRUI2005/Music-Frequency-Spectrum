#ifndef __FFT_H__
#define __FFT_H__

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "stdio.h"
#include "math.h"
#include "arm_math.h"
#include "arm_const_structs.h"

/***********采样点：16、32、128、256、1024、2048、4096********************/
#define SAMPLING 256                                     //采样点数
#define arm_cfft_sR_f32_lenxxx &arm_cfft_sR_f32_len256   //调用
#define System_Clock 72000000
#define FFT_PSC 72
#define FFT_ARR 100

typedef struct{
	uint16_t ADC_Digital_Value[SAMPLING];//ADC电压数字量
	float ADC_Analog_Value[SAMPLING];//ADC电压模拟量
	float testInput[2*SAMPLING];//FFT输入数组实部、虚部各SAMPLING个点
	float testOutput[SAMPLING];//FFT输出频谱
}fft_typedef;

typedef struct{
	float MAX_Value;//最大值
	float MIN_Value;//最小值
	float AMPLITUDE_Value;//振幅
}MMA_typedef;

extern uint16_t AD_Flag;//定时器触发ADC采集中断标志位
extern float FFT_FS,MAX_F;//采样频率、频率（最大值）
extern uint16_t MAX_F_POINT;//频率（最大值点）
extern fft_typedef fft_Struct;
extern MMA_typedef MMA_ADC_Struct;

void fft_Init(void);
void fft_get_adc_value(void);
void fft_calculation(void);
void GetPowerMag(void);
void GetVoltageMMA(void);

#endif
