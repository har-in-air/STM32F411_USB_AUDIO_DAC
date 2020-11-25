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

typedef enum {
  BUTTON_WAKEUP = 0
} Button_TypeDef;


typedef enum {
 BUTTON_MODE_GPIO = 0,
 BUTTON_MODE_EXTI = 1

} ButtonMode_TypeDef;

typedef enum {
  PB_SET = 0, 
  PB_RESET = !PB_SET
} ButtonValue_TypeDef;


#define LEDn                             ((uint8_t)4)

// 4 Leds are connected to MCU directly on PB4, PB5, PB6, PB7 
#define LED_GPIO_PORT                   ((GPIO_TypeDef*)GPIOB)

#define LED_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOB_CLK_DISABLE()

#define LED1_PIN                         ((uint32_t)GPIO_PIN_4)
#define LED2_PIN                         ((uint32_t)GPIO_PIN_5)
#define LED3_PIN                         ((uint32_t)GPIO_PIN_6)
#define LED4_PIN                         ((uint32_t)GPIO_PIN_7)

// one User button PA1
#define BUTTONn                             ((uint8_t)1)

#define USER_BUTTON_PIN                   GPIO_PIN_0
#define USER_BUTTON_GPIO_PORT             GPIOA
#define USER_BUTTON_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOA_CLK_ENABLE()
#define USER_BUTTON_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOA_CLK_DISABLE()
#define USER_BUTTON_EXTI_IRQn             EXTI0_IRQn

#define BUTTON_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOA_CLK_ENABLE()

void             BSP_LED_Init(Led_TypeDef Led);
void             BSP_LED_DeInit(Led_TypeDef Led);
void             BSP_LED_On(Led_TypeDef Led);
void             BSP_LED_Off(Led_TypeDef Led);
void             BSP_LED_Toggle(Led_TypeDef Led);
void             BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void             BSP_PB_DeInit(Button_TypeDef Button);
uint32_t         BSP_PB_GetState(Button_TypeDef Button);

#ifdef __cplusplus
}
#endif

#endif 


