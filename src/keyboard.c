#include "keyboard.h"
#include "dataflash.h"
#include "timers.h"

// Substaintial amounts of code here has been taken from the Arduino SDK
// As such GPLv2 applies here
//https://github.com/arduino/Arduino
//https://github.com/arduino/Arduino/blob/master/license.txt

/*!<Define HID Class Specific Request */
#define KBD_GET_REPORT          0x01
#define KBD_GET_IDLE            0x02
#define KBD_GET_PROTOCOL        0x03
#define KBD_SET_REPORT          0x09
#define KBD_SET_IDLE            0x0A
#define KBD_SET_PROTOCOL        0x0B

/*!<USB HID Interface Class protocol */
#define KBD_HID_NONE            0x00
#define KBD_HID_KEYBOARD        0x01
#define KBD_HID_MOUSE           0x02

/*!<USB HID Class Report Type */
#define KBD_HID_RPT_TYPE_INPUT      0x01
#define KBD_HID_RPT_TYPE_OUTPUT     0x02
#define KBD_HID_RPT_TYPE_FEATURE    0x03

/*-------------------------------------------------------------*/
/* Define EP maximum packet size */
#define KBD_EP0_MAX_PKT_SIZE    8
#define KBD_EP1_MAX_PKT_SIZE    KBD_EP0_MAX_PKT_SIZE
#define KBD_EP2_MAX_PKT_SIZE    8

#define KBD_SETUP_BUF_BASE  0
#define KBD_SETUP_BUF_LEN   8
#define KBD_EP0_BUF_BASE    (KBD_SETUP_BUF_BASE + KBD_SETUP_BUF_LEN)
#define KBD_EP0_BUF_LEN     KBD_EP0_MAX_PKT_SIZE
#define KBD_EP1_BUF_BASE    (KBD_SETUP_BUF_BASE + KBD_SETUP_BUF_LEN)
#define KBD_EP1_BUF_LEN     KBD_EP1_MAX_PKT_SIZE
#define KBD_EP2_BUF_BASE    (KBD_EP1_BUF_BASE + KBD_EP1_BUF_LEN)
#define KBD_EP2_BUF_LEN     KBD_EP2_MAX_PKT_SIZE

/* Define the interrupt In EP number */
#define KBD_INT_IN_EP_NUM   0x01

/* Define Descriptor information */
#define KBD_HID_DEFAULT_INT_IN_INTERVAL     20
#define KBD_USBD_SELF_POWERED               0
#define KBD_USBD_REMOTE_WAKEUP              0
#define KBD_USBD_MAX_POWER                  50  /* The unit is in 2mA. ex: 50 * 2mA = 100mA */

#define KBD_LEN_CONFIG_AND_SUBORDINATE      (LEN_CONFIG+LEN_INTERFACE+LEN_HID+LEN_ENDPOINT)

/*!<USB HID Report Descriptor */
const uint8_t HID_KeyboardReportDescriptor[] =
{
	0x05, 0x01,     /* Usage Page(Generic Desktop Controls) */
	0x09, 0x06,     /* Usage(Keyboard) */
	0xA1, 0x01,     /* Collection(Application) */
	0x05, 0x07,     /* Usage Page(Keyboard/Keypad) */
	0x19, 0xE0,     /* Usage Minimum(0xE0) */
	0x29, 0xE7,     /* Usage Maximum(0xE7) */
	0x15, 0x00,     /* Logical Minimum(0x0) */
	0x25, 0x01,     /* Logical Maximum(0x1) */
	0x75, 0x01,     /* Report Size(0x1) */
	0x95, 0x08,     /* Report Count(0x8) */
	0x81, 0x02,     /* Input (Data) => Modifier byte */
	0x95, 0x01,     /* Report Count(0x1) */
	0x75, 0x08,     /* Report Size(0x8) */
	0x81, 0x01,     /* Input (Constant) => Reserved byte */
	0x95, 0x05,     /* Report Count(0x5) */
	0x75, 0x01,     /* Report Size(0x1) */
	0x05, 0x08,     /* Usage Page(LEDs) */
	0x19, 0x01,     /* Usage Minimum(0x1) */
	0x29, 0x05,     /* Usage Maximum(0x5) */
	0x91, 0x02,     /* Output (Data) => LED report */
	0x95, 0x01,     /* Report Count(0x1) */
	0x75, 0x03,     /* Report Size(0x3) */
	0x91, 0x01,     /* Output (Constant) => LED report padding */
	0x95, 0x06,     /* Report Count(0x6) */
	0x75, 0x08,     /* Report Size(0x8) */
	0x15, 0x00,     /* Logical Minimum(0x0) */
	0x25, 0x65,     /* Logical Maximum(0x65) */
	0x05, 0x07,     /* Usage Page(Keyboard/Keypad) */
	0x19, 0x00,     /* Usage Minimum(0x0) */
	0x29, 0x65,     /* Usage Maximum(0x65) */
	0x81, 0x00,     /* Input (Data) */
	0xC0            /* End Collection */
};

