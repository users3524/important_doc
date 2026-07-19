#include "tec_hw.h"

#include "adc.h"
#include "dac.h"
#include "gpio.h"
#include "tim.h"

#define TEC_ADC_TIMEOUT_MS       10U
#define TEC_ADC_DEFAULT_SAMPLES  8U

static TEC_HW_State tec_hw_state = TEC_HW_STATE_OFF;
static uint16_t tec_dac_raw = 0U;
static uint16_t tec_pwm_compare = 0U;
static uint8_t tec_hw_fault_handling = 0U;

static uint16_t TEC_ClampU16(uint32_t value, uint16_t max_value)
{
  return (value > max_value) ? max_value : (uint16_t)value;
}

static void TEC_HW_SetFault(void)
{
  if (tec_hw_fault_handling != 0U)
  {
    tec_hw_state = TEC_HW_STATE_FAULT;
    return;
  }

  tec_hw_fault_handling = 1U;
  TEC_PWM_Stop();
  TEC_DAC_SetMillivolt(TEC_VSET_SAFE_MV);
  tec_hw_state = TEC_HW_STATE_FAULT;
  tec_hw_fault_handling = 0U;
}

void TEC_HW_Init(void)
{
  tec_hw_state = TEC_HW_STATE_OFF;

  HAL_GPIO_WritePin(DAC_SYNC_GPIO_Port, DAC_SYNC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(U14_14_GPIO_Port, U14_14_Pin, GPIO_PIN_RESET);

  TEC_PWM_InitSafe();
  TEC_DAC_InitSafe();
  if (tec_hw_state == TEC_HW_STATE_FAULT)
  {
    return;
  }

  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    TEC_HW_SetFault();
    return;
  }

  tec_hw_state = TEC_HW_STATE_READY;
}

void TEC_HW_EnableOutput(void)
{
  if ((tec_hw_state == TEC_HW_STATE_FAULT) || (tec_hw_state == TEC_HW_STATE_PWM_ON))
  {
    return;
  }

  TEC_PWM_SetCenter();
  TEC_PWM_Start();
  if (tec_hw_state != TEC_HW_STATE_FAULT)
  {
    tec_hw_state = TEC_HW_STATE_PWM_ON;
  }
}

void TEC_HW_DisableOutput(void)
{
  TEC_PWM_SetCenter();
  TEC_PWM_Stop();
  TEC_DAC_SetMillivolt(TEC_VSET_SAFE_MV);
  if (tec_hw_state != TEC_HW_STATE_FAULT)
  {
    tec_hw_state = TEC_HW_STATE_OFF;
  }
}

void TEC_HW_SetVsetMv(uint16_t mv)
{
  if (tec_hw_state == TEC_HW_STATE_FAULT)
  {
    return;
  }

  TEC_DAC_SetMillivolt(mv);
}

uint16_t TEC_HW_ReadVsenMv(void)
{
  return TEC_ADC_ReadMillivoltAvg(TEC_ADC_DEFAULT_SAMPLES);
}

void TEC_HW_SetPwmCommand(int16_t cmd_permille)
{
  if (tec_hw_state == TEC_HW_STATE_FAULT)
  {
    return;
  }

  TEC_PWM_SetCommand(cmd_permille);
}

TEC_HW_State TEC_HW_GetState(void)
{
  return tec_hw_state;
}

void TEC_DAC_InitSafe(void)
{
  if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK)
  {
    TEC_HW_SetFault();
    return;
  }

  TEC_DAC_SetMillivolt(TEC_VSET_SAFE_MV);
}

void TEC_DAC_SetRaw(uint16_t raw)
{
  tec_dac_raw = (raw > TEC_DAC_MAX_RAW) ? TEC_DAC_MAX_RAW : raw;

  if (HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, tec_dac_raw) != HAL_OK)
  {
    TEC_HW_SetFault();
  }
}

