/*******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @brief   This file provides the Audio core functions.
  *
  *                                AUDIO Class  Description
  *          ===================================================================
  *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 44.1kHz, 48kHz, 96kHz
  *             - Bit resolution: 24
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *             - Endpoint for Sampling frequency DbgFeedbackHistory 10.14 3bytes
  ******************************************************************************
  */

#include "usbd_audio.h"
#include "usbd_ctlreq.h"
#include "bsp_audio.h"


#define AUDIO_SAMPLE_FREQ(frq) (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

 // Max packet size: (freq / 1000 + extra_samples) * channels * bytes_per_sample
 // e.g. (48000 / 1000 + 1) * 2(stereo) * 3(24bit) = 388

#define AUDIO_PACKET_SZE_24B(frq) (uint8_t)(((frq / 1000U + 1) * 2U * 3U) & 0xFFU), \
                                  (uint8_t)((((frq / 1000U + 1) * 2U * 3U) >> 8) & 0xFFU)

// Feature Unit Config
#define AUDIO_CONTROL_FEATURES (AUDIO_CONTROL_MUTE | AUDIO_CONTROL_VOL)


#define AUDIO_FB_DEFAULT 0x1800ED70 // I2S_Clk_Config24[2].nominal_fdbk (96kHz, 24bit, USE_MCLK_OUT false)

