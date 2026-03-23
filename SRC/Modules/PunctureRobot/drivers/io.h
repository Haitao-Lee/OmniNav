#ifndef __IO_H
#define __IO_H

#include <stdint.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#ifndef WINAPI
#define WINAPI
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

	enum IO_MODE
	{
		IO_MODE_INPUT,			//Input mode.
		IO_MODE_OUTPUT,			//Output mode.
		IO_MODE_OPEN_DRAIN,		//Open-drain mode.
	};
	typedef enum IO_MODE IO_MODE_t;

	enum IO_PULL
	{
		IO_NOPULL,			//No pull-up/down.
		IO_PULLUP,			//Pull-up.
		IO_PULLDOWN,		//Pull-down.
	};
	typedef enum IO_PULL IO_PULL_t;

	enum IO_STATE
	{
		IO_STATE_LOW,		//Low level.
		IO_STATE_HIGH,		//High level.
	};
	typedef enum IO_STATE IO_STATE_t;

	struct IO_InitStruct_Tx
	{
		uint8_t Pin;
		uint8_t Mode;
		uint8_t Pull;
	};
	typedef struct IO_InitStruct_Tx IO_InitStruct_Tx_t;

	struct IO_InitStruct_Rx
	{
		uint8_t Ret;
	};
	typedef struct IO_InitStruct_Rx IO_InitStruct_Rx_t;

	struct IO_ReadStruct_Tx
	{
		uint8_t Pin;
	};
	typedef struct IO_ReadStruct_Tx IO_ReadStruct_Tx_t;

	struct IO_ReadStruct_Rx
	{
		uint8_t Ret;
		uint8_t PinState;
	};
	typedef struct IO_ReadStruct_Rx IO_ReadStruct_Rx_t;

	struct IO_WriteStruct_Tx
	{
		uint8_t Pin;
		uint8_t PinState;
	};
	typedef struct IO_WriteStruct_Tx IO_WriteStruct_Tx_t;
	
	struct IO_WriteStruct_Rx
	{
		uint8_t Ret;
	};
	typedef struct IO_WriteStruct_Rx IO_WriteStruct_Rx_t;
	
	//Initialize pin mode.
	//SerialNumber: device serial number.
	//Pin：Pin: pin index. 0 = P0, 1 = P1...
	//Mode：Mode: I/O mode. 0 = input, 1 = output, 2 = open-drain.
	//Pull：Pull: pull resistor. 0 = none, 1 = internal pull-up, 2 = internal pull-down.
	//Return: 0 for success; <0 for error.
	int WINAPI IO_InitPin(int SerialNumber, int Pin, int Mode, int Pull);
	//Read pin state.
	//SerialNumber: device serial number.
	//Pin：Pin: pin index. 0 = P0, 1 = P1...
	//PinState：PinState: returned pin state. 0 = low, 1 = high.
	//Return: 0 for success; <0 for error.
	int WINAPI IO_ReadPin(int SerialNumber, int Pin, int *PinState);
	//Set pin output state.
	//SerialNumber: device serial number.
	//Pin：Pin: pin index. 0 = P0, 1 = P1...
	//PinState：PinState: pin state. 0 = low, 1 = high.
	//Return: 0 for success; <0 for error.
	int WINAPI IO_WritePin(int SerialNumber, int Pin, int PinState);

	int WINAPI IO_InitMultiPin(int SerialNumber, IO_InitStruct_Tx_t* TxStruct, IO_InitStruct_Rx_t* RxStruct, int Number);
	int WINAPI IO_ReadMultiPin(int SerialNumber, IO_ReadStruct_Tx_t* TxStruct, IO_ReadStruct_Rx_t* RxStruct, int Number);
	int WINAPI IO_WriteMultiPin(int SerialNumber, IO_WriteStruct_Tx_t* TxStruct, IO_WriteStruct_Rx_t* RxStruct, int Number);

#if defined(__cplusplus)
}
#endif

#endif

