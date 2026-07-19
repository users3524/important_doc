# DAC/ADC/PWM/PWMN 驱动实现规划

版本：V0.1
日期：2026-07-06
范围：仅规划 A3161 实际原理图上的 TEC_VSET、TEC_VSEN、TEC_PWM、TEC_PWMN 相关底层驱动，目标是先让 H 桥可控地驱动起来。

## 1. 依据与边界

以 `05_Hardware/A3161.pdf` 为实际原理图依据。`TEC控制与驱动板电路极详细讲解.pdf`、`固件功能规划.pdf`、`Tec_las_dvbd` 只作为思路参考，不能覆盖 A3161 的引脚映射。

本版只实现：

- DAC：输出 `TEC_VSET` 设定电压。
- ADC：采样 `TEC_VSEN` 反馈电压。
- PWM/PWMN：输出 `TEC_PWM` / `TEC_PWMN` 互补波形。
- 必要的安全启动/停止顺序：确保 H 桥默认不误动作。

本版不实现：

- PID 温控闭环。
- SCPI/上位机协议。
- NTC/ADS1120 温度采样。
- EEPROM 校准。
- 外部 DAC8562 的正式输出控制。

## 2. A3161 硬件映射

从 A3161 原理图核对到的 MCU 关键网络：

| 功能 | A3161 网络 | MCU 引脚 | STM32 外设 | 第一版用途 |
|---|---|---|---|---|
| TEC 反馈采样 | `TEC_VSEN` | PA0 | ADC1_IN1 | 读取模拟反馈电压 |
| TEC 半桥反相信号 | `TEC_PWMN` | PA1 | TIM15_CH1N | 互补 PWM 输出 |
| TEC 半桥正相信号 | `TEC_PWM` | PA2 | TIM15_CH1 | 互补 PWM 输出 |
| TEC 设定电压 | `TEC_VSET` | PA4 | DAC1_OUT1 | 输出模拟设定电压 |
| 外部 DAC 片选 | `DAC_SYNC` | PB12 | GPIO | 本版保持高电平未选中 |
| SPI DAC/ADC 总线 | `SPI2_SCK/MISO/MOSI` | PB13/PB14/PB15 | SPI2 | 本版不主动使用 |
| 外部 ADC 片选 | `ADC_CS` | PA11 | GPIO | 本版保持高电平未选中 |
| 外部 ADC 就绪 | `ADC_DRDY` | PC6 | GPIO Input | 本版不主动使用 |

H 桥部分：

- `TEC_PWM` 进入 U22 `NSI6602B-DLAR`，驱动 `TEC-` 半桥。
- `TEC_PWMN` 进入 U23 `NSI6602B-DLAR`，驱动 `TEC+` 半桥。
- 两路必须为互补关系，推荐使用 TIM15 的 CH1/CH1N，不能用两个普通定时器分别输出。
- U22/U23 的 DT 网络已提供半桥内部死区，TIM15 死区第一版可先置 0；若示波器确认需要，再加小死区。

模拟环路部分：

- `TEC_VSET` 与 `TEC_VSEN` 进入 U19 运放误差/补偿网络。
- 固件第一版只负责给出设定电压并读取反馈，不在 MCU 内实现电流环。
- 零电流对应的 `TEC_VSET` 电压需要实测确认，不能直接假定 0 V 或 1.65 V。

## 3. 当前工程需要修正的差异

`07_Firmware/TECControlV1.0` 当前 CubeMX/HAL 配置与 A3161 不一致：

| 当前配置 | 问题 | 规划修正 |
|---|---|---|
| PA0 标为 `Laser_VSEN` / ADC1_IN1 | A3161 上 PA0 是 `TEC_VSEN` | 改名为 `TEC_VSEN`，保留 ADC1_IN1 |
| PA4 标为 `Laser_VSET` / DAC1_OUT1 | A3161 上 PA4 是 `TEC_VSET` | 改名为 `TEC_VSET`，使用 DAC1_CH1 |
| PA5 标为 `TEC_VSET` / DAC1_OUT2 | A3161 上 PA5 是 `U14-13` | 本版不要把 PA5 当 TEC DAC |
| PA7 标为 `TEC_VSEN` / ADC2_IN4 | A3161 上 PA7 是 `U14-15` | 本版不要把 PA7 当 TEC ADC |
| `.ioc` 写了 PA1/PA2 为 TIM15 | 生成代码没有 `htim15`，只有 TIM3/TIM17 | 重新生成或手工补齐 TIM15 |
| 参考工程 `Tec_las_dvbd` 的 TIM15 频率定义不一致 | 不能直接照搬 | 以本规划明确参数为准 |

