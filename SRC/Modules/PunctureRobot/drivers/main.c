#include "stdio.h"
#include "usb_device.h"
#include "io.h"
#include "adc.h"
#include "pwm.h"

int main(void)
{
	int ret = 0, i;
	int SerialNumbers[16];
	int Value = 0;
	
	ret = UsbDevice_Scan(SerialNumbers);
	if (ret < 0)
	{
		printf("Scan error: %d\n", ret);
		return ret;
	}
	else if (ret == 0)
	{
		printf("No device\n");
		return ret;
	}
	else
	{
		for (i = 0; i < ret; i++)
		{
			printf("Dev%d SN: %d\n", i, SerialNumbers[i]);
		}
	}
	
	//初始化pwm0
	//频率 = 72MHz / (2*36000) = 1KHz
	//占空比 = 18000 / 36000 * 100% = 50%
	ret = PWM_Init(SerialNumbers[0], 0, 2, 36000, 18000, 0, 0);
	if (ret < 0)
	{
		printf("Error: %d\n", ret);
		return ret;
	}

	//PWM开始输出
	ret = PWM_Start(SerialNumbers[0], 0, 0);
	if (ret < 0)
	{
		printf("Error: %d\n", ret);
		return ret;
	}

	//PWM停止输出
	ret = PWM_Stop(SerialNumbers[0], 0);
	if (ret < 0)
	{
		printf("Error: %d\n", ret);
		return ret;
	}
}
