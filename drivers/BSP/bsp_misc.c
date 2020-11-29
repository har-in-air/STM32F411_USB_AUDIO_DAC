#include "main.h"
#include "bsp_misc.h"

void Error_Handler(void);

uint32_t GPIO_PIN[4] = {
	LED1_PIN,
    LED2_PIN,
    LED3_PIN,
    LED4_PIN
};


volatile uint32_t BtnPressed = 0;

void BSP_LED_Init(Led_TypeDef Led) {
  GPIO_InitTypeDef  gpio_init_structure = {0};

  if (Led <= LED4)  {
    gpio_init_structure.Pin   = GPIO_PIN[Led];
    gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull  = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_LOW;

    LED_GPIO_CLK_ENABLE();

    HAL_GPIO_Init(LED_GPIO_PORT, &gpio_init_structure);

    // turn off LED by setting a high level on pin
    HAL_GPIO_WritePin(LED_GPIO_PORT, GPIO_PIN[Led], GPIO_PIN_SET);
  } 
}


void BSP_LED_DeInit(Led_TypeDef Led){
  GPIO_InitTypeDef  gpio_init_structure = {0};

  if (Led <= LED4)  {
    gpio_init_structure.Pin = GPIO_PIN[Led];
    HAL_GPIO_WritePin(LED_GPIO_PORT, GPIO_PIN[Led], GPIO_PIN_SET);
    HAL_GPIO_DeInit(LED_GPIO_PORT, gpio_init_structure.Pin);
    }
}


void BSP_LED_On(Led_TypeDef Led) {
  if (Led <= LED4)  {
     HAL_GPIO_WritePin(LED_GPIO_PORT, GPIO_PIN[Led], GPIO_PIN_RESET);
  }
}


void BSP_LED_Off(Led_TypeDef Led){
  if (Led <= LED4)  {
    HAL_GPIO_WritePin(LED_GPIO_PORT, GPIO_PIN[Led], GPIO_PIN_SET);
  }
}


void BSP_LED_Toggle(Led_TypeDef Led){
  if (Led <= LED4)  {
     HAL_GPIO_TogglePin(LED_GPIO_PORT, GPIO_PIN[Led]);
  }
}


void BSP_PB_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}



uint32_t BSP_PB_GetState(void) {
  return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
}


void HAL_GPIO_EXTI_Callback(uint16_t gpioPin) {
	if (gpioPin == GPIO_PIN_1) {
		BtnPressed = 1;
		}
	}