void TEC_DAC_SetMillivolt(uint16_t mv)
{
  uint16_t limited_mv = (mv > TEC_DAC_VREF_MV) ? TEC_DAC_VREF_MV : mv;
  uint32_t raw = ((uint32_t)limited_mv * TEC_DAC_MAX_RAW + (TEC_DAC_VREF_MV / 2U)) / TEC_DAC_VREF_MV;

  TEC_DAC_SetRaw((uint16_t)raw);
}

uint16_t TEC_DAC_GetRaw(void)
{
  return tec_dac_raw;
}

uint16_t TEC_ADC_ReadRaw(void)
{
  uint16_t raw = 0U;

  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    TEC_HW_SetFault();
    return 0U;
  }

  if (HAL_ADC_PollForConversion(&hadc1, TEC_ADC_TIMEOUT_MS) != HAL_OK)
  {
    (void)HAL_ADC_Stop(&hadc1);
    TEC_HW_SetFault();
    return 0U;
  }

  raw = TEC_ClampU16(HAL_ADC_GetValue(&hadc1), TEC_ADC_MAX_RAW);

  if (HAL_ADC_Stop(&hadc1) != HAL_OK)
  {
    TEC_HW_SetFault();
  }

  return raw;
}

uint16_t TEC_ADC_ReadMillivolt(void)
{
  uint32_t raw = TEC_ADC_ReadRaw();
  return (uint16_t)((raw * TEC_ADC_VREF_MV + (TEC_ADC_MAX_RAW / 2U)) / TEC_ADC_MAX_RAW);
}

uint16_t TEC_ADC_ReadMillivoltAvg(uint8_t samples)
{
  uint32_t sum = 0U;
  uint8_t count = (samples == 0U) ? 1U : samples;

  for (uint8_t i = 0U; i < count; i++)
  {
    sum += TEC_ADC_ReadMillivolt();
    if (tec_hw_state == TEC_HW_STATE_FAULT)
    {
      return 0U;
    }
  }

  return (uint16_t)((sum + (count / 2U)) / count);
}

void TEC_PWM_InitSafe(void)
{
  TEC_PWM_SetCenter();
  TEC_PWM_Stop();
}

void TEC_PWM_Start(void)
{
  if (HAL_TIM_PWM_Start(&htim15, TIM_CHANNEL_1) != HAL_OK)
  {
    TEC_HW_SetFault();
    return;
  }

  if (HAL_TIMEx_PWMN_Start(&htim15, TIM_CHANNEL_1) != HAL_OK)
  {
    (void)HAL_TIM_PWM_Stop(&htim15, TIM_CHANNEL_1);
    TEC_HW_SetFault();
  }
}

void TEC_PWM_Stop(void)
{
  (void)HAL_TIMEx_PWMN_Stop(&htim15, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Stop(&htim15, TIM_CHANNEL_1);
}

void TEC_PWM_SetCenter(void)
{
  uint32_t period_counts = __HAL_TIM_GET_AUTORELOAD(&htim15) + 1U;
  tec_pwm_compare = (uint16_t)(period_counts / 2U);
  __HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_1, tec_pwm_compare);
}

void TEC_PWM_SetCommand(int16_t cmd_permille)
{
  uint32_t period_counts = __HAL_TIM_GET_AUTORELOAD(&htim15) + 1U;
  int32_t center = (int32_t)(period_counts / 2U);
  int32_t limited_cmd = cmd_permille;
  int32_t delta;
  int32_t compare;

  if (limited_cmd > TEC_PWM_CMD_LIMIT_PERMILLE)
  {
    limited_cmd = TEC_PWM_CMD_LIMIT_PERMILLE;
  }
  else if (limited_cmd < -TEC_PWM_CMD_LIMIT_PERMILLE)
  {
    limited_cmd = -TEC_PWM_CMD_LIMIT_PERMILLE;
  }

  delta = (center * limited_cmd) / 1000;
  compare = center + delta;

  if (compare < 1)
  {
    compare = 1;
  }
  else if (compare > ((int32_t)period_counts - 1))
  {
    compare = (int32_t)period_counts - 1;
  }

  tec_pwm_compare = (uint16_t)compare;
  __HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_1, tec_pwm_compare);
}

uint16_t TEC_PWM_GetCompare(void)
{
  return tec_pwm_compare;
}
