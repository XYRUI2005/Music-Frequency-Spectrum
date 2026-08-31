#ifndef __MD_OLED_DRIVER_H
#define __MD_OLED_DRIVER_H

#include "stdlib.h"	  
#include "main.h"
#include "gpio.h"
#include "u8g2.h"
#include "stdio.h"

//-----------------OLED端口定义----------------  					   
#define MD_OLED_RST_Clr() HAL_GPIO_WritePin(OLED_RES_GPIO_Port,OLED_RES_Pin,GPIO_PIN_RESET) //oled 复位端口操作
#define MD_OLED_RST_Set() HAL_GPIO_WritePin(OLED_RES_GPIO_Port,OLED_RES_Pin,GPIO_PIN_SET)

//OLED控制用函数
uint8_t u8x8_byte_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,void *arg_ptr);
uint8_t u8x8_stm32_gpio_and_delay(U8X8_UNUSED u8x8_t *u8x8,U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int,U8X8_UNUSED void *arg_ptr) ;
void u8g2Init(u8g2_t *u8g2);
void draw(u8g2_t *u8g2);


void testDrawProcess(u8g2_t *u8g2);
void testShowFont(u8g2_t *u8g2);
void testDrawFrame(u8g2_t *u8g2);
void testDrawRBox(u8g2_t *u8g2);
void testDrawCircle(u8g2_t *u8g2);
void testDrawFilledEllipse(u8g2_t *u8g2);
void testDrawMulti(u8g2_t *u8g2);
void testDrawXBM(u8g2_t *u8g2);
 
void u8g2DrawTest(u8g2_t *u8g2);

#endif  