const uint8_t gu8DeviceDescriptor_kbd[] =
{
	LEN_DEVICE,     /* bLength */
	DESC_DEVICE,    /* bDescriptorType */
	0x10, 0x01,     /* bcdUSB */
	0x00,           /* bDeviceClass */
	0x00,           /* bDeviceSubClass */
	0x00,           /* bDeviceProtocol */
	KBD_EP0_MAX_PKT_SIZE,   /* bMaxPacketSize0 */
							/* idVendor */
	USBD_VID & 0x00FF,
	(USBD_VID & 0xFF00) >> 8,
	/* idProduct */
	USBD_PID & 0x00FF,
	(USBD_PID & 0xFF00) >> 8,
	0x00, 0x00,     /* bcdDevice */
	0x01,           /* iManufacture */
	0x02,           /* iProduct */
	0x03,           /* iSerialNumber - no serial */
	0x01            /* bNumConfigurations */
};

/*----------------------------------------------------------------------------*/

/*!<USB Configure Descriptor */
const uint8_t usbdKBDConfigDesc[] =
{
	LEN_CONFIG,     /* bLength */
	DESC_CONFIG,    /* bDescriptorType */
					/* wTotalLength */
	KBD_LEN_CONFIG_AND_SUBORDINATE & 0x00FF,
	(KBD_LEN_CONFIG_AND_SUBORDINATE & 0xFF00) >> 8,
	0x01,           /* bNumInterfaces */
	0x01,           /* bConfigurationValue */
	0x00,           /* iConfiguration */
	0x80 | (KBD_USBD_SELF_POWERED << 6) | (KBD_USBD_REMOTE_WAKEUP << 5),/* bmAttributes */
	KBD_USBD_MAX_POWER,         /* MaxPower */

								/* I/F descr: HID */
	LEN_INTERFACE,  /* bLength */
	DESC_INTERFACE, /* bDescriptorType */
	0x00,           /* bInterfaceNumber */
	0x00,           /* bAlternateSetting */
	0x01,           /* bNumEndpoints */
	0x03,           /* bInterfaceClass */
	0x01,           /* bInterfaceSubClass */
	KBD_HID_KEYBOARD,   /* bInterfaceProtocol */
	0x00,           /* iInterface */

					/* HID Descriptor */
	LEN_HID,        /* Size of this descriptor in UINT8s. */
	DESC_HID,       /* HID descriptor type. */
	0x10, 0x01,     /* HID Class Spec. release number. */
	0x00,           /* H/W target country. */
	0x01,           /* Number of HID class descriptors to follow. */
	DESC_HID_RPT,   /* Descriptor type. */
					/* Total length of report descriptor. */
	sizeof(HID_KeyboardReportDescriptor) & 0x00FF,
	(sizeof(HID_KeyboardReportDescriptor) & 0xFF00) >> 8,

	/* EP Descriptor: interrupt in. */
	LEN_ENDPOINT,   /* bLength */
	DESC_ENDPOINT,  /* bDescriptorType */
	(KBD_INT_IN_EP_NUM | EP_INPUT), /* bEndpointAddress */
	EP_INT,         /* bmAttributes */
					/* wMaxPacketSize */
	KBD_EP2_MAX_PKT_SIZE & 0x00FF,
	(KBD_EP2_MAX_PKT_SIZE & 0xFF00) >> 8,
	KBD_HID_DEFAULT_INT_IN_INTERVAL     /* bInterval */
};