// DbgFeedbackHistory is limited to +/- 1kHz
#define  AUDIO_FB_DELTA_MAX (uint32_t)(1 << 22)

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef* pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef* pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static uint8_t* USBD_AUDIO_GetCfgDesc(uint16_t* length);
static uint8_t* USBD_AUDIO_GetDeviceQualifierDesc(uint16_t* length);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef* pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef* pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef* pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef* pdev);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef* pdev);
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef* pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef* pdev, uint8_t epnum);
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static void AUDIO_REQ_GetMax(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static void AUDIO_REQ_GetMin(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static void AUDIO_REQ_GetRes(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req);
static void AUDIO_OUT_StopAndReset(USBD_HandleTypeDef* pdev);
static void AUDIO_OUT_Restart(USBD_HandleTypeDef* pdev);
static uint8_t VOL_PERCENT(int16_t vol);


USBD_ClassTypeDef USBD_AUDIO = {
    USBD_AUDIO_Init,
    USBD_AUDIO_DeInit,
    USBD_AUDIO_Setup,
    USBD_AUDIO_EP0_TxReady,
    USBD_AUDIO_EP0_RxReady,
    USBD_AUDIO_DataIn,
    USBD_AUDIO_DataOut,
    USBD_AUDIO_SOF,
    USBD_AUDIO_IsoINIncomplete,
    USBD_AUDIO_IsoOutIncomplete,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetDeviceQualifierDesc,
};

// USB AUDIO device Configuration Descriptor
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZ] __ALIGN_END = {
    // Configuration 1
    0x09,                              /* bLength */
    USB_DESC_TYPE_CONFIGURATION,       /* bDescriptorType */
    LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ), /* wTotalLength bytes*/
    HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
    0x02, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    0x00, /* iConfiguration */
    0x80, /* bmAttributes  BUS Powered (0xC0 = self-powered) */
    0x32, /* bMaxPower = 50*2mA = 100 mA*/
    // 09 byte

    // USB Speaker Standard interface descriptor
    AUDIO_INTERFACE_DESC_SIZE,   /* bLength */
    USB_DESC_TYPE_INTERFACE,     /* bDescriptorType */
    0x00,                        /* bInterfaceNumber */
    0x00,                        /* bAlternateSetting */
    0x00,                        /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,      /* bInterfaceClass */
    AUDIO_SUBCLASS_AUDIOCONTROL, /* bInterfaceSubClass */
    AUDIO_PROTOCOL_UNDEFINED,    /* bInterfaceProtocol */
    0x00,                        /* iInterface */
    // 09 byte

    // USB Speaker Class-specific AC Interface Descriptor
    AUDIO_INTERFACE_DESC_SIZE,       /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_HEADER,            /* bDescriptorSubtype */
    0x00, /* 1.00 */                 /* bcdADC */
    0x01,
    0x27, /* wTotalLength = 39*/
    0x00,
    0x01, /* bInCollection */
    0x01, /* baInterfaceNr */
    // 09 byte

    // USB Speaker Input Terminal Descriptor
    AUDIO_INPUT_TERMINAL_DESC_SIZE,  /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_INPUT_TERMINAL,    /* bDescriptorSubtype */
    0x01,                            /* bTerminalID */
    0x01,                            /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
    0x01,
    0x00, /* bAssocTerminal */
    0x02, /* bNrChannels */
    0x03, /* wChannelConfig 0x0003  FL FR */
    0x00,
    0x00, /* iChannelNames */
    0x00, /* iTerminal */
    // 12 byte

    // USB Speaker Audio Feature Unit Descriptor
    0x09,                            /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_FEATURE_UNIT,      /* bDescriptorSubtype */
    AUDIO_OUT_STREAMING_CTRL,        /* bUnitID */
    0x01,                            /* bSourceID */
    0x01,                            /* bControlSize */
    AUDIO_CONTROL_FEATURES,          /* bmaControls(0) */
    0,                               /* bmaControls(1) */
    0x00,                            /* iTerminal */
    // 09 byte

    // USB Speaker Output Terminal Descriptor
    0x09,                            /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_CONTROL_OUTPUT_TERMINAL,   /* bDescriptorSubtype */
    0x03,                            /* bTerminalID */
    0x01,                            /* wTerminalType  0x0301*/
    0x03,
    0x00, /* bAssocTerminal */
    0x02, /* bSourceID */
    0x00, /* iTerminal */
    // 09 byte

    // USB Speaker Standard AS Interface Descriptor
    // Interface 1, Alternate Setting 0
	// Zero Bandwidth with zero endpoints, used to relinquish bandwidth
	// when audio not used.
    AUDIO_INTERFACE_DESC_SIZE,     /* bLength */
    USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
    0x01,                          /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints */
    USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass */
    AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
    AUDIO_PROTOCOL_UNDEFINED,      /* bInterfaceProtocol */
    0x00,                          /* iInterface */
    // 09 byte

    // USB Speaker Standard AS Interface Descriptor
    // Interface 1, Alternate Setting 1
	// Used when Audio Streaming is in operation
    AUDIO_INTERFACE_DESC_SIZE,     /* bLength */
    USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
    0x01,                          /* bInterfaceNumber */
    0x01,                          /* bAlternateSetting */
    0x02,                          /* bNumEndpoints - 1 output & 1 feedback */
    USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass */
    AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
    AUDIO_PROTOCOL_UNDEFINED,      /* bInterfaceProtocol */
    0x00,                          /* iInterface */
    // 09 byte

    // USB Speaker Audio Streaming Interface Descriptor
    AUDIO_STREAMING_INTERFACE_DESC_SIZE, /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    AUDIO_STREAMING_GENERAL,             /* bDescriptorSubtype */
    0x01,                                /* bTerminalLink */
    0x01,                                /* bDelay */
    0x01,                                /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/
    0x00,
    // 07 byte

    // USB Speaker Audio Type I Format Interface Descriptor
    0x11,                            /* bLength */
    AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
    AUDIO_STREAMING_FORMAT_TYPE,     /* bDescriptorSubtype */
    AUDIO_FORMAT_TYPE_I,             /* bFormatType */
    0x02,                            /* bNrChannels */
    0x03,                            /* bSubFrameSize :  3 Bytes per frame (24bits) */
    0x18,                            /* bBitResolution (24-bits per sample) */
    0x03,                            /* bSamFreqType 3 frequencies supported */
    AUDIO_SAMPLE_FREQ(44100),        /* Audio sampling frequency coded on 3 bytes */
    AUDIO_SAMPLE_FREQ(48000),        /* Audio sampling frequency coded on 3 bytes */
    AUDIO_SAMPLE_FREQ(96000),        /* Audio sampling frequency coded on 3 bytes */
    // 17 byte

    // Endpoint 1 - Standard Descriptor
	// Isochronous Async endpoint for audio packets
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE,         /* bLength */
    USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */
    AUDIO_OUT_EP,                              /* bEndpointAddress 1 out endpoint*/
    USBD_EP_TYPE_ISOC_ASYNC,                   /* bmAttributes */
    AUDIO_PACKET_SZE_24B(USBD_AUDIO_FREQ_MAX), /* wMaxPacketSize in Bytes (freq / 1000 + extra_samples) * channels * bytes_per_sample */
    0x01,                                      /* bInterval */
    0x00,                                      /* bRefresh */
    AUDIO_IN_EP,                               /* bSynchAddress */
    // 09 byte

    // Endpoint - Audio Streaming Descriptor
    AUDIO_STREAMING_ENDPOINT_DESC_SIZE, /* bLength */
    AUDIO_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType */
    AUDIO_ENDPOINT_GENERAL,             /* bDescriptor */
    0x01,                               /* bmAttributes - Sampling Frequency control is supported. See UAC Spec 1.0 p.62 */
    0x00,                               /* bLockDelayUnits */
    0x00,                               /* wLockDelay */
    0x00,
    // 07 byte

    // Endpoint 2 - Standard Descriptor - See UAC Spec 1.0 p.63 4.6.2.1 Standard AS Isochronous Synch Endpoint Descriptor
	// 3byte 10.14 sampling frequency feedback to host
    AUDIO_STANDARD_ENDPOINT_DESC_SIZE, /* bLength */
    USB_DESC_TYPE_ENDPOINT,            /* bDescriptorType */
    AUDIO_IN_EP,                       /* bEndpointAddress */
    0x11,                              /* bmAttributes */
    0x03, 0x00,                        /* wMaxPacketSize in Bytes */
    0x01,                              /* bInterval 1ms */
    SOF_RATE,                          /* bRefresh 4ms = 2^2 */
    0x00,                              /* bSynchAddress */
    // 09 byte

};

