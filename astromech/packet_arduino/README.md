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