const uint8_t *gpu8UsbHidReport_kbd[3] =
{
	HID_KeyboardReportDescriptor,
	NULL,
	NULL
};

const uint32_t gu32UsbHidReportLen_kbd[3] = {
	sizeof(HID_KeyboardReportDescriptor),
	0,
	0
};


const S_USBD_INFO_T usbdKBDDescriptors =
{
	gu8DeviceDescriptor_kbd,
	usbdKBDConfigDesc,
	usbdStrings,
	gpu8UsbHidReport_kbd,
	gu32UsbHidReportLen_kbd,
	usbdConfigHidDescIdx
};

//================================================================================
//================================================================================
//  Keyboard

#define KEY_LEFT_CTRL   0x80
#define KEY_LEFT_SHIFT    0x81
#define KEY_LEFT_ALT    0x82
#define KEY_LEFT_GUI    0x83
#define KEY_RIGHT_CTRL    0x84
#define KEY_RIGHT_SHIFT   0x85
#define KEY_RIGHT_ALT   0x86
#define KEY_RIGHT_GUI   0x87

#define KEY_UP_ARROW    0xDA
#define KEY_DOWN_ARROW    0xD9
#define KEY_LEFT_ARROW    0xD8
#define KEY_RIGHT_ARROW   0xD7
#define KEY_BACKSPACE   0xB2
#define KEY_TAB       0xB3
#define KEY_RETURN      0xB0
#define KEY_ESC       0xB1
#define KEY_INSERT      0xD1
#define KEY_DELETE      0xD4
#define KEY_PAGE_UP     0xD3
#define KEY_PAGE_DOWN   0xD6
#define KEY_HOME      0xD2
#define KEY_END       0xD5
#define KEY_CAPS_LOCK   0xC1
#define KEY_F1        0xC2
#define KEY_F2        0xC3
#define KEY_F3        0xC4
#define KEY_F4        0xC5
#define KEY_F5        0xC6
#define KEY_F6        0xC7
#define KEY_F7        0xC8
#define KEY_F8        0xC9
#define KEY_F9        0xCA
#define KEY_F10       0xCB
#define KEY_F11       0xCC
#define KEY_F12       0xCD

__myevic__ void KBD_SendRawReport(KeyReport *keyReport);

__myevic__ void KBD_ClassRequest(uint8_t *token)
{
	if (token[0] & 0x80)    /* request data transfer direction */
	{
		// Device to host
		switch (token[1])
			{
			case KBD_GET_REPORT:
			case KBD_GET_IDLE:
			case KBD_GET_PROTOCOL:
			default:
			{
				/* Setup error, stall the device */
				USBD_SetStall(EP0);
				USBD_SetStall(EP1);
				break;
			}
		}
	}
	else
	{
		// Host to device
		switch (token[1])
		{
			case KBD_SET_REPORT:
			{
				if (token[3] == 2)
				{
					/* Request Type = Output */
					USBD_SET_DATA1(EP1);
					USBD_SET_PAYLOAD_LEN(EP1, token[6]);

					/* Status stage */
					USBD_PrepareCtrlIn(0, 0);
				}
				break;
			}
			case KBD_SET_IDLE:
			{
				/* Status stage */
				USBD_SET_DATA1(EP0);
				USBD_SET_PAYLOAD_LEN(EP0, 0);
				break;
			}
			case KBD_SET_PROTOCOL:
			default:
			{
				// Stall
				/* Setup error, stall the device */
				USBD_SetStall(EP0);
				USBD_SetStall(EP1);
				break;
			}
		}
	}
}