## 4. 外设配置规划

### 4.1 DAC1

目标：PA4 输出 `TEC_VSET`。

配置：

- DAC1 Channel 1：`DAC_CHANNEL_1`
- 输出模式：外部引脚输出，buffer enable。
- 触发：software/no trigger。
- 分辨率：12 bit right aligned。

接口：

```c
void TEC_DAC_InitSafe(void);
void TEC_DAC_SetRaw(uint16_t raw);
void TEC_DAC_SetMillivolt(uint16_t mv);
uint16_t TEC_DAC_GetRaw(void);
```

第一版策略：

- 初始化后先写 `TEC_VSET_SAFE_RAW`，默认 0。
- H 桥未验证前，禁止自动写大幅设定值。
- 后续通过实测把 `TEC_VSET_ZERO_MV` 固化为校准参数。

### 4.2 ADC1

目标：PA0 采样 `TEC_VSEN`。

配置：

- ADC1 regular channel：`ADC_CHANNEL_1`
- 分辨率：12 bit。
- 触发：software start。
- 采样时间：先用较长采样时间，例如 47.5 cycles 或更高，避免 2.5 cycles 带来噪声/源阻抗风险。
- 启动时执行 ADC calibration。

接口：

```c
uint16_t TEC_ADC_ReadRaw(void);
uint16_t TEC_ADC_ReadMillivolt(void);
uint16_t TEC_ADC_ReadMillivoltAvg(uint8_t samples);
```

第一版策略：

- 先用 polling 方式，避免 DMA 增加调试变量。
- 做 8 或 16 点平均，用于串口/调试打印和安全判断。
- 暂不把 ADC 值闭环到 PWM，只用于观测和保护。

### 4.3 TIM15 PWM/PWMN

目标：PA2/PA1 输出互补 PWM，驱动 U22/U23。

配置：

- TIM15 internal clock。
- CH1：PA2 `TEC_PWM`
- CH1N：PA1 `TEC_PWMN`
- PWM mode 1。
- 极性：先按 `TEC_PWM` 高、`TEC_PWMN` 低的互补关系配置；上板后用示波器确认。
- 自动输出：enable。
- Break：第一版 disable。
- DeadTime：第一版 0，依赖 NSI6602 外部 DT 网络；如波形验证需要再增加。

频率建议：

- 联调第一版先用 2 kHz，匹配参考工程设计文档，便于示波器观察。
- 系统时钟 170 MHz、APB2 timer clock 170 MHz 时，可用 `PSC=1`、`ARR=42499` 得到约 2 kHz。
- 后续若 TEC 纹波或噪声要求更高，再评估 10 kHz 或 20 kHz。

占空比定义：

- H 桥互补驱动采用中心点控制。
- 50% 作为零平均差分输出中心点。
- `cmd = 0`：CCR = ARR/2。
- `cmd > 0`：`TEC_PWM` 高电平时间增加。
- `cmd < 0`：`TEC_PWMN` 等效方向增加。
- 第一版限幅不超过中心点的 +/-10%，确认功率级安全后再放宽。

接口：

```c
void TEC_PWM_InitSafe(void);
void TEC_PWM_Start(void);
void TEC_PWM_Stop(void);
void TEC_PWM_SetCenter(void);
void TEC_PWM_SetCommand(int16_t cmd_permille);
uint16_t TEC_PWM_GetCompare(void);
```

`cmd_permille` 建议范围：

- 第一版：`-100` 到 `+100`，即中心点 +/-10%。
- 后续验证后再扩展到 `-900` 到 `+900`。

### 4.4 外部 DAC8562/ADS1120

