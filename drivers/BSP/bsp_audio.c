#include <string.h>
#include "bsp_audio.h"
#include "stm32f4xx_ll_dma.h"

const uint32_t I2SFreq[3] = {44100, 48000, 96000};

#ifdef USE_MCLK_OUT // Makefile compile flag

const I2S_CLK_CONFIG I2S_Clk_Config24[3]  = {
{271, 2, 6, 0, 0x0B06EAB0}, // 44.1081
{258, 3, 3, 1, 0x0BFF6DB2}, // 47.9911
{344, 2, 3, 1, 0x17FEDB64}  // 95.9821
};

#else

const I2S_CLK_CONFIG I2S_Clk_Config24[3]  = {
{429, 4, 19, 0, 0x0B065E56}, // 44.0995  
{384, 5, 12, 1, 0x0C000000}, // 48.0000  
{424, 3, 11, 1, 0x1800ED70}  // 96.0144  
};

#endif

I2S_HandleTypeDef  haudio_i2s;
DMA_HandleTypeDef hdma_i2sTx;


static void I2Sx_Init(uint32_t AudioFreq);
static void I2Sx_DeInit(void);
static HAL_StatusTypeDef I2S_Config_I2SPR(uint32_t regVal);
void BSP_AUDIO_OUT_ChangeAudioConfig(uint32_t AudioOutOption);

/**
  * @brief  Configures the audio peripherals.
  * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_Init(uint8_t Volume, uint32_t AudioFreq) {
  I2Sx_DeInit();
  BSP_AUDIO_OUT_ClockConfig(&haudio_i2s, AudioFreq, NULL);
  
  haudio_i2s.Instance = AUDIO_I2Sx;
  if(HAL_I2S_GetState(&haudio_i2s) == HAL_I2S_STATE_RESET) {
    BSP_AUDIO_OUT_MspInit(&haudio_i2s, NULL);
    }  

  I2Sx_Init(AudioFreq);
  return AUDIO_OK;
}



/**
  * @brief  De-initialize the audio peripherals.
  * @retval None
  */
void BSP_AUDIO_OUT_DeInit(void) {
  I2Sx_DeInit();
  BSP_AUDIO_OUT_MspDeInit(&haudio_i2s, NULL);
}


/**
  * @brief  Starts playing audio stream from a data buffer for a determined size.
  * @param  pBuffer: Pointer to PCM samples buffer
  * @param  Size: number of bytes.
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_Play(uint16_t* pBuffer, uint32_t Size) {
  uint8_t ret = AUDIO_OK;
  // I2s transmit of 24bit data requires number of words
  if (HAL_I2S_Transmit_DMA(&haudio_i2s, pBuffer, Size/4) != HAL_OK)    {
      ret = AUDIO_ERROR;
    }   
  return ret;
}


/**
  * @brief  Transmit buffer via I2S interface
  * @param  pData: pointer to PCM samples buffer 
  * @param  Size: number of bytes to be written
  */
void BSP_AUDIO_OUT_ChangeBuffer(uint16_t *pData, uint16_t Size){
  // I2s transmit of 24bit data requires number of words
  HAL_I2S_Transmit_DMA(&haudio_i2s, pData, Size/4 );
}


/**
  * @brief   This function Pauses the audio file stream. In case
  *          of using DMA, the DMA Pause feature is used.
  * @warning When calling BSP_AUDIO_OUT_Pause() function for pause, only
  *          BSP_AUDIO_OUT_Resume() function should be called for resume (use of BSP_AUDIO_OUT_Play()
  *          function for resume could lead to unexpected behavior).
  * @retval  AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_Pause(void) {
  uint8_t ret = AUDIO_OK;
  if (HAL_I2S_DMAPause(&haudio_i2s) != HAL_OK)    {
    ret =  AUDIO_ERROR;
    }
  return ret;
}


/**
  * @brief  This function  Resumes the audio file stream.
  * WARNING: When calling BSP_AUDIO_OUT_Pause() function for pause, only
  *          BSP_AUDIO_OUT_Resume() function should be called for resume
  *          (use of BSP_AUDIO_OUT_Play() function for resume could lead to 
  *           unexpected behavior).
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_Resume(void) {
uint8_t ret = AUDIO_OK;
  if (HAL_I2S_DMAResume(&haudio_i2s)!= HAL_OK)    {
    ret =  AUDIO_ERROR;
    }
  return ret;
}


/**
  * @brief  Stops audio playing
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_Stop(void) {
uint8_t ret = AUDIO_OK;
  if (HAL_I2S_DMAStop(&haudio_i2s) != HAL_OK)    {
    ret = AUDIO_ERROR;
    }
  return ret;
}


/**
  * @brief  Controls the current audio volume level.
  * @param  Volume: Volume level to be set in percentage from 0% to 100% (0 for
  *         Mute and 100 for Max volume level).
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_SetVolume(uint8_t Volume){
  return AUDIO_OK;
}


/**
  * @brief  Enables or disables the MUTE mode by software
  * @param  Cmd: Could be AUDIO_MUTE_ON to mute sound or AUDIO_MUTE_OFF to
  *         unmute the codec and restore previous volume level.
  * @retval AUDIO_OK if correct communication, else wrong communication
  */
