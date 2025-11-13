/**
  ******************************************************************************
  * @file    MAX31865.c
  * @brief   MAX31865 RTD-to-Digital Converter Driver Implementation
  ******************************************************************************
  * @author  Orkun ZA
  * @date    28.01.2025
  * @version 1.2
  ******************************************************************************
  * @attention
  *
  * This file provides the implementation of the MAX31865 driver functions for
  * STM32 using SPI communication. It supports basic initialization and RTD
  * temperature readings for PT100 sensors.
  *
  ******************************************************************************
  */

#include "max31865.h"

/* SPI Read Register */
static uint32_t Max31865_readRegister(MAX31865_t *max, uint8_t addr, uint8_t size) {
    uint8_t buffer[2] = {0};
    addr &= 0x7F; // Read Mode
    MAX31865_CS_LOW(max);
    HAL_SPI_Transmit(max->spi, &addr, 1, 100);
    HAL_SPI_Receive(max->spi, buffer, size, 100);
    MAX31865_CS_HIGH(max);
    HAL_Delay(50);
    return (size == 1) ? buffer[0] : ((buffer[0] << 8) | buffer[1]);
}

/* SPI Write Register */
static void Max31865_writeRegister(MAX31865_t *max, uint8_t addr, uint8_t data) {
    addr |= 0x80;  // Write Mode
    uint8_t txData[2] = {addr, data};
    MAX31865_CS_LOW(max);
    HAL_SPI_Transmit(max->spi, txData, 2, 100);
    MAX31865_CS_HIGH(max);
}

/* Fault Management */
uint8_t Max31865_readFault(MAX31865_t *max) {
    return (uint8_t)Max31865_readRegister(max, MAX31865_FAULT_REG, 1);
}

void Max31865_clearFault(MAX31865_t *max) {
    uint8_t cfg = (uint8_t)Max31865_readRegister(max, MAX31865_CONFIG_REG, 1);
    cfg &= ~0x2C;
    cfg |= MAX31865_FAULT_STATUS;
    Max31865_writeRegister(max, MAX31865_CONFIG_REG, cfg);
}

/* Configuration Functions */
void Max31865_enableBias(MAX31865_t *max, bool enable) {
    uint8_t cfg = (uint8_t)Max31865_readRegister(max, MAX31865_CONFIG_REG, 1);
    cfg = enable ? (cfg | MAX31865_BIAS_ON) : (cfg & ~MAX31865_BIAS_ON);
    Max31865_writeRegister(max, MAX31865_CONFIG_REG, cfg);
}

void Max31865_autoConvert(MAX31865_t *max, bool enable) {
    uint8_t cfg = (uint8_t)Max31865_readRegister(max, MAX31865_CONFIG_REG, 1);
    cfg = enable ? (cfg | MAX31865_MODE_AUTO) : (cfg & ~MAX31865_MODE_AUTO);
    Max31865_writeRegister(max, MAX31865_CONFIG_REG, cfg);
}

void Max31865_setWires(MAX31865_t *max, uint8_t wires) {
    uint8_t cfg = (uint8_t)Max31865_readRegister(max, MAX31865_CONFIG_REG, 1);
    cfg = (wires == 3) ? (cfg | MAX31865_3WIRE_MODE) : (cfg & ~MAX31865_3WIRE_MODE);
    Max31865_writeRegister(max, MAX31865_CONFIG_REG, cfg);
}

/* Single Shot Read */
void Max31865_singleShot(MAX31865_t *max) {
    uint8_t cfg = (uint8_t)Max31865_readRegister(max, MAX31865_CONFIG_REG, 1);
    Max31865_writeRegister(max, MAX31865_CONFIG_REG, cfg | MAX31865_ONE_SHOT);
}

/* Read RTD Value */
uint16_t Max31865_readRTD(MAX31865_t *max) {
    Max31865_clearFault(max);
    Max31865_enableBias(max, true);
    HAL_Delay(100);

    Max31865_singleShot(max);
    HAL_Delay(100);

    uint16_t rtd = (uint16_t)Max31865_readRegister(max, MAX31865_RTD_MSB_REG, 2) >> 1;
    Max31865_enableBias(max, false);
    return rtd;
}

/* Initialization */
void Max31865_Init(MAX31865_t *max) {
    if (!max) return;

    MAX31865_CS_HIGH(max);
    HAL_Delay(10);
    Max31865_setWires(max, max->num_wires);
    Max31865_clearFault(max);

    uint8_t cfg = (uint8_t)Max31865_readRegister(max, MAX31865_CONFIG_REG, 1);
    if (max->filter_hz == 50)
        cfg |= MAX31865_FILTER_50HZ;
    else
        cfg &= ~MAX31865_FILTER_50HZ;
    Max31865_writeRegister(max, MAX31865_CONFIG_REG, cfg);
}

/* Temperature Calculation */
bool Max31865_ReadTempC(MAX31865_t *max, float *temperature) {
    float Rt = Max31865_readRTD(max);
    Rt = (Rt / 32768.0f) * max->RRef;

    float Z1 = -RTD_A;
    float Z2 = RTD_A * RTD_A - (4 * RTD_B);
    float Z3 = (4 * RTD_B) / max->SensorR;
    float Z4 = 2 * RTD_B;

    float temp = sqrtf(Z2 + (Z3 * Rt)) + Z1;
    temp /= Z4;

    if (temp < 0) {
        Rt /= max->SensorR;
        Rt *= 100;
        temp = -242.02 + 2.2228 * Rt + 2.5859e-3 * Rt * Rt - 4.8260e-6 * Rt * Rt * Rt;
    }

    *temperature = temp;
    return Max31865_readFault(max) == 0;
}

/* Low-pass Filter */
float Max31865_Filter(float new_input, float last_output, float alpha) {
    return (alpha >= 0.0F && alpha <= 1.0F) ? (last_output * (1.0F - alpha) + new_input * alpha) : last_output;
}
