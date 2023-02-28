Required libraries:
- hd44780 (https://github.com/duinoWitchery/hd44780)


| GCLK number   | Device |
| ------------- | ------------- |
| 1  |  |
| 2  | |
| 3  | RC decoder loop (TC3) |
| 4  | RC decoder loop (Sercom 2) |
| 5  | Servo (TCC0), Motors (TCC1) |
| 6  | Logic Engine (Sercom 4) |

| Device  | I2C |
| ------------- | ------------- |
| Tsunami Audio Board  | 0x13 |
| LCD | 0x27 |
| REON front  | 0x19 |
| REON top  | 0x1A |
| REON rear  | 0x1B |
| speaker amp | 0x58 |

packet format

| Command  | byte 1 (address) | byte 2 | byte 3 | byte 4 | byte 5 | byte 6 | byte 7 | byte 8 |
| ------------- | ------------- | --- | ------------- | ------------- | --- | ------------- | ------------- | --- |
| Left Motor Command  | 0xFF  | 1 | 0 | left low byte  | CRC1 | CRC2 | | |
| Right Motor Command | 0xFF  | 1 | 1 | left low byte  | CRC1 | CRC2 | | |
| Set Servo | 0xFF  | 2 | 2 | servo number | servo position | CRC1 | CRC2 | |
| LCD Cursor | 0xFF  | 1 | 3 | column (upper 6 bits), row (lower 2 bits) | CRC1 | CRC2 | |
| Print String | 0xFF  | n | 4 | n-byte payload | CRC1 | CRC2 | |

| Set Servo | 0xFF | 6 | servo num  | servo position high | servo position low | CRC1 | CRC2 | |
| Clear Screen | 0xFF | 0 | CRC1 | CRC2 | | | | |
| Set Cursor | 0xFF | 1 | column  | row | CRC1 | CRC2 | | |
| Print String | 0xFF | 2 | string length (n) | n bytes... | ... | ... | CRC1  | CRC2 |
| Print Integer | 0xFF | 3 | lowest byte | ... | ... | highest byte | CRC1  | CRC2 |
| Backlight Off | 0xFF | 4 | CRC1 | CRC2 | | | | |
| Backlight On | 0xFF | 5 | CRC1 | CRC2 | | | | |
| Set Servo | 0xFF | 6 | servo num  | servo position high | servo position low | CRC1 | CRC2 | |
| LED On | 0xFF | 7 | LED number | CRC1 | CRC2 | | | |
| LED Off | 0xFF | 8 | LED number | CRC1 | CRC2 | | | |
