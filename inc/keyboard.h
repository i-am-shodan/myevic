#ifndef KEYBOARD_h
#define KEYBOARD_h

#include "myevic.h"
#include "meusbd.h"

//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
	uint8_t modifiers;
	uint8_t reserved;
	uint8_t keys[6];
} KeyReport;

extern const S_USBD_INFO_T usbdKBDDescriptors;

extern void KBD_ClassRequest(uint8_t * key);
extern void KBD_SendKey(uint8_t key);
extern void KBD_SendKeys(const char * keys);
extern void KBD_SendRawReport(KeyReport * report);

#endif