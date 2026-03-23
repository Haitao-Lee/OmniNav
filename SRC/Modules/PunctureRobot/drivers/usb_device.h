#ifndef __USB_DEVICE_H
#define __USB_DEVICE_H

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

	//Scan USB devices.
	//Return value > 0 is the number of devices found; 0 means no device; < 0 indicates an error.
	int WINAPI UsbDevice_Scan(int* SerialNumbers);

#if defined(__cplusplus)
}
#endif

#endif

