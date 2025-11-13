#ifndef UART_H
#define UART_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
// Размеры буферов
#define UART_BUFFER_SIZE           64
#define DATA_PAYLOAD               55
#define NUMBERS_OF_START_BYTE      4

// Состояния
typedef enum {
    STATE_WAIT_SYNC = 1,
    STATE_READY_FOR_COMMAND = 2,
	STATE_TRANSMIT_DATA = 3,
    STATE_UNKNOWN_COMMAND = 5
} CommandTypeState;

// Состояния приема
typedef enum {
    STATE_OK = 0,
    STATE_ERROR = 1,
    STATE_BUSY = 2,
    STATE_BAD_CRC = 3,
    STATE_UNKNOWN = 4,
    STATE_TIMEOUT = 0xFF
} StateProcess;

// Статусы ответа
typedef enum {
    RESP_OK = 0x00,
    RESP_BUSY = 0x01,
    RESP_ERROR = 0x02,
    RESP_TIMEOUT = 0x03,
    RESP_INVALID_CRC = 0x04,
    RESP_INVALID_CMD = 0x05
} ResponseStatus;

//Стартовые байты
typedef enum{
    START_BYTE_ONE = 0x03,
    START_BYTE_TWO = 0x03,
    START_BYTE_TRE = 0x03,
    START_BYTE_FOR = 0x03,
} StartBytes;

// Команды
typedef enum {
    CMD_WAIT_SYNC = 0x3A,
    CMD_GET_DATA = 0x3B,
    CMD_ERROR_RESPONSE = 0xFF
} CommandTo;

// Структура для формата команды UART
typedef struct {
    uint8_t start_byte[NUMBERS_OF_START_BYTE];
    uint8_t command;
    uint8_t status;
    uint8_t payload_len;
    uint8_t payload[DATA_PAYLOAD];
    uint8_t crc_one;
    uint8_t crc_two;
} FormatCommandUART;

#ifdef __cplusplus
extern "C" {
#endif

void InitStartByteUART(FormatCommandUART* cmd);
void TransmitError(uint8_t cmd_in_error, ResponseStatus error_code);
uint16_t CalcCRC(const uint8_t *data, uint8_t length);
bool CheckStartBytes(FormatCommandUART *data);
CommandTypeState SendData(FormatCommandUART *data);
CommandTypeState ReceiveData(FormatCommandUART *data);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
CommandTypeState WaitSync(void);
#ifdef __cplusplus
}

#endif


#endif // UART_H
