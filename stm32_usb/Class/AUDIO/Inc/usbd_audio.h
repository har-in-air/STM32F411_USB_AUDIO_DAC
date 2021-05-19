/**
  ******************************************************************************
  * @file    usbd_audio.h
  * @author  MCD Application Team
  * @brief   header file for the usbd_audio.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      http://www.st.com/SLA0044
  *
  ******************************************************************************
  */

#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include  "usbd_ioreq.h"


#ifndef USBD_AUDIO_FREQ_DEFAULT
#define USBD_AUDIO_FREQ_DEFAULT                       96000U
#endif

#ifndef USBD_AUDIO_FREQ_MAX
#define USBD_AUDIO_FREQ_MAX                           96000U
#endif

// See USB Device Class Definition for Audio Devices v1.0 p.77
 // max volume is 0dB, this is to avoid clipping
 #ifndef USBD_AUDIO_VOL_MAX
 #define USBD_AUDIO_VOL_MAX                            0x0000U
 #endif

 //0x100 = 1dB, 0x8200 = -32256 =  -126dB
 #ifndef USBD_AUDIO_VOL_MIN
 #define USBD_AUDIO_VOL_MIN                            0x8200U
 #endif

 #ifndef USBD_AUDIO_VOL_DEFAULT
 #define USBD_AUDIO_VOL_DEFAULT                        0x8D00U
 #endif

 // 6dB step resolution
 #ifndef USBD_AUDIO_VOL_STEP
 #define USBD_AUDIO_VOL_STEP                           0x0600U
 #endif

 // default mute state is on (muted)
#ifndef USBD_AUDIO_MUTE_DEFAULT
#define USBD_AUDIO_MUTE_DEFAULT                     	0x00U
#endif

/* Interface */
#ifndef USBD_MAX_NUM_INTERFACES
#define USBD_MAX_NUM_INTERFACES                       1U
#endif

/* bEndpointAddress, see UAC 1.0 spec, p.61 */
#define AUDIO_OUT_EP                                  0x01U
#define AUDIO_IN_EP                                   0x81U

#define SOF_RATE                                      0x02U

#define USB_AUDIO_CONFIG_DESC_SIZ                     124

#define AUDIO_INTERFACE_DESC_SIZE                     0x09U
#define USB_AUDIO_DESC_SIZ                            0x09U
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x09U
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x07U

#define AUDIO_DESCRIPTOR_TYPE                         0x21U
#define USB_DEVICE_CLASS_AUDIO                        0x01U
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01U
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02U
#define AUDIO_PROTOCOL_UNDEFINED                      0x00U
#define AUDIO_STREAMING_GENERAL                       0x01U
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02U

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24U
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25U

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01U
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02U
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03U
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06U

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0CU
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09U
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07U

#define AUDIO_CONTROL_MUTE                            0x0001U
#define AUDIO_CONTROL_VOL                             0x0002U

#define AUDIO_FORMAT_TYPE_I                           0x01U
#define AUDIO_FORMAT_TYPE_III                         0x03U

#define AUDIO_ENDPOINT_GENERAL                        0x01U

/* Audio Requests */
#define AUDIO_REQ_GET_CUR                             0x81U
#define AUDIO_REQ_GET_MIN                             0x82U
#define AUDIO_REQ_GET_MAX                             0x83U
#define AUDIO_REQ_GET_RES                             0x84U
#define AUDIO_REQ_SET_CUR                             0x01U
#define AUDIO_REQ_SET_MIN                             0x02U
#define AUDIO_REQ_SET_MAX                             0x03U
#define AUDIO_REQ_SET_RES                             0x04U

#define AUDIO_OUT_STREAMING_CTRL                      0x02U

/* Audio Control Requests */
#define AUDIO_CONTROL_REQ                             0x01U
/* Feature Unit, UAC Spec 1.0 p.102 */
#define AUDIO_CONTROL_REQ_FU_MUTE                     0x01U
#define AUDIO_CONTROL_REQ_FU_VOL                      0x02U

/* Audio Streaming Requests */
#define AUDIO_STREAMING_REQ                           0x02U
#define AUDIO_STREAMING_REQ_FREQ_CTRL                 0x01U
#define AUDIO_STREAMING_REQ_PITCH_CTRL                0x02U


