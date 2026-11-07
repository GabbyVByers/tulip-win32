
/*
 *   Source [main.c]
 */

#include "tulip.h"

int main(void)
{
	TULIP_WindowInit("App", 320, 200, 4);

	while (TULIP_WindowIsOpen())
	{
		TULIP_RenderClearFast();
		TULIP_RenderFrame();
	}

	TULIP_WindowQuit();
	return EXIT_SUCCESS;
}

