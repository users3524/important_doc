#ifndef __TEC_HW_H__
#define __TEC_HW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define TEC_DAC_VREF_MV              3300U
#define TEC_DAC_MAX_RAW              4095U
#define TEC_ADC_VREF_MV              3300U
#define TEC_ADC_MAX_RAW              4095U

#define TEC_VSET_SAFE_MV             0U
#define TEC_PWM_CMD_LIMIT_PERMILLE   100
#define TEC_BRINGUP_ENABLE_ON_BOOT   0U
#define TEC_BRINGUP_VSET_MV          TEC_VSET_SAFE_MV
#define TEC_BRINGUP_PWM_CMD_PERMILLE 0

typedef enum {
  TEC_HW_STATE_OFF = 0,
  TEC_HW_STATE_READY,
  TEC_HW_STATE_PWM_ON,
  TEC_HW_STATE_FAULT
} TEC_HW_State;

void TEC_HW_Init(void);
void TEC_HW_EnableOutput(void);
void TEC_HW_DisableOutput(void);
void TEC_HW_SetVsetMv(uint16_t mv);
uint16_t TEC_HW_ReadVsenMv(void);
void TEC_HW_SetPwmCommand(int16_t cmd_permille);
TEC_HW_State TEC_HW_GetState(void);

void TEC_DAC_InitSafe(void);
void TEC_DAC_SetRaw(uint16_t raw);
void TEC_DAC_SetMillivolt(uint16_t mv);
uint16_t TEC_DAC_GetRaw(void);

uint16_t TEC_ADC_ReadRaw(void);
uint16_t TEC_ADC_ReadMillivolt(void);
uint16_t TEC_ADC_ReadMillivoltAvg(uint8_t samples);

void TEC_PWM_InitSafe(void);
void TEC_PWM_Start(void);
void TEC_PWM_Stop(void);
void TEC_PWM_SetCenter(void);
void TEC_PWM_SetCommand(int16_t cmd_permille);
uint16_t TEC_PWM_GetCompare(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEC_HW_H__ */
