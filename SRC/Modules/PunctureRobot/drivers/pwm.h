#ifndef __PWM_H
#define __PWM_H

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
	
	// 初始化PWM
	// SerialNumber: 设备序号
	// Channel: 通道编号。0，PWM0. 1, PWM1 ...
	// Prescaler: 预分频器。范围1~65535
	// Precision: 脉冲精度。范围1~65535. PWM频率=72MHz/(Prescaler*Precision)
	// Duty: 占空比。范围0~Precision。实际占空比=(Duty/Precision)*100%
	// Phase: 波形相位。范围0~Precision-1。
	// Polarity: 波形极性。范围0~1。
	// 函数返回：0，正常；<0，异常
	int WINAPI PWM_Init(int SerialNumber, int Channel, int Prescaler, int Precision, int Duty, int Phase, int Polarity);
	
	// PWM开始输出
	// Channel: 通道编号。0，PWM0. 1, PWM1 ...
	// RunTimeUs: 输出波形的时间，单位为微妙，启动波形输出之后，RunTimeOfUs微妙之后会停止波形输出，该参数为0，波形会一直输出，直到手动停止，利用该参数可以控制脉冲输出个数。
	// 函数返回：0，正常；<0，异常
	int WINAPI PWM_Start(int SerialNumber, int Channel, int RunTimeUs);

	// PWM停止输出
	// Channel: 通道编号。0，PWM0. 1, PWM1 ...
	// 函数返回：0，正常；<0，异常
	int WINAPI PWM_Stop(int SerialNumber, int Channel);

	// PWM占空比动态调节。可以在PWM启动之后调用
	// Channel: 通道编号。0，PWM0. 1, PWM1 ...
	// Duty: 占空比。范围0~Precision。实际占空比=(Duty/Precision)*100%
	// 函数返回：0，正常；<0，异常
	int WINAPI PWM_SetDuty(int SerialNumber, int Channel, int Duty);

	// PWM频率动态调节。可以在PWM启动之后调用
	// Channel: 通道编号。0，PWM0. 1, PWM1 ...
	// Prescaler: 预分频器。范围1~65535
	// Precision: 脉冲精度。范围1~65535
	// 函数返回：0，正常；<0，异常
	int WINAPI PWM_SetFrequency(int SerialNumber, int Channel, int Prescaler, int Precision);

#if defined(__cplusplus)
}
#endif

#endif
