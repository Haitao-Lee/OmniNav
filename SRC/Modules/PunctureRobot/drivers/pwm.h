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
	
	// Initialize PWM.
	// SerialNumber: device serial number.
	// Channel: channel index. 0 = PWM0, 1 = PWM1 ...
	// Prescaler: prescaler, range 1~65535.
	// Precision: pulse precision, range 1~65535. PWM frequency = 72 MHz/(Prescaler*Precision).
	// Duty: duty cycle, range 0~Precision. Actual duty = (Duty/Precision)*100%.
	// Phase: waveform phase, range 0~Precision-1.
	// Polarity: waveform polarity, range 0~1.
	// Return: 0 for success; <0 for error.
	int WINAPI PWM_Init(int SerialNumber, int Channel, int Prescaler, int Precision, int Duty, int Phase, int Polarity);
	
	// Start PWM output.
	// Channel: channel index. 0 = PWM0, 1 = PWM1 ...
	// RunTimeUs: output duration in microseconds; after starting, output stops after RunTimeUs us. If 0, output runs until stopped manually; use this to control pulse count.
	// Return: 0 for success; <0 for error.
	int WINAPI PWM_Start(int SerialNumber, int Channel, int RunTimeUs);

	// Stop PWM output.
	// Channel: channel index. 0 = PWM0, 1 = PWM1 ...
	// Return: 0 for success; <0 for error.
	int WINAPI PWM_Stop(int SerialNumber, int Channel);

	// Adjust PWM duty cycle dynamically; can be called after PWM starts.
	// Channel: channel index. 0 = PWM0, 1 = PWM1 ...
	// Duty: duty cycle, range 0~Precision. Actual duty = (Duty/Precision)*100%.
	// Return: 0 for success; <0 for error.
	int WINAPI PWM_SetDuty(int SerialNumber, int Channel, int Duty);

	// Adjust PWM frequency dynamically; can be called after PWM starts.
	// Channel: channel index. 0 = PWM0, 1 = PWM1 ...
	// Prescaler: prescaler, range 1~65535.
	// Precision: pulse precision, range 1~65535.
	// Return: 0 for success; <0 for error.
	int WINAPI PWM_SetFrequency(int SerialNumber, int Channel, int Prescaler, int Precision);

#if defined(__cplusplus)
}
#endif

#endif


