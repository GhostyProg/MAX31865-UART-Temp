#include "UART.h"

extern UART_HandleTypeDef huart1;		  // Заменить на свой номер huart
extern DMA_HandleTypeDef hdma_usart1_rx; // Заменить на свой номер huart
extern DMA_HandleTypeDef hdma_usart1_tx; // Заменить на свой номер huart
UART_HandleTypeDef *active_uart = &huart1;  // Заменить на свой номер huart

/*
 * Что бы увидеть эти переменные у себя в проекте требуется написать
	extern FormatCommandUART TXData;
	extern FormatCommandUART RXData;
*/

CommandTypeState State = STATE_WAIT_SYNC;
FormatCommandUART TXData;
FormatCommandUART RXData;

// Определение байтов находятся в заголовке (UART.h) (меняем стартовые байты на свои)


void InitStartByteUART(FormatCommandUART* cmd) {
	cmd->start_byte[0] = START_BYTE_ONE;
	cmd->start_byte[1] = START_BYTE_TWO;
	cmd->start_byte[2] = START_BYTE_TRE;
	cmd->start_byte[3] = START_BYTE_FOR;
}

void TransmitError(uint8_t cmd_in_error, ResponseStatus error_code) {
	TXData.command = CMD_ERROR_RESPONSE;
	TXData.status = error_code;
	TXData.payload_len = 5;
	TXData.payload[0] = cmd_in_error;
	SendData(&TXData);
}

uint16_t CalcCRC(const uint8_t *data, uint8_t length) {
	uint16_t crc = 0xFFFF;
	for (uint8_t i = 0; i < length; i++) {
		crc ^= (uint16_t)data[i] << 8;
		for (uint8_t bit = 0; bit < 8; bit++) {
			if (crc & 0x8000) {
				crc = (crc << 1) ^ 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

bool CheckStartBytes(FormatCommandUART *data) {
	CommandTo start_bytes[] = {
		START_BYTE_ONE,
		START_BYTE_TWO,
		START_BYTE_TRE,
		START_BYTE_FOR,
	};

	for (int i = 0; i < NUMBERS_OF_START_BYTE; i++) {
		if (data->start_byte[i] != start_bytes[i]) {
			return false;
		}
	}
	return true;
}

uint8_t CheckPayloadlen(FormatCommandUART *data){

	uint8_t NumberPayloadBytes = 0;
	for(int i=0; i<DATA_PAYLOAD; i++){
		if ((data->payload[i]!=0x00) && (NumberPayloadBytes!=0)){
			NumberPayloadBytes++;
			}else{break;}
	}
	return NumberPayloadBytes;
}

CommandTypeState SendData(FormatCommandUART *data) {
	uint16_t crcValue = 0;
	uint8_t buffer_crc[61];
	InitStartByteTXData();
	if (data->payload_len == 5) {
		buffer_crc[0] = data->command;
		buffer_crc[1] = data->status;
		buffer_crc[2] = data->payload_len;

		crcValue = CalcCRC(buffer_crc, DATA_PAYLOAD + 3);
		data->crc_one = (crcValue >> 8) & 0xFF;
		data->crc_two = crcValue & 0xFF;

		while (HAL_UART_Transmit_DMA(active_uart, (uint8_t*)data, UART_BUFFER_SIZE) != HAL_OK) {
			return STATE_ERROR;
		}
	} else {
		data->status = RESP_OK;
		data->payload_len = CheckPayloadlen(data);
		buffer_crc[0] = data->command;
		buffer_crc[1] = data->status;
		buffer_crc[2] = data->payload_len;
		memcpy(&buffer_crc[3], data->payload, DATA_PAYLOAD);
		crcValue = CalcCRC(buffer_crc, DATA_PAYLOAD + 3);
		data->crc_one = (crcValue >> 8) & 0xFF;
		data->crc_two = crcValue & 0xFF;
		while (HAL_UART_Transmit_DMA(active_uart, (uint8_t*)data, UART_BUFFER_SIZE) != HAL_OK) {
			return STATE_ERROR;
		}
	}
	HAL_Delay(10);
	return STATE_OK;
}

CommandTypeState ReceiveData(FormatCommandUART *data) {
	uint8_t buffer_crc[61];

	while (HAL_UART_Receive_DMA(active_uart, (uint8_t*)data, UART_BUFFER_SIZE) == HAL_OK) {
		if (CheckStartBytes(data)) {
			uint16_t ReceivedCRC = ((uint16_t)data->crc_one << 8) | data->crc_two;

			buffer_crc[0] = data->command;
			buffer_crc[1] = data->status;
			buffer_crc[2] = data->payload_len;

			memcpy(&buffer_crc[3], data->payload, DATA_PAYLOAD);
			uint16_t CalculateCRC = CalcCRC(buffer_crc, DATA_PAYLOAD + 3);

			if (ReceivedCRC != CalculateCRC) {
				return STATE_BAD_CRC;
			} else {
				return STATE_OK;
			}
		}
	}
	return STATE_ERROR;
}


// Колбэки
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == active_uart->Instance) {
		// Обработка завершения передачи
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == active_uart->Instance) {
		// Обработка завершения приёма
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == active_uart->Instance) {
		HAL_UART_DMAStop(active_uart);
		__HAL_UART_CLEAR_PEFLAG(active_uart);
		HAL_UART_Receive_DMA(active_uart, (uint8_t*)&RXData, sizeof(FormatCommandUART));
	}
}


CommandTypeState WaitSync(void) {

	StateProcess status = ReceiveData(&RXData);

	if (status == STATE_OK && RXData.command == CMD_WAIT_SYNC) {
		TXData.command = CMD_WAIT_SYNC;
		SendData(&TXData);
		return STATE_READY_FOR_COMMAND;
	} else if (status == STATE_TIMEOUT) {
		TransmitError(status,RESP_TIMEOUT);
		return STATE_WAIT_SYNC;
	} else if (status == STATE_BAD_CRC){
		TransmitError(status,RESP_INVALID_CRC); //(uint8_t cmd_in_error, ResponseStatus error_code)
		return STATE_WAIT_SYNC;
	}
	return STATE_ERROR;
}

static CommandTypeState WaitCommandState(void) {

	StateProcess status = ReceiveData(&RXData);
	if (status != STATE_ERROR) {
		switch (RXData.command) {
			case CMD_WAIT_SYNC :
				return STATE_WAIT_SYNC;
			case CMD_GET_DATA:
				return STATE_TRANSMIT_DATA;
		}
	}
	return STATE_READY_FOR_COMMAND;
}

CommandTypeState ProcessState(CommandTypeState CurrentState) {
	switch (CurrentState) {
	case STATE_WAIT_SYNC:
		return WaitSync();
	case STATE_READY_FOR_COMMAND:
		return WaitCommandState();
	case STATE_TRANSMIT_DATA:
		return SendData(&TXData);
	case STATE_UNKNOWN_COMMAND:
		return STATE_READY_FOR_COMMAND;
	default:
		return STATE_READY_FOR_COMMAND;
	}
}