/** 
 * USB Standard Device Descriptor
 * @see https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__device__qualifier__descriptor.html
 */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

volatile uint32_t tx_flag = 1;
volatile uint32_t is_playing = 0;
volatile uint32_t all_ready = 0;

volatile uint32_t fb_nom = AUDIO_FB_DEFAULT;
volatile uint32_t fb_value = AUDIO_FB_DEFAULT;
volatile uint32_t audio_buf_writable_samples_last = AUDIO_TOTAL_BUF_SIZE /(2*6);

volatile uint8_t fb_data[3] = {
    (uint8_t)((AUDIO_FB_DEFAULT >> 8) & 0x000000FF),
    (uint8_t)((AUDIO_FB_DEFAULT >> 16) & 0x000000FF),
    (uint8_t)((AUDIO_FB_DEFAULT >> 24) & 0x000000FF)
};

// FNSOF is critical for frequency changing to work
volatile uint32_t fnsof = 0;


/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef* pdev, uint8_t cfgidx)
{
  USBD_AUDIO_HandleTypeDef* haudio;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, AUDIO_OUT_EP, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET_24B);
  pdev->ep_out[AUDIO_OUT_EP & 0xFU].is_used = 1U;

  /* Open EP IN */
  USBD_LL_OpenEP(pdev, AUDIO_IN_EP, USBD_EP_TYPE_ISOC, AUDIO_IN_PACKET);
  pdev->ep_in[AUDIO_IN_EP & 0xFU].is_used = 1U;

  /* Flush feedback endpoint */
  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);

  /** 
   * Set tx_flag 1 to block feedback transmission in SOF handler since 
   * device is not ready.
   */
  tx_flag = 1U;

  /* Allocate Audio structure */
  pdev->pClassData = USBD_malloc(sizeof(USBD_AUDIO_HandleTypeDef));

  if (pdev->pClassData == NULL) {
    return USBD_FAIL;
  } else {
    haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;
    haudio->alt_setting = 0U;
    haudio->offset = AUDIO_OFFSET_UNKNOWN;
    haudio->wr_ptr = 0U;
    haudio->rd_ptr = 0U;
    haudio->rd_enable = 0U;
    haudio->freq = USBD_AUDIO_FREQ_DEFAULT;
    haudio->bit_depth = USBD_AUDIO_BIT_DEPTH_DEFAULT;
    haudio->vol = USBD_AUDIO_VOL_DEFAULT;

    /* Initialize the Audio output Hardware layer */
    if (((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->Init(haudio->freq, VOL_PERCENT(haudio->vol), 0U) != 0) {
      return USBD_FAIL;
    }
  }
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_DeInit
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef* pdev,
                                 uint8_t cfgidx)
{
  /* Flush all endpoints */
  USBD_LL_FlushEP(pdev, AUDIO_OUT_EP);
  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);
  pdev->ep_out[AUDIO_OUT_EP & 0xFU].is_used = 0U;

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, AUDIO_IN_EP);
  pdev->ep_in[AUDIO_IN_EP & 0xFU].is_used = 0U;

  /* Clear feedback transmission flag */
  tx_flag = 0U;

  /* DeInit physical Interface components */
  if (pdev->pClassData != NULL) {
    ((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->DeInit(0U);
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef* pdev,
                                USBD_SetupReqTypedef* req)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  uint16_t len;
  uint8_t* pbuf;
  uint16_t status_info = 0U;
  uint8_t ret = USBD_OK;

  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  switch (req->bmRequest & USB_REQ_TYPE_MASK) {
    /* AUDIO Class Requests */
    case USB_REQ_TYPE_CLASS:
      switch (req->bRequest) {
        case AUDIO_REQ_GET_CUR:
          AUDIO_REQ_GetCurrent(pdev, req);
          break;

        case AUDIO_REQ_GET_MAX:
          AUDIO_REQ_GetMax(pdev, req);
          break;

        case AUDIO_REQ_GET_MIN:
          AUDIO_REQ_GetMin(pdev, req);
          break;

        case AUDIO_REQ_GET_RES:
          AUDIO_REQ_GetRes(pdev, req);
          break;

        case AUDIO_REQ_SET_CUR:
          AUDIO_REQ_SetCurrent(pdev, req);
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    /* Standard Requests */
    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest) {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            USBD_CtlSendData(pdev, (uint8_t*)(void*)&status_info, 2U);
          } else {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE) {
            pbuf = USBD_AUDIO_CfgDesc + 18;
            len = MIN(USB_AUDIO_DESC_SIZ, req->wLength);

            USBD_CtlSendData(pdev, pbuf, len);
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            USBD_CtlSendData(pdev, (uint8_t*)(void*)&haudio->alt_setting, 1U);
          } else {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES) {
              /* Do things only when alt_setting changes */
              if (haudio->alt_setting != (uint8_t)(req->wValue)) {
                haudio->alt_setting = (uint8_t)(req->wValue);
                if (haudio->alt_setting == 0U) {
                	AUDIO_OUT_StopAndReset(pdev);
                	}
                else {
                	haudio->bit_depth = USBD_AUDIO_BIT_DEPTH_DEFAULT;
                  	AUDIO_OUT_Restart(pdev);
                	}
              	}
              USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
            } else {
              /* Call the error management function (command will be nacked */
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
          } else {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;
    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return ret;
}


/**
  * @brief  USBD_AUDIO_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t* USBD_AUDIO_GetCfgDesc(uint16_t* length)
{
  *length = sizeof(USBD_AUDIO_CfgDesc);
  return USBD_AUDIO_CfgDesc;
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef* pdev,
                                 uint8_t epnum)
{
  /* epnum is the lowest 4 bits of bEndpointAddress. See UAC 1.0 spec, p.61 */
  if (epnum == (AUDIO_IN_EP & 0xf)) {
    tx_flag = 0U;
  }
  return USBD_OK;
}


#ifdef DEBUG_FEEDBACK_ENDPOINT
static volatile uint32_t  DbgMinWritableSamples = 99999;
static volatile uint32_t  DbgMaxWritableSamples = 0;
static volatile uint32_t  DbgSofHistory[256] = {0};
static volatile uint32_t  DbgWritableSampleHistory[256] = {0};
static volatile float     DbgFeedbackHistory[256] = {0};
static volatile uint8_t   DbgIndex = 0; // rollover every 256 entries
static volatile uint32_t  DbgSofCounter = 0;
#endif

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef* pdev)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;
  static volatile uint32_t sof_count = 0;

  /* Do stuff only when playing */
  if (haudio->rd_enable == 1U && all_ready == 1U) {
#ifdef DEBUG_FEEDBACK_ENDPOINT
	DbgSofCounter++;
#endif
	// Update audio read pointer
    haudio->rd_ptr = AUDIO_TOTAL_BUF_SIZE - BSP_AUDIO_OUT_GetRemainingDataSize();

    // Calculate remaining writable buffer size
    uint32_t audio_buf_writable_samples = haudio->rd_ptr < haudio->wr_ptr ?
    		  (haudio->rd_ptr + AUDIO_TOTAL_BUF_SIZE - haudio->wr_ptr)/6 : (haudio->rd_ptr - haudio->wr_ptr)/6;

    // Monitor remaining writable buffer size with LED
    if (audio_buf_writable_samples < AUDIO_BUF_SAFEZONE_SAMPLES) {
    	BSP_LED_On(LED_RED);
    	}
    else {
    	BSP_LED_Off(LED_RED);
    	}

    sof_count += 1;

    if (sof_count == 1U) {
		sof_count = 0;
		// we start transmitting to I2S DAC when the audio buffer is half full, so the optimal
		// remaining writable size is (AUDIO_TOTAL_BUF_SIZE/2)/6 samples
		// Calculate feedback value based on the deviation from optimal
		int32_t audio_buf_writable_dev_from_nom_samples = audio_buf_writable_samples - AUDIO_TOTAL_BUF_SIZE/(2*6);
		fb_value += audio_buf_writable_dev_from_nom_samples * 384;
		// Clamp feedback value to nominal value +/- 1kHz
		if (fb_value > fb_nom +  AUDIO_FB_DELTA_MAX) {
			fb_value = fb_nom +  AUDIO_FB_DELTA_MAX;
			}
		else
		if (fb_value < fb_nom -  AUDIO_FB_DELTA_MAX) {
			fb_value = fb_nom -  AUDIO_FB_DELTA_MAX;
			}

		#ifdef DEBUG_FEEDBACK_ENDPOINT
		if (audio_buf_writable_samples != audio_buf_writable_samples_last) {
			if (audio_buf_writable_samples > DbgMaxWritableSamples) DbgMaxWritableSamples = audio_buf_writable_samples;
			if (audio_buf_writable_samples < DbgMinWritableSamples) DbgMinWritableSamples = audio_buf_writable_samples;
			DbgWritableSampleHistory[DbgIndex] = audio_buf_writable_samples;
			DbgFeedbackHistory[DbgIndex] = (float)(fb_value >> 8)/(float)(1<<14);
			DbgSofHistory[DbgIndex] = DbgSofCounter;
			DbgIndex++; // uint8_t, so only record last 256 entries
			}

		if (BtnPressed) {
			printMsg("DbgOptimalWritableSamples = %d\r\nDbgSafeZoneWritableSamples = %d\r\n", AUDIO_TOTAL_BUF_SIZE/(2*6), AUDIO_BUF_SAFEZONE_SAMPLES);
			printMsg("DbgMaxWritableSamples = %d\r\nDbgMinWritableSamples = %d\r\n", DbgMaxWritableSamples, DbgMinWritableSamples);
			BtnPressed = 0;
			int count = 256;
			while (count--){
				// print oldest to newest
				//printMsg("%d %d %f\r\n", DbgSofHistory[DbgIndex], DbgWritableSampleHistory[DbgIndex], DbgFeedbackHistory[DbgIndex]);
				DbgIndex++;
				}
			}
		#endif

		// Update last writable buffer size
		audio_buf_writable_samples_last = audio_buf_writable_samples;
		// Set 10.14 format feedback data
		// Order of 3 bytes in feedback packet: { LO byte, MID byte, HI byte }
		// 48.0 (dec) => 300000(hex, 8.16) => 0C0000(hex, 10.14) => packet { 00, 00, 0C }
		// Note that ALSA accepts 8.16 format.
		fb_data[0] = (uint8_t)((fb_value >> 8) & 0x000000FF);
		fb_data[1] = (uint8_t)((fb_value >> 16) & 0x000000FF);
		fb_data[2] = (uint8_t)((fb_value >> 24) & 0x000000FF);
		}


    /* Transmit feedback only when the last one is transmitted */
    if (tx_flag == 0U) {
      /* Get FNSOF. Use volatile for fnsof_new since its address is mapped to a hardware register. */
      USB_OTG_GlobalTypeDef* USBx = USB_OTG_FS;
      uint32_t USBx_BASE = (uint32_t)USBx;
      uint32_t volatile fnsof_new = (USBx_DEVICE->DSTS & USB_OTG_DSTS_FNSOF) >> 8;

      if ((fnsof & 0x1) == (fnsof_new & 0x1)) {
        USBD_LL_Transmit(pdev, AUDIO_IN_EP, (uint8_t*)fb_data, 3U);
        /* Block transmission until it's finished. */
        tx_flag = 1U;
      }
    }
  }

  return USBD_OK;
}


