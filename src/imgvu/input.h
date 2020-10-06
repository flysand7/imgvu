
// NOTE(bumbread): The imgvu input
// requires that virtual keys supplied
// to the program via t_button array
// are mapped to the values described below

// the t_button array supplied MUST be 0x100
// t_buttons long.

#define KEYBOARD_SIZE 0x100

#define VKEY_LBUTTON        0x01
#define VKEY_RBUTTON        0x02
#define VKEY_CANCEL         0x03
#define VKEY_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

#define VKEY_BACK           0x08
#define VKEY_TAB            0x09

#define VKEY_CLEAR          0x0C
#define VKEY_RETURN         0x0D

#define VKEY_SHIFT          0x10
#define VKEY_CONTROL        0x11
#define VKEY_MENU           0x12
#define VKEY_PAUSE          0x13
#define VKEY_CAPITAL        0x14

#define VKEY_ESCAPE         0x1B

#define VKEY_SPACE          0x20
#define VKEY_PRIOR          0x21
#define VKEY_NEXT           0x22
#define VKEY_END            0x23
#define VKEY_HOME           0x24
#define VKEY_LEFT           0x25
#define VKEY_UP             0x26
#define VKEY_RIGHT          0x27
#define VKEY_DOWN           0x28
#define VKEY_SELECT         0x29
#define VKEY_PRINT          0x2A
#define VKEY_EXECUTE        0x2B
#define VKEY_SNAPSHOT       0x2C
#define VKEY_INSERT         0x2D
#define VKEY_DELETE         0x2E
#define VKEY_HELP           0x2F

/*
 * VKEY_0 - VKEY_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x3A - 0x40 : unassigned
 * VKEY_A - VKEY_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

#define VKEY_LWIN           0x5B

#define VKEY_NUMPAD0        0x60
#define VKEY_NUMPAD1        0x61
#define VKEY_NUMPAD2        0x62
#define VKEY_NUMPAD3        0x63
#define VKEY_NUMPAD4        0x64
#define VKEY_NUMPAD5        0x65
#define VKEY_NUMPAD6        0x66
#define VKEY_NUMPAD7        0x67
#define VKEY_NUMPAD8        0x68
#define VKEY_NUMPAD9        0x69
#define VKEY_MULTIPLY       0x6A
#define VKEY_ADD            0x6B
#define VKEY_SEPARATOR      0x6C
#define VKEY_SUBTRACT       0x6D
#define VKEY_DECIMAL        0x6E
#define VKEY_DIVIDE         0x6F
#define VKEY_F1             0x70
#define VKEY_F2             0x71
#define VKEY_F3             0x72
#define VKEY_F4             0x73
#define VKEY_F5             0x74
#define VKEY_F6             0x75
#define VKEY_F7             0x76
#define VKEY_F8             0x77
#define VKEY_F9             0x78
#define VKEY_F10            0x79
#define VKEY_F11            0x7A
#define VKEY_F12            0x7B
#define VKEY_F13            0x7C
#define VKEY_F14            0x7D
#define VKEY_F15            0x7E
#define VKEY_F16            0x7F
#define VKEY_F17            0x80
#define VKEY_F18            0x81
#define VKEY_F19            0x82
#define VKEY_F20            0x83
#define VKEY_F21            0x84
#define VKEY_F22            0x85
#define VKEY_F23            0x86
#define VKEY_F24            0x87

#define VKEY_NUMLOCK        0x90
#define VKEY_SCROLL         0x91

#define VKEY_OEM_1          0xBA   // ';:' for US
#define VKEY_OEM_PLUS       0xBB   // '+' any country
#define VKEY_OEM_COMMA      0xBC   // ',' any country
#define VKEY_OEM_MINUS      0xBD   // '-' any country
#define VKEY_OEM_PERIOD     0xBE   // '.' any country
#define VKEY_OEM_2          0xBF   // '/?' for US
#define VKEY_OEM_3          0xC0   // '`~' for US

#define VKEY_OEM_4          0xDB  //  '[{' for US
#define VKEY_OEM_5          0xDC  //  '\|' for US
#define VKEY_OEM_6          0xDD  //  ']}' for US
#define VKEY_OEM_7          0xDE  //  ''"' for US
#define VKEY_OEM_8          0xDF

struct {
  bool pressed;
  bool released;
  bool down;
} typedef t_button;