uint8_t BSP_AUDIO_OUT_SetMute(uint32_t Cmd) {
  return AUDIO_OK;
}


/**
  * @brief  Updates the audio frequency.
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @note   This API should be called after the BSP_AUDIO_OUT_Init() to adjust the
  *         audio frequency.
  */
void BSP_AUDIO_OUT_SetFrequency(uint32_t AudioFreq){ 
  BSP_AUDIO_OUT_ClockConfig(&haudio_i2s, AudioFreq, NULL);
  I2Sx_Init(AudioFreq);
}


/**
  * @brief  Changes the Audio Out Configuration.
  * @param  AudioOutOption: specifies the audio out new configuration
  *         This parameter can be any value of @ref BSP_Audio_Out_Option
  * @note   This API should be called after the BSP_AUDIO_OUT_Init() to adjust the
  *         audio out configuration.
  */
void BSP_AUDIO_OUT_ChangeAudioConfig(uint32_t AudioOutOption) { 
  if(AudioOutOption & BSP_AUDIO_OUT_CIRCULARMODE)  {
    HAL_DMA_DeInit(haudio_i2s.hdmatx);
    haudio_i2s.hdmatx->Init.Mode = DMA_CIRCULAR;
    HAL_DMA_Init(haudio_i2s.hdmatx);      
  }
  else {
    HAL_DMA_DeInit(haudio_i2s.hdmatx);
    haudio_i2s.hdmatx->Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(haudio_i2s.hdmatx);      
  }
 }


/**
 * @brief  Get size of remaining audio data to be transmitted.
 * @see
 */
uint32_t BSP_AUDIO_OUT_GetRemainingDataSize(void){
  return LL_DMA_ReadReg(AUDIO_I2Sx_DMAx_STREAM, NDTR) & 0xFFFF;
}


/**
  * @brief Tx Transfer completed callbacks
  * @param hi2s: I2S handle
  */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s){
  /* Manage the remaining file size and new address offset: This function 
     should be coded by user (its prototype is already declared in stm324xg_eval_audio.h) */  
  BSP_AUDIO_OUT_TransferComplete_CallBack();       
}


/**
  * @brief Tx Transfer Half completed callbacks
  * @param hi2s: I2S handle
  */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s){
  /* Manage the remaining file size and new address offset: This function 
     should be coded by user (its prototype is already declared in stm324xg_eval_audio.h) */  
  BSP_AUDIO_OUT_HalfTransfer_CallBack();   
}


/**
  * @brief  I2S error callbacks.
  * @param  hi2s: I2S handle
  */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s) {
  BSP_AUDIO_OUT_Error_CallBack();
}


/**
  * @brief  Manages the DMA full Transfer complete event.
  */
__weak void BSP_AUDIO_OUT_TransferComplete_CallBack(void){}


/**
  * @brief  Manages the DMA Half Transfer complete event.
  */
__weak void BSP_AUDIO_OUT_HalfTransfer_CallBack(void){}


/**
  * @brief  Manages the DMA FIFO error event.
  */
__weak void BSP_AUDIO_OUT_Error_CallBack(void)
{
}


/**
  * @brief  Initializes BSP_AUDIO_OUT MSP.
  * @param  
  * @param  Params : pointer on additional configuration parameters, can be NULL.
    // PA6     ------> I2S2_MCK
    // PB12     ------> I2S2_WS
    // PB13     ------> I2S2_CK
    // PB15     ------> I2S2_SD
  */
__weak void BSP_AUDIO_OUT_MspInit(I2S_HandleTypeDef *hi2s, void *Params)
{
  GPIO_InitTypeDef  GPIO_InitStruct = {0};  
  
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

#ifdef USE_MCLK_OUT
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  __HAL_RCC_DMA1_CLK_ENABLE();
    
  if(hi2s->Instance == SPI2)  {
    hdma_i2sTx.Instance = DMA1_Stream4;
    hdma_i2sTx.Init.Channel             = DMA_CHANNEL_0;
    hdma_i2sTx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2sTx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2sTx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2sTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; // I2S peripheral data register is 16bits
    hdma_i2sTx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_i2sTx.Init.Mode                = DMA_CIRCULAR;
    hdma_i2sTx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_i2sTx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;         
    hdma_i2sTx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_i2sTx.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma_i2sTx.Init.PeriphBurst         = DMA_PBURST_SINGLE; 
        
    __HAL_LINKDMA(hi2s, hdmatx, hdma_i2sTx);
    
    HAL_DMA_DeInit(&hdma_i2sTx);
    HAL_DMA_Init(&hdma_i2sTx);      
  }
  
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn); 
}



/**
  * @brief  Deinitializes BSP_AUDIO_OUT MSP.
  * @param 
  * @param  Params : pointer on additional configuration parameters, can be NULL.
  */