A3161 上存在 DAC8562 与 ADS1120，但本版不纳入 H 桥最小驱动闭环。

本版只保证：

- `DAC_SYNC` 默认高电平，避免外部 DAC 被误写。
- `ADC_CS` 默认高电平，避免外部 ADC 占用 SPI。
- SPI2 可以保留初始化，但不作为 TEC_VSET/TEC_VSEN 的主路径。

## 5. 软件模块规划

新增最小模块：

```text
Core/Inc/tec_hw.h
Core/Src/tec_hw.c
```

职责：

- 封装 TEC DAC/ADC/PWM 的硬件细节。
- 统一 H 桥安全状态机。
- 不包含 PID、温控目标、协议解析。

建议状态：

```c
typedef enum {
  TEC_HW_STATE_OFF = 0,
  TEC_HW_STATE_READY,
  TEC_HW_STATE_PWM_ON,
  TEC_HW_STATE_FAULT
} TEC_HW_State;
```

建议核心接口：

```c
void TEC_HW_Init(void);
void TEC_HW_EnableOutput(void);
void TEC_HW_DisableOutput(void);
void TEC_HW_SetVsetMv(uint16_t mv);
uint16_t TEC_HW_ReadVsenMv(void);
void TEC_HW_SetPwmCommand(int16_t cmd_permille);
TEC_HW_State TEC_HW_GetState(void);
```

初始化顺序：

1. HAL/CubeMX 初始化 GPIO、ADC1、DAC1、TIM15。
2. `TEC_HW_Init()`：
   - 停止 TIM15 CH1/CH1N。
   - DAC 写安全值。
   - ADC calibration。
   - PWM compare 置中心点。
   - 外部 `DAC_SYNC`、`ADC_CS` 拉高。
3. 人工调试命令或临时代码设置小幅 `TEC_VSET`。
4. 启动 TIM15 互补 PWM。
5. 逐步增加 `cmd_permille` 或 DAC 设定，观察 `TEC_VSEN` 与 H 桥输出。

关闭顺序：

1. `TEC_PWM_SetCenter()`。
2. `TEC_PWM_Stop()` 停止 CH1/CH1N。
3. `TEC_DAC_SetMillivolt(TEC_VSET_SAFE_MV)`。
4. 必要时关闭功率 EN GPIO。

## 6. H 桥联调步骤

### 阶段 1：不上功率负载

- 烧录后确认 PA4 `TEC_VSET` 默认安全电压。
- 用可调命令输出 0.5 V、1.0 V、1.5 V，万用表测 PA4 单调变化。
- 给 PA0 输入已知电压，确认 ADC 读数误差在可接受范围。
- 示波器测 PA2/PA1：
  - 频率约 2 kHz。
  - 两路互补。
  - 50% 中心点正确。
  - Stop 后两路无持续误触发。

### 阶段 2：接 H 桥功率但不接 TEC

- 限流电源上电。
- 保持 DAC 安全值、PWM 中心点。
- 启动 PWM 后观察 `TEC+`、`TEC-` 差分波形。
- 检查 U22/U23 输入与半桥输出逻辑是否和预期一致。
- 若出现电源电流异常，立即停止 PWM 并回到 OFF。

### 阶段 3：接假负载

- 用功率电阻或电子负载替代 TEC。
- 从 `cmd_permille = 0` 开始，每次只增加 1% 到 2%。
- 记录 `TEC_VSEN`、电源电流、MOS 温升。
- 确认正负命令对应的电流方向。

### 阶段 4：接 TEC

- 先小功率运行。
- 固件仍只做开环设定和采样，不做 PID。
- 确认 `TEC_VSEN` 随 `TEC_VSET` 和 PWM 命令变化。
- 通过实测确定 `TEC_VSET_ZERO_MV`、最大安全设定、电流方向。

## 7. 安全约束

第一版代码必须满足：

- 上电默认不启动 TIM15 PWM。
- DAC 默认写安全值。
- 未显式 enable 前，任何 set 命令只更新缓存，不推动 H 桥。
- PWM 命令必须限幅。
- Stop/Disable 必须可重复调用。
- ADC 读数超出配置阈值时进入 FAULT，并停止 PWM。
- `DAC_SYNC`、`ADC_CS` 默认保持高电平，避免 SPI 外设误动作。