// https://github.com/arduino/Arduino
#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK  
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT 
	0x00,             // FF 
	0x00,             // CR 
	0x00,             // SO 
	0x00,             // SI 
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM 
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS 
	0x00,             // GS 
	0x00,             // RS 
	0x00,             // US 

	0x2c,		   //  ' '
	0x1e | SHIFT,	   // !
	0x34 | SHIFT,	   // "
	0x20 | SHIFT,    // #
	0x21 | SHIFT,    // $
	0x22 | SHIFT,    // %
	0x24 | SHIFT,    // &
	0x34,          // '
	0x26 | SHIFT,    // (
	0x27 | SHIFT,    // )
	0x25 | SHIFT,    // *
	0x2e | SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33 | SHIFT,      // :
	0x33,          // ;
	0x36 | SHIFT,      // <
	0x2e,          // =
	0x37 | SHIFT,      // >
	0x38 | SHIFT,      // ?
	0x1f | SHIFT,      // @
	0x04 | SHIFT,      // A
	0x05 | SHIFT,      // B
	0x06 | SHIFT,      // C
	0x07 | SHIFT,      // D
	0x08 | SHIFT,      // E
	0x09 | SHIFT,      // F
	0x0a | SHIFT,      // G
	0x0b | SHIFT,      // H
	0x0c | SHIFT,      // I
	0x0d | SHIFT,      // J
	0x0e | SHIFT,      // K
	0x0f | SHIFT,      // L
	0x10 | SHIFT,      // M
	0x11 | SHIFT,      // N
	0x12 | SHIFT,      // O
	0x13 | SHIFT,      // P
	0x14 | SHIFT,      // Q
	0x15 | SHIFT,      // R
	0x16 | SHIFT,      // S
	0x17 | SHIFT,      // T
	0x18 | SHIFT,      // U
	0x19 | SHIFT,      // V
	0x1a | SHIFT,      // W
	0x1b | SHIFT,      // X
	0x1c | SHIFT,      // Y
	0x1d | SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23 | SHIFT,    // ^
	0x2d | SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f | SHIFT,    // {
	0x64 | SHIFT,    // |
	0x30 | SHIFT,    // }
	0x32 | SHIFT,    // ~
	0				// DEL
};


typedef unsigned int size_t;

void ZeroReport(KeyReport *keys)
{
	int i = 0;
	for (i = 0; i < 6; i++)
	{
		keys->keys[i] = 0;
	}
	keys->modifiers = 0;
	keys->reserved = 0;
}

//https://github.com/arduino/Arduino
size_t PressKey(uint8_t k)
{
	KeyReport _keyReport = { 0 };

	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	}
	else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1 << (k - 128));
		k = 0;
	}
	else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			return 0;
		}
		if (k & 0x80) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i = 0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			return 0;
		}
	}
	KBD_SendRawReport(&_keyReport);
	return 1;
}

//https://github.com/arduino/Arduino
// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t ReleaseKey(uint8_t k)
{
	KeyReport _keyReport = { 0 };

	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	}
	else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1 << (k - 128));
		k = 0;
	}
	else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i = 0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}

	KBD_SendRawReport(&_keyReport);
	return 1;
}

__myevic__ void KBD_SendKey(uint8_t key)
{
	(void)PressKey(key);  // Keydown
	ReleaseKey(key);            // Keyup
	ResetWatchDog();
}

__myevic__ void KBD_SendKeys(const char * keys)
{
	int i = 0;
	while (keys != NULL && keys[i] != '\0')
	{
		KBD_SendKey(keys[i]);
		++i;
	}
}

void KBD_SendRawReport(KeyReport *keyReport)
{
	int i = 0;
	uint32_t shouldHIDMessageBeSent;

	if (dfStatus.keyboard)
	{
		KeyReport *buf = (KeyReport *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));
		ZeroReport(buf);

		shouldHIDMessageBeSent = (PB->PIN & (1 << 5)) ? 0 : 1;

		if (shouldHIDMessageBeSent)
		{
			for (i = 0; i < 6; i++)
			{
				buf->keys[i] = keyReport->keys[i];
			}
			buf->modifiers = keyReport->modifiers;
			buf->reserved = keyReport->reserved;

			USBD_SET_PAYLOAD_LEN(EP2, sizeof(KeyReport));
			TIMER_Delay(TIMER0, 30000);
		}
	}
}