/**
  * @brief  USBD_AUDIO_Sync
  *         handle Sync event called from usbd_audio_if.c
  * @param  pdev: device instance
  * @retval status
  */
void USBD_AUDIO_Sync(USBD_HandleTypeDef* pdev, AUDIO_OffsetTypeDef offset)
{
}

/**
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * USBD_AUDIO_IsoINIncomplete & USBD_AUDIO_IsoOutIncomplete are not 
 * enabled by default.
 * 
 * Go to Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c
 * Fill in USBD_LL_IsoINIncomplete and USBD_LL_IsoOUTIncomplete with 
 * actual handler functions.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/**
  * @brief  USBD_AUDIO_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef* pdev, uint8_t epnum)
{
  USB_OTG_GlobalTypeDef* USBx = USB_OTG_FS;
  uint32_t USBx_BASE = (uint32_t)USBx;
  fnsof = (USBx_DEVICE->DSTS & USB_OTG_DSTS_FNSOF) >> 8;

  if (tx_flag == 1U) {
    tx_flag = 0U;
    USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef* pdev, uint8_t epnum)
{
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef* pdev,  uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  static uint8_t tmpbuf[1024];

  if (all_ready == 1U && epnum == AUDIO_OUT_EP) {
    uint32_t curr_length = USBD_GetRxCount(pdev, epnum);

    /* Ignore strangely large packets */
    if (curr_length > AUDIO_OUT_PACKET_24B) {
      curr_length = 0U;
    }

    uint32_t tmpbuf_ptr = 0U;
    uint32_t num_of_samples = curr_length / 6; // 3bytes per sample

    for (int i = 0; i < num_of_samples; i++) {
    	// I2S transmit buffer expects {HiByte:MidByte, LoByte:00} for each 24-bit sample
    	// { 0: L_LOBYTE, 1: L_MDBYTE, 2: L_HIBYTE, 3: R_LOBYTE, 4: R_MDBYTE, 5: R_HIBYTE }
        haudio->buffer[haudio->wr_ptr++] = (((uint16_t)tmpbuf[tmpbuf_ptr+2]) << 8) | (uint16_t)tmpbuf[tmpbuf_ptr+1];
        haudio->buffer[haudio->wr_ptr++] = ((uint16_t)tmpbuf[tmpbuf_ptr]) << 8;

        haudio->buffer[haudio->wr_ptr++] = (((uint16_t)tmpbuf[tmpbuf_ptr+5]) << 8) | (uint16_t)tmpbuf[tmpbuf_ptr+4];
        haudio->buffer[haudio->wr_ptr++] = ((uint16_t)tmpbuf[tmpbuf_ptr+3]) << 8;
        tmpbuf_ptr += 6;

        // Rollover at end of buffer
        if (haudio->wr_ptr >= AUDIO_TOTAL_BUF_SIZE) {
        	haudio->wr_ptr = 0U;
      	  	}
    	}

    // Start playing when half of the audio buffer is filled
    // so if you increase the buffer length too much, the audio latency will be obvious when watching video+audio
    if (haudio->offset == AUDIO_OFFSET_UNKNOWN && is_playing == 0U) {
      if (haudio->wr_ptr >= AUDIO_TOTAL_BUF_SIZE / 2U) {
        haudio->offset = AUDIO_OFFSET_NONE;
        is_playing = 1U;

        if (haudio->rd_enable == 0U) {
          haudio->rd_enable = 1U;
          // Set last writable buffer size to actual value. Note that rd_ptr is 0 now.
          audio_buf_writable_samples_last = (AUDIO_TOTAL_BUF_SIZE - haudio->wr_ptr)/6;
        }

        ((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->AudioCmd(&haudio->buffer[0], AUDIO_TOTAL_BUF_SIZE * 2, AUDIO_CMD_START);
      }
    }

    USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, tmpbuf, AUDIO_OUT_PACKET_24B);
  }

  return USBD_OK;
}


