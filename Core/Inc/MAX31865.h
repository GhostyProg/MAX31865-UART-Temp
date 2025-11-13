/**
  ******************************************************************************
  * @file    MAX31865.h
  * @brief   MAX31865 RTD-to-Digital Converter Library for STM32
  ******************************************************************************
  * @author  Orkun ZA
  * @date    28.01.2025
  * @version 1.2
  ******************************************************************************
  * @attention
  *
  * This library provides functions to interface with the MAX31865 RTD-to-Digital
  * converter using SPI communication on an STM32 microcontroller.
  *
  ******************************************************************************
  */

#ifndef MAX31865_H
#define MAX31865_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <math.h>
#include <stdbool.h>

/* MAX31865  Definitions */
#define MAX31865_2_WIRE             2
#define MAX31865_3_WIRE             3
#define MAX31865_4_WIRE             4
#define MAX31865_SAMPLE_50HZ        50
#define MAX31865_SAMPLE_60HZ        60

/* MAX31865 Register Definitions */
#define MAX31865_CONFIG_REG        0x00
#define MAX31865_BIAS_ON           0x80
#define MAX31865_MODE_AUTO         0x40
#define MAX31865_ONE_SHOT          0x20
#define MAX31865_3WIRE_MODE        0x10
#define MAX31865_FAULT_STATUS      0x02
#define MAX31865_FILTER_50HZ       0x01

#define MAX31865_RTD_MSB_REG       0x01
#define MAX31865_FAULT_REG         0x07

#define RTD_A                      (3.9083e-3F)
#define RTD_B                      (-5.775e-7F)

/* SPI Helper Macros */
#define MAX31865_CS_LOW(max)       HAL_GPIO_WritePin((max)->cs_gpio, (max)->cs_pin, GPIO_PIN_RESET)
#define MAX31865_CS_HIGH(max)      HAL_GPIO_WritePin((max)->cs_gpio, (max)->cs_pin, GPIO_PIN_SET)

/* MAX31865 Struct Definition */
typedef struct {
    GPIO_TypeDef      *cs_gpio; // GPIO port for Chip Select
    uint16_t           cs_pin;  // GPIO pin for Chip Select
    SPI_HandleTypeDef *spi;     // SPI handle
    uint8_t 		   num_wires;
    uint8_t 		   filter_hz;
    float 			   RRef;
    float 			   SensorR;
} MAX31865_t;

/* Function Prototypes */
void  Max31865_Init(MAX31865_t *max);
bool  Max31865_ReadTempC(MAX31865_t *max31865, float *readTemp);
float Max31865_Filter(float new_input, float last_output, float alpha);


#ifdef __cplusplus
}
#endif

#endif /* MAX31865_H */