__weak void BSP_AUDIO_OUT_MspDeInit(I2S_HandleTypeDef *hi2s, void *Params)
{
  GPIO_InitTypeDef  GPIO_InitStruct;  
  
  __HAL_RCC_SPI2_CLK_DISABLE();
  
  // _I2S pins configuration: WS, SCK and SD pins
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
  HAL_GPIO_DeInit(GPIOB, GPIO_InitStruct.Pin);

#ifdef USE_MCLK_OUT
  //I2S pins configuration: MCK pin
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_DeInit(GPIOA, GPIO_InitStruct.Pin); 
#endif
  }


/**
  * @brief  Clock Config, assumes input to PLL I2S Block = 1MHz
  * @param 
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @note   This API is called by BSP_AUDIO_OUT_Init() and BSP_AUDIO_OUT_SetFrequency()
  *         Being __weak it can be overwritten by the application     
  * @param  Params : pointer on additional configuration parameters, can be NULL.
  */
__weak void BSP_AUDIO_OUT_ClockConfig(I2S_HandleTypeDef *hi2s, uint32_t AudioFreq, void *Params) {
  RCC_PeriphCLKInitTypeDef RCC_ExCLKInitStruct;
  int index = 0, freqindex = -1;
  
  for(index = 0; index < 3; index++)  {
	  if (I2SFreq[index] == AudioFreq) {
		  freqindex = index;
		  break;
	  	  }
  	  }
  uint32_t N, R, I2SDIV, ODD, MCKOE, i2s_pr;
#ifdef USE_MCLK_OUT
    MCKOE = 1;
#else        
    MCKOE = 0;
#endif
    // PLLI2S_VCO = f(VCO clock) = f(PLLI2S clock input)  (PLLI2SN/PLLM)
    // I2SCLK = f(PLLI2S clock output) = f(VCO clock) / PLLI2SR

  HAL_RCCEx_GetPeriphCLKConfig(&RCC_ExCLKInitStruct); 
  if (freqindex != -1)  {
    N = I2S_Clk_Config24[freqindex].N;
    R = I2S_Clk_Config24[freqindex].R;
    I2SDIV = I2S_Clk_Config24[freqindex].I2SDIV;
    ODD = I2S_Clk_Config24[freqindex].ODD;

    RCC_ExCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SN = N;  
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SR = R;  
    HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);     
    i2s_pr = (MCKOE<<9) | (ODD<<8) | I2SDIV;
    I2S_Config_I2SPR(i2s_pr);
    } 
  else { // Default PLL I2S configuration for 96000 Hz 24bit
    N = I2S_Clk_Config24[2].N;
    R = I2S_Clk_Config24[2].R;
    I2SDIV = I2S_Clk_Config24[2].I2SDIV;
    ODD = I2S_Clk_Config24[2].ODD;

    RCC_ExCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SN = N;
    RCC_ExCLKInitStruct.PLLI2S.PLLI2SR = R;
    HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct); 
    i2s_pr = (MCKOE<<9) | (ODD<<8) | I2SDIV;
    I2S_Config_I2SPR(i2s_pr);
  }
}


static HAL_StatusTypeDef I2S_Config_I2SPR(uint32_t regVal) {
uint32_t tickstart = 0U;
    __HAL_RCC_PLLI2S_DISABLE();
    tickstart = HAL_GetTick();
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLI2SRDY)  != RESET) {
      if((HAL_GetTick() - tickstart ) > PLLI2S_TIMEOUT_VALUE) { 
         return HAL_TIMEOUT;
         }
      }

    SPI2->I2SPR = regVal;
      
    __HAL_RCC_PLLI2S_ENABLE();
    tickstart = HAL_GetTick();
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLI2SRDY)  == RESET)    {
      if((HAL_GetTick() - tickstart ) > PLLI2S_TIMEOUT_VALUE)      {
        return HAL_TIMEOUT;
      }
    }      
   return HAL_OK;
   }
   
   
/**
  * @brief  Initializes the Audio Codec audio interface (I2S).
  * dataFormat : I2S_DATAFORMAT_16B, I2S_DATAFORMAT_24B
  * @param  AudioFreq: Audio frequency to be configured for the I2S peripheral. 
  */
static void I2Sx_Init(uint32_t AudioFreq) {
  haudio_i2s.Instance = SPI2;

  __HAL_I2S_DISABLE(&haudio_i2s);  

  haudio_i2s.Init.Mode = I2S_MODE_MASTER_TX;
  haudio_i2s.Init.Standard = I2S_STANDARD_PHILIPS;
  haudio_i2s.Init.DataFormat = I2S_DATAFORMAT_24B;
  haudio_i2s.Init.AudioFreq = AudioFreq;
  haudio_i2s.Init.CPOL = I2S_CPOL_LOW;
  haudio_i2s.Init.ClockSource = I2S_CLOCK_PLL;
#ifdef USE_MCLK_OUT
  haudio_i2s.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
#endif
  haudio_i2s.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;  

  HAL_I2S_Init(&haudio_i2s); 
}


/**
  * @brief  Deinitialize the Audio Codec audio interface (I2S).
  */
static void I2Sx_DeInit(void) {
  haudio_i2s.Instance = SPI2;
  __HAL_I2S_DISABLE(&haudio_i2s);
  HAL_I2S_DeInit(&haudio_i2s);
}

