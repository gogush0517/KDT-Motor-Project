#include "device_driver.h"

#if 01
void Main(void)
{
	Sys_Init(115200);
	Total_Init();

	for(;;)
	{
		Event_Handler();
		Op_Handler();
	}
}

#endif
