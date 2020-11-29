#ifndef __BSP_MISC_H
#define __BSP_MISC_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx_hal.h"

typedef enum {
 LED1 = 0,
 LED_YELLOW = LED1,
 LED2 = 1,
 LED_RED = LED2,
 LED3 = 2,
 LED_GREEN = LED3,
 LED4 = 3,
 LED_BLUE = LED4
} Led_TypeDef;


extern volatile uint32_t BtnPressed;

// 4 Leds are connected to MCU directly on PB4, PB5, PB6, PB7 
#define LED_GPIO_PORT                   GPIOB

#define LED_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()

#define LED1_PIN                         GPIO_PIN_4
#define LED2_PIN                         GPIO_PIN_5
#define LED3_PIN                         GPIO_PIN_6
#define LED4_PIN                         GPIO_PIN_7


void BSP_LED_Init(Led_TypeDef Led);
void BSP_LED_DeInit(Led_TypeDef Led);
void BSP_LED_On(Led_TypeDef Led);
void BSP_LED_Off(Led_TypeDef Led);
void BSP_LED_Toggle(Led_TypeDef Led);
void BSP_PB_Init(void);
uint32_t BSP_PB_GetState(void);

#ifdef __cplusplus
}
#endif

#endif 


