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
	
	//初始化ADC
	//SerialNumber: 设备序号
	//Channel：通道编号。0，ADC0. 1, ADC1...
	//SampleRateHz：采样率。一般设置0
	//函数返回：0，正常；<0，异常
	int WINAPI ADC_Init(int SerialNumber, int Channel, int SampleRateHz);
	
	//ADC读取
	//SerialNumber: 设备序号
	//Channel：通道编号。0，ADC0. 1, ADC1...
	//Value：AD值
	//函数返回：0，正常；<0，异常
	int WINAPI ADC_Read(int SerialNumber, int Channel, int *Value);

#if defined(__cplusplus)
}
#endif

#endif