// Max packet size: (freq / 1000 + extra_samples) * channels * bytes_per_sample
// e.g. 96kHz, 24bit : (96000 / 1000 + 1) * 2(stereo) * 3(24bit) = 582 bytes

#define AUDIO_OUT_PACKET_24B                          ((uint16_t)((USBD_AUDIO_FREQ_MAX / 1000U + 1) * 2U * 3U))

/* Input endpoint is for feedback. See USB 1.1 Spec, 5.10.4.2 Feedback. */
#define AUDIO_IN_PACKET                               3U

// Number of sub-packets in the audio transfer buffer.
// You can modify this value but always make sure that it is an even number higher than 3.
// Larger values will increase latency since we start playing only when the buffer is half-full
#define AUDIO_OUT_PACKET_NUM                          8U

// Total size of the audio transfer buffer
#define AUDIO_TOTAL_BUF_SIZE                          ((uint16_t)((USBD_AUDIO_FREQ_MAX / 1000U + 1) * 2U * 3U * AUDIO_OUT_PACKET_NUM))


// The minimum distance between rd_ptr and wr_ptr to prevent overwriting unplayed buffer

#define AUDIO_BUF_SAFEZONE_SAMPLES                    ((USBD_AUDIO_FREQ_MAX / 1000U) + 1)

    /* Audio Commands enumeration */
typedef enum
{
  AUDIO_CMD_START = 1,
  AUDIO_CMD_PLAY,
  AUDIO_CMD_STOP,
} AUDIO_CMD_TypeDef;


typedef enum
{
  AUDIO_OFFSET_NONE = 0,
  AUDIO_OFFSET_HALF,
  AUDIO_OFFSET_FULL,
  AUDIO_OFFSET_UNKNOWN,
} AUDIO_OffsetTypeDef;



 typedef struct
{
   uint8_t cmd;                    /* bRequest */
   uint8_t req_type;               /* bmRequest */
   uint8_t cs;                     /* wValue (high byte): Control Selector */
   uint8_t cn;                     /* wValue (low byte): Control Number */
   uint8_t unit;                   /* wIndex: Feature Unit ID, Extension Unit ID, or Interface, Endpoint */
   uint8_t len;                    /* wLength */
   uint8_t data[USB_MAX_EP0_SIZE]; /* Data */
}
USBD_AUDIO_ControlTypeDef;



typedef struct
{
  uint32_t                  alt_setting;
  uint16_t                  buffer[AUDIO_TOTAL_BUF_SIZE];
  AUDIO_OffsetTypeDef       offset;
  uint8_t                   rd_enable;
  uint16_t                  rd_ptr;
  uint16_t                  wr_ptr;
  uint32_t                  freq;
  uint32_t                  bit_depth;
  int16_t                   volume;
  int32_t                   vol_6dB_shift; // number of 6dB attenuation steps
  uint8_t                   mute;
  USBD_AUDIO_ControlTypeDef control;
} USBD_AUDIO_HandleTypeDef;


typedef struct
{
    int8_t  (*Init)         (uint32_t  AudioFreq, int16_t Volume, uint8_t options);
    int8_t  (*DeInit)       (uint8_t options);
    int8_t  (*AudioCmd)     (uint16_t* pbuf, uint32_t size, uint8_t cmd);
    int8_t  (*VolumeCtl)    (int16_t vol);
    int8_t  (*MuteCtl)      (uint8_t cmd);
    int8_t  (*PeriodicTC)   (uint8_t cmd);
    int8_t  (*GetState)     (void);
} USBD_AUDIO_ItfTypeDef;

#ifdef DEBUG_FEEDBACK_ENDPOINT
extern volatile uint32_t  DbgMinWritableSamples;
extern volatile uint32_t  DbgMaxWritableSamples;
extern volatile uint32_t  DbgSofHistory[];
extern volatile uint32_t  DbgWritableSampleHistory[];
extern volatile float     DbgFeedbackHistory[];
extern volatile uint8_t   DbgIndex;
#endif

extern USBD_ClassTypeDef  USBD_AUDIO;
#define USBD_AUDIO_CLASS    &USBD_AUDIO


uint8_t  USBD_AUDIO_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                        USBD_AUDIO_ItfTypeDef *fops);
void  USBD_AUDIO_Sync (USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset);

#ifdef __cplusplus
}
#endif

#endif  /* __USB_AUDIO_H */
