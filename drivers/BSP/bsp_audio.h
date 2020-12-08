#ifndef __BSP_AUDIO_H
#define __BSP_AUDIO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdlib.h>

#include "main.h"
#include "bsp_misc.h"


typedef struct I2S_CLK_CONFIG_ {
	uint32_t N;
	uint32_t R;
	uint32_t I2SDIV;
	uint32_t ODD;
	uint32_t nominal_fdbk;
} I2S_CLK_CONFIG;

extern const I2S_CLK_CONFIG I2S_Clk_Config24[];

#define BSP_AUDIO_OUT_CIRCULARMODE      ((uint32_t)0x00000001) /* BUFFER CIRCULAR MODE */
#define BSP_AUDIO_OUT_NORMALMODE        ((uint32_t)0x00000002) /* BUFFER NORMAL MODE   */
#define BSP_AUDIO_OUT_STEREOMODE        ((uint32_t)0x00000004) /* STEREO MODE          */
#define BSP_AUDIO_OUT_MONOMODE          ((uint32_t)0x00000008) /* MONO MODE            */

#define AUDIO_MUTE_ON                 1
#define AUDIO_MUTE_OFF                0

#define AUDIO_MUTE_PIN						GPIO_PIN_8
#define AUDIO_MUTE_PORT						GPIOA
#define AUDIO_MUTE_PORT_ENABLE()		    __HAL_RCC_GPIOA_CLK_ENABLE()

/* I2S peripheral configuration defines */
#define AUDIO_I2Sx                          SPI2
#define AUDIO_I2Sx_CLK_ENABLE()             __HAL_RCC_SPI2_CLK_ENABLE()
#define AUDIO_I2Sx_CLK_DISABLE()            __HAL_RCC_SPI2_CLK_DISABLE()   
#define AUDIO_I2Sx_SCK_SD_WS_AF             GPIO_AF5_SPI2
#define AUDIO_I2Sx_SCK_SD_WS_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define AUDIO_I2Sx_MCK_CLK_ENABLE()         __HAL_RCC_GPIOA_CLK_ENABLE()
#define AUDIO_I2Sx_WS_PIN                   GPIO_PIN_12
#define AUDIO_I2Sx_SCK_PIN                  GPIO_PIN_13
#define AUDIO_I2Sx_SD_PIN                   GPIO_PIN_15
#define AUDIO_I2Sx_MCK_PIN                  GPIO_PIN_6
#define AUDIO_I2Sx_SCK_SD_WS_GPIO_PORT      GPIOB
#define AUDIO_I2Sx_MCK_GPIO_PORT            GPIOA

/* I2S DMA Stream definitions */
#define AUDIO_I2Sx_DMAx_CLK_ENABLE()        __HAL_RCC_DMA1_CLK_ENABLE()
#define AUDIO_I2Sx_DMAx_STREAM              DMA1_Stream4
#define AUDIO_I2Sx_DMAx_CHANNEL             DMA_CHANNEL_0
#define AUDIO_I2Sx_DMAx_IRQ                 DMA1_Stream4_IRQn
#define AUDIO_I2Sx_DMAx_PERIPH_DATA_SIZE    DMA_PDATAALIGN_HALFWORD
#define AUDIO_I2Sx_DMAx_MEM_DATA_SIZE       DMA_MDATAALIGN_HALFWORD
#define DMA_MAX_SZE                         0xFFFF
   
#define AUDIO_I2Sx_DMAx_IRQHandler          DMA1_Stream4_IRQHandler

#define AUDIO_IRQ_PREPRIO           	5   // DMA int preemption priority level(0 is the highest)

#define AUDIODATA_SIZE                      4   // 24-bit audio sample in 32-bit frame

// Audio status definition
#define AUDIO_OK                            ((uint8_t)0)
#define AUDIO_ERROR                         ((uint8_t)1)
#define AUDIO_TIMEOUT                       ((uint8_t)2)


uint8_t BSP_AUDIO_OUT_Init(uint8_t Volume, uint32_t AudioFreq);
uint8_t BSP_AUDIO_OUT_Play(uint16_t* pBuffer, uint32_t Size);
void    BSP_AUDIO_OUT_ChangeBuffer(uint16_t *pData, uint16_t Size);
uint8_t BSP_AUDIO_OUT_Pause(void);
uint8_t BSP_AUDIO_OUT_Resume(void);
uint8_t BSP_AUDIO_OUT_Stop(void);
uint8_t BSP_AUDIO_OUT_SetVolume(uint8_t Volume);
void    BSP_AUDIO_OUT_SetFrequency(uint32_t AudioFreq);
uint8_t BSP_AUDIO_OUT_SetMute(uint32_t Cmd);
void    BSP_AUDIO_OUT_DeInit(void);
uint32_t BSP_AUDIO_OUT_GetRemainingDataSize(void);

/* User Callbacks: user has to implement these functions in his code if they are needed. */
/* This function is called when the requested data has been completely transferred.*/
void    BSP_AUDIO_OUT_TransferComplete_CallBack(void);

/* This function is called when half of the requested buffer has been transferred. */
void    BSP_AUDIO_OUT_HalfTransfer_CallBack(void);

/* This function is called when an Interrupt due to transfer error on or peripheral
   error occurs. */
void    BSP_AUDIO_OUT_Error_CallBack(void);

/* These function can be modified in case the current settings (e.g. DMA stream)
   need to be changed for specific application needs */
void  BSP_AUDIO_OUT_ClockConfig(I2S_HandleTypeDef *hi2s, uint32_t AudioFreq, void *Params);
void  BSP_AUDIO_OUT_MspInit(I2S_HandleTypeDef *hi2s, void *Params);
void  BSP_AUDIO_OUT_MspDeInit(I2S_HandleTypeDef *hi2s, void *Params);


#ifdef __cplusplus
}
#endif

#endif

