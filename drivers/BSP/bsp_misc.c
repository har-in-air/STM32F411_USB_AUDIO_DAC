#include "main.h"
#include "bsp_misc.h"

void Error_Handler(void);

uint32_t GPIO_PIN[LEDn] = {
	LED1_PIN,
    LED2_PIN,
    LED3_PIN,
    LED4_PIN
};

GPIO_TypeDef* GPIO_PORT[LEDn] = {
	LED1_GPIO_PORT,
    LED2_GPIO_PORT,
    LED3_GPIO_PORT,
    LED4_GPIO_PORT
};

GPIO_TypeDef* BUTTON_PORT[BUTTONn] = {WAKEUP_BUTTON_GPIO_PORT };

const uint16_t BUTTON_PIN[BUTTONn] = {WAKEUP_BUTTON_PIN };

const uint16_t BUTTON_IRQn[BUTTONn] = {WAKEUP_BUTTON_EXTI_IRQn };


void BSP_LED_Init(Led_TypeDef Led) {
  GPIO_InitTypeDef  gpio_init_structure;

  if (Led <= LED4)  {
    gpio_init_structure.Pin   = GPIO_PIN[Led];
    gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull  = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;

    switch(Led)    {
    case LED1 :
      LED1_GPIO_CLK_ENABLE();
      break;
    case LED2 :
      LED2_GPIO_CLK_ENABLE();
      break;
    case LED3 :
      LED3_GPIO_CLK_ENABLE();
      break;
    case LED4 :
      LED4_GPIO_CLK_ENABLE();
      break;
    default :
      break;
    } 

    HAL_GPIO_Init(GPIO_PORT[Led], &gpio_init_structure);

    /* By default, turn off LED by setting a high level on corresponding GPIO */
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
  } 
}


void BSP_LED_DeInit(Led_TypeDef Led){
  GPIO_InitTypeDef  gpio_init_structure;

  if (Led <= LED4)  {
    /* DeInit the GPIO_LED pin */
    gpio_init_structure.Pin = GPIO_PIN[Led];

    /* Turn off LED */
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
    HAL_GPIO_DeInit(GPIO_PORT[Led], gpio_init_structure.Pin);
  }
}


void BSP_LED_On(Led_TypeDef Led) {
  if (Led <= LED4)  {
     HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
  }
}


void BSP_LED_Off(Led_TypeDef Led){
  if (Led <= LED4)  {
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
  }
}


void BSP_LED_Toggle(Led_TypeDef Led){
  if (Led <= LED4)  {
     HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
  }
}


void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode) {
  GPIO_InitTypeDef gpio_init_structure;

  /* Enable the BUTTON clock */
  BUTTON_GPIO_CLK_ENABLE();

  if(Button_Mode == BUTTON_MODE_GPIO)  {
    /* Configure Button pin as input */
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);
  }

  if(Button_Mode == BUTTON_MODE_EXTI)  {
    /* Configure Button pin as input with External interrupt */
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FAST;

    gpio_init_structure.Mode = GPIO_MODE_IT_RISING;

    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((IRQn_Type)(BUTTON_IRQn[Button]), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  }
}


void BSP_PB_DeInit(Button_TypeDef Button) {
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = BUTTON_PIN[Button];
    HAL_NVIC_DisableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
    HAL_GPIO_DeInit(BUTTON_PORT[Button], gpio_init_structure.Pin);
}


uint32_t BSP_PB_GetState(Button_TypeDef Button) {
  return HAL_GPIO_ReadPin(BUTTON_PORT[Button], BUTTON_PIN[Button]);
}