/**
 * @brief  AUDIO_Req_GetCurrent
 *         Handles the GET_CUR Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ) {
    switch (HIBYTE(req->wValue)) {
      case AUDIO_CONTROL_REQ_FU_MUTE: {
        /* Current mute state */
        uint8_t mute = 0;
        USBD_CtlSendData(pdev, &mute, 1);
      };
          break;
      case AUDIO_CONTROL_REQ_FU_VOL: {
        /* Current volume. See UAC Spec 1.0 p.77 */
        USBD_CtlSendData(pdev, (uint8_t*)&haudio->vol, 2);
      };
          break;
    }
  } else if ((req->bmRequest & 0x1f) == AUDIO_STREAMING_REQ) {
    if (HIBYTE(req->wValue) == AUDIO_STREAMING_REQ_FREQ_CTRL) {
      /* Current frequency */
      uint32_t freq __attribute__((aligned(4))) = haudio->freq;
      USBD_CtlSendData(pdev, (uint8_t*)&freq, 3);
    }
  }
}


/**
 * @brief  AUDIO_Req_GetMax
 *         Handles the GET_MAX Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetMax(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req)
{
  if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ) {
    switch (HIBYTE(req->wValue)) {
      case AUDIO_CONTROL_REQ_FU_VOL: {
        int16_t vol_max = USBD_AUDIO_VOL_MAX;
        USBD_CtlSendData(pdev, (uint8_t*)&vol_max, 2);
      };
          break;
    }
  }
}


/**
 * @brief  AUDIO_Req_GetMin
 *         Handles the GET_MIN Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetMin(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req)
{
  if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ) {
    switch (HIBYTE(req->wValue)) {
      case AUDIO_CONTROL_REQ_FU_VOL: {
        int16_t vol_min = USBD_AUDIO_VOL_MIN;
        USBD_CtlSendData(pdev, (uint8_t*)&vol_min, 2);
      };
          break;
    }
  }
}


/**
 * @brief  AUDIO_Req_GetRes
 *         Handles the GET_RES Audio control request.
 * @param  pdev: instance
 * @param  req: setup class request
 * @retval status
 */