需要硬件实测后确认：

- `TEC_VSET` 零电流电压。
- `TEC_VSEN` 到实际电流/电压的比例关系。
- TIM15 输出极性是否需要反转。
- 是否需要启用 TIM15 deadtime。
- `U14-14`、`U14-45` 等功率 EN 脚的默认电平和启停顺序。

## 8. 实施顺序

1. 修正 CubeMX 引脚：
   - PA0：`TEC_VSEN` / ADC1_IN1。
   - PA4：`TEC_VSET` / DAC1_OUT1。
   - PA1：`TEC_PWMN` / TIM15_CH1N。
   - PA2：`TEC_PWM` / TIM15_CH1。
   - 移除 PA5/PA7 作为 TEC DAC/ADC 的旧配置。
2. 生成并检查 `adc.c`、`dac.c`、`tim.c`：
   - 必须有 `hadc1`。
   - 必须有 `hdac1` 且配置 `DAC_CHANNEL_1`。
   - 必须有 `htim15`、`MX_TIM15_Init()`、`HAL_TIMEx_PWMN_Start()` 可用。
3. 新增 `tec_hw.c/h`。
4. 在 `main.c` 的 USER CODE 区调用 `TEC_HW_Init()`。
5. 加一个临时联调入口：
   - 可以先写固定小测试序列。
   - 后续再接串口命令。
6. 编译。
7. 按第 6 节四阶段上板验证。

## 9. 第一版验收标准

- 编译通过。
- PA4 DAC 可控，电压单调且可回到安全值。
- PA0 ADC 可读取实际电压，平均值稳定。
- PA2/PA1 输出互补 PWM，频率和占空比符合配置。
- Stop/Disable 后 H 桥输入不再持续驱动。
- 限流电源下，H 桥空载和假负载测试无异常电流。
- 文档记录实测的零点、极性和安全限幅，为下一版 PID/闭环控制提供参数。

## 10. V0.1 代码实施记录

已按 A3161 实际原理图完成第一版底层代码：

- `TEC_VSEN` 使用 PA0 / ADC1_IN1，ADC1 采用软件触发、单次转换、47.5 cycles 采样时间，并在 `TEC_HW_Init()` 中做 ADC calibration。
- `TEC_VSET` 使用 PA4 / DAC1_OUT1，默认写入 `TEC_VSET_SAFE_MV`，当前安全值为 0 mV。
- `TEC_PWM` 使用 PA2 / TIM15_CH1，`TEC_PWMN` 使用 PA1 / TIM15_CH1N，由 TIM15 硬件互补输出，频率约 2 kHz。
- 新增 `Core/Inc/tec_hw.h` 和 `Core/Src/tec_hw.c`，封装 DAC、ADC、PWM/PWMN 以及 `OFF/READY/PWM_ON/FAULT` 状态机。
- 第一版 PWM 命令限制为 `-100` 到 `+100` permille，即围绕 50% 中心点的 +/-10%。
- TIM15 停止/关闭时配置 `OSSR/OSSI`，配合 idle reset 尽量将 H 桥驱动输入保持为低电平。
- 上电默认只初始化并保持 H 桥关闭；如需临时上电起振，将 `TEC_BRINGUP_ENABLE_ON_BOOT` 改为 1 后重新编译，启动时会以中心点 PWM 输出。
- PA5/PA7 已从旧 TEC DAC/ADC 角色中移除，按 A3161 作为 `U14_13` / `U14_15` 普通输入保留。
- `.ioc` 已同步移除 ADC2 和 DAC1_OUT2 残留，避免重新生成时覆盖回旧映射。

当前仍需上板实测确认：

- `TEC_PWM` / `TEC_PWMN` 在 U22/U23 输入处的实际极性。
- 0 mV 是否确为 `TEC_VSET` 安全值，以及零电流对应电压。
- 是否需要在 TIM15 中增加 MCU 侧 deadtime。
- `TEC_VSEN` 与实际 TEC 电流/桥臂电压的换算关系。
