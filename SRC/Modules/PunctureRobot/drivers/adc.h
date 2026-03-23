#ifndef __ADC_H
#define __ADC_H

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
	
	//Initialize ADC.
	//SerialNumber: device serial number.
	//Channel: channel index. 0 = ADC0, 1 = ADC1...
	//SampleRateHz: sampling rate; usually set to 0.
	//Return: 0 for success; <0 for error.
	int WINAPI ADC_Init(int SerialNumber, int Channel, int SampleRateHz);
	
	//Read ADC.
	//SerialNumber: device serial number.
	//Channel: channel index. 0 = ADC0, 1 = ADC1...
	//Value: ADC value.
	//Return: 0 for success; <0 for error.
	int WINAPI ADC_Read(int SerialNumber, int Channel, int *Value);

#if defined(__cplusplus)
}
#endif

#endif