static void AUDIO_REQ_GetRes(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req)
{
  if ((req->bmRequest & 0x1f) == AUDIO_CONTROL_REQ) {
    switch (HIBYTE(req->wValue)) {
      case AUDIO_CONTROL_REQ_FU_VOL: {
        int16_t vol_res = USBD_AUDIO_VOL_STEP;
        USBD_CtlSendData(pdev, (uint8_t*)&vol_res, 2);
      };
          break;
    }
  }
}


/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef* pdev, USBD_SetupReqTypedef* req)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  if (req->wLength) {
    /* Prepare the reception of the buffer over EP0 */
    USBD_CtlPrepareRx(pdev,
                      haudio->control.data,
                      req->wLength);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;          /* Set the request value */
    haudio->control.req_type = req->bmRequest & 0x1f; /* Set the request type. See UAC Spec 1.0 - 5.2.1 Request Layout */
    haudio->control.len = (uint8_t)req->wLength;      /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);       /* Set the request target unit */
    haudio->control.cs = HIBYTE(req->wValue);         /* Set the request control selector (high byte) */
    haudio->control.cn = LOBYTE(req->wValue);         /* Set the request control number (low byte) */
  }
}


/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef* pdev)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR) { /* In this driver, to simplify code, only SET_CUR request is managed */

    if (haudio->control.req_type == AUDIO_CONTROL_REQ) {
      switch (haudio->control.cs) {
        /* Mute Control */
        case AUDIO_CONTROL_REQ_FU_MUTE: {
          ((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->MuteCtl(haudio->control.data[0]);
        };
            break;
        /* Volume Control */
        case AUDIO_CONTROL_REQ_FU_VOL: {
          int16_t vol = *(int16_t*)&haudio->control.data[0];
          haudio->vol = vol;
          ((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->VolumeCtl(VOL_PERCENT(vol));
        };
            break;
      }

    } else if (haudio->control.req_type == AUDIO_STREAMING_REQ) {
      /* Frequency Control */
      if (haudio->control.cs == AUDIO_STREAMING_REQ_FREQ_CTRL) {
        uint32_t new_freq = *(uint32_t*)&haudio->control.data & 0x00ffffff;

        if (haudio->freq != new_freq) {
          haudio->freq = new_freq;
          AUDIO_OUT_Restart(pdev);
        }
      }
    }

    haudio->control.req_type = 0U;
    haudio->control.cs = 0U;
    haudio->control.cn = 0U;
    haudio->control.cmd = 0U;
    haudio->control.len = 0U;
  }

  return USBD_OK;
}


/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef* pdev)
{
  /* Only OUT control data are processed */
  return USBD_OK;
}


/**
 * @brief  Stop playing and reset buffer pointers
 * @param  pdev: instance
 */
static void AUDIO_OUT_StopAndReset(USBD_HandleTypeDef* pdev)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  all_ready = 0U;
  tx_flag = 1U;
  is_playing = 0U;
  audio_buf_writable_samples_last = AUDIO_TOTAL_BUF_SIZE /(2*6);

  haudio->offset = AUDIO_OFFSET_UNKNOWN;
  haudio->rd_enable = 0U;
  haudio->rd_ptr = 0U;
  haudio->wr_ptr = 0U;

  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
  USBD_LL_FlushEP(pdev, AUDIO_OUT_EP);

  ((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->DeInit(0);
}


/**
 * @brief  Restart playing with new parameters
 * @param  pdev: instance
 */
static void AUDIO_OUT_Restart(USBD_HandleTypeDef* pdev)
{
  USBD_AUDIO_HandleTypeDef* haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;

  AUDIO_OUT_StopAndReset(pdev);

  switch (haudio->freq) {
    case 44100:
      fb_nom = fb_value = I2S_Clk_Config24[0].nominal_fdbk;
      break;
    case 48000:
      fb_nom = fb_value = I2S_Clk_Config24[1].nominal_fdbk;
      break;
    case 96000:
    default :
      fb_nom = fb_value = I2S_Clk_Config24[2].nominal_fdbk;
      break;
  }

  ((USBD_AUDIO_ItfTypeDef*)pdev->pUserData)->Init(haudio->freq, VOL_PERCENT(haudio->vol), 0);

  tx_flag = 0U;
  all_ready = 1U;
}


/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t* USBD_AUDIO_GetDeviceQualifierDesc(uint16_t* length)
{
  *length = sizeof(USBD_AUDIO_DeviceQualifierDesc);
  return USBD_AUDIO_DeviceQualifierDesc;
}


/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef* pdev,
                                     USBD_AUDIO_ItfTypeDef* fops)
{
  if (fops != NULL) {
    pdev->pUserData = fops;
  }
  return USBD_OK;
}


/* Convert USB volume value to % */
uint8_t VOL_PERCENT(int16_t vol)
{
  return (uint8_t)((vol - (int16_t)USBD_AUDIO_VOL_MIN) / (((int16_t)USBD_AUDIO_VOL_MAX - (int16_t)USBD_AUDIO_VOL_MIN) / 100));
}



