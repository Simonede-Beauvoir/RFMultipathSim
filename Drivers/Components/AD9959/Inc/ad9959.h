#ifndef AD9959_H
#define AD9959_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AD9959_AMPLITUDE_MAX    1023U
#define AD9959_PHASE_WORD_MAX   16383U
#define AD9959_PHASE_STEPS      16384U
#define AD9959_MAX_SYSCLK_HZ    500000000ULL

typedef enum {
    SINGLE_TONE,
    MODULATION,
    LINEAR_SWEEP,
    OUTPUT_AMPLITUDE_CONTROL
} AD9959_Mode;

typedef enum {
    Channel_None = 0x00,
    Channel_0 = 0x10,
    Channel_1 = 0x20,
    Channel_2 = 0x40,
    Channel_3 = 0x80,
    Channel_All = 0xF0
} AD9959_Channel;

typedef enum {
    CSR = 0x00,
    FR1 = 0x01,
    FR2 = 0x02,
    CFR = 0x03,
    CFTW = 0x04,
    CPOW = 0x05,
    ACR = 0x06,
    LSRR = 0x07,
    RDW = 0x08,
    FDW = 0x09,
    CW1 = 0x0A,
    CW2 = 0x0B,
    CW3 = 0x0C,
    CW4 = 0x0D,
    CW5 = 0x0E,
    CW6 = 0x0F,
    CW7 = 0x10,
    CW8 = 0x11,
    CW9 = 0x12,
    CW10 = 0x13,
    CW11 = 0x14,
    CW12 = 0x15,
    CW13 = 0x16,
    CW14 = 0x17,
    CW15 = 0x18
} AD9959_Register;

typedef enum {
    MSB_First = 0x00,
    LSB_First = 0x01,
    IO2Wire = 0x00,
    IO3Wire = 0x02,
    IO2Bit = 0x04,
    IO4Bit = 0x06
} AD9959_CSR_Bits;

typedef enum {
    ChargePump0 = 0x00,
    ChargePump1 = 0x01,
    ChargePump2 = 0x02,
    ChargePump3 = 0x03,
    PllDivider = 0x04,
    VCOGain = 0x80,
    ModLevels2 = 0x00,
    ModLevels4 = 0x01,
    ModLevels8 = 0x02,
    ModLevels16 = 0x03,
    RampUpDownOff = 0x00,
    RampUpDownP2P3 = 0x04,
    RampUpDownP3 = 0x08,
    RampUpDownSDIO123 = 0x0C,
    Profile0 = 0x00,
    Profile7 = 0x07,
    SyncAuto = 0x00,
    SyncSoft = 0x01,
    SyncHard = 0x02,
    DACRefPwrDown = 0x10,
    SyncClkDisable = 0x20,
    ExtFullPwrDown = 0x40,
    RefClkInPwrDown = 0x80
} AD9959_FR1_Bits;

typedef enum {
    AllChanAutoClearSweep = 0x8000,
    AllChanClearSweep = 0x4000,
    AllChanAutoClearPhase = 0x2000,
    AllChanClearPhase = 0x1000,
    AutoSyncEnable = 0x0080,
    MasterSyncEnable = 0x0040,
    MasterSyncStatus = 0x0020,
    MasterSyncMask = 0x0010,
    SystemClockOffset = 0x0003
} AD9959_FR2_Bits;

typedef enum {
    ModulationMode = 0xC00000,
    AmplitudeModulation = 0x400000,
    FrequencyModulation = 0x800000,
    PhaseModulation = 0xC00000,
    SweepNoDwell = 0x008000,
    SweepEnable = 0x004000,
    SweepStepTimerExt = 0x002000,
    DACFullScale = 0x000300,
    DigitalPowerDown = 0x000080,
    DACPowerDown = 0x000040,
    MatchPipeDelay = 0x000020,
    AutoclearSweep = 0x000010,
    ClearSweep = 0x000008,
    AutoclearPhase = 0x000004,
    ClearPhase = 0x000002,
    OutputSineWave = 0x000001
} AD9959_CFR_Bits;

typedef enum {
    RampRate = 0xFF0000,
    StepSize = 0x00C000,
    MultiplierEnable = 0x001000,
    RampEnable = 0x000800,
    LoadARRAtIOUpdate = 0x000400,
    ScaleFactor = 0x0003FF
} AD9959_ACR_Bits;

typedef struct {
    SPI_HandleTypeDef* SPI_handler;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
    GPIO_TypeDef* mrst_port;
    uint16_t mrst_pin;
    GPIO_TypeDef* update_port;
    uint16_t update_pin;
    uint32_t crystal_clock;
    int32_t calibration_factor;
    uint8_t multiplier;
    uint64_t core_clock;
    AD9959_CSR_Bits IO_mode;
    AD9959_Channel last_channels;
    HAL_StatusTypeDef last_status;
} AD9959_Handler;

void AD9959_Init(AD9959_Handler* device, SPI_HandleTypeDef* hspi, GPIO_TypeDef* device_cs_port, uint16_t device_cs_pin, GPIO_TypeDef* device_mrst_port,
                 uint16_t device_mrst_pin, GPIO_TypeDef* device_update_port, uint16_t device_update_pin, uint32_t device_crystal_clock,
                 int32_t device_calibration);
void AD9959_Reset(AD9959_Handler* device, AD9959_CFR_Bits device_cfr);
void AD9959_SetClock(AD9959_Handler* device, uint8_t mult, int32_t calibration);
void AD9959_SetChannels(AD9959_Handler* device, AD9959_Channel chan);
uint32_t AD9959_GenerateFrequencyDelta(const AD9959_Handler* device, uint32_t freq);
void AD9959_SetChannelFrequency(AD9959_Handler* device, AD9959_Channel chan, uint32_t freq);
void AD9959_SetChannelDelta(AD9959_Handler* device, AD9959_Channel chan, uint32_t delta);
void AD9959_SetPhase(AD9959_Handler* device, AD9959_Channel chan, int32_t phase_degrees);
void AD9959_SetAmplitude(AD9959_Handler* device, AD9959_Channel chan, uint16_t amplitude);
void AD9959_Update(AD9959_Handler* device);
HAL_StatusTypeDef AD9959_GetLastStatus(const AD9959_Handler* device);
uint32_t AD9959_GetCoreClockHz(const AD9959_Handler* device);
void AD9959_SweepFrequency(AD9959_Handler* device, AD9959_Channel chan, uint32_t freq, uint8_t follow);
void AD9959_SweepDelta(AD9959_Handler* device, AD9959_Channel chan, uint32_t delta, uint8_t follow);
void AD9959_SweepRates(AD9959_Handler* device, AD9959_Channel chan, uint32_t increment, uint8_t up_rate, uint32_t decrement, uint8_t down_rate);
void AD9959_SweepAmplitude(AD9959_Handler* device, AD9959_Channel chan, uint16_t amplitude, uint8_t follow);
void AD9959_SweepPhase(AD9959_Handler* device, AD9959_Channel chan, uint16_t phase, uint8_t follow);

#ifdef __cplusplus
}
#endif

#endif
