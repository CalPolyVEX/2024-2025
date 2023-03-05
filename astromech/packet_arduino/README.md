### How to compile the firmware

- Install the Arduino IDE
- In the IDE, go to:  
  - Tools -> Board -> Boards Manager...
  - Install Arduino SAMD Boards (32-bits ARM Cortex-M0+)
- In the IDE, go to:
  - File -> Settings -> Addition boards manager URLs
  - Paste:  https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json
- In the IDE, install the libraries:
  - hd44780 (https://github.com/duinoWitchery/hd44780)

---

| GCLK number   | Device |
| ------------- | ------------- |
| 1  |  |
| 2  | |
| 3  | RC decoder loop (TC3) |
| 4  | RC decoder loop (Sercom 2) |
| 5  | Servo (TCC0), Motors (TCC1) |
| 6  | Logic Engine (Sercom 4) |
| 7  |  |
| 8  |  |

---

| Device  | I2C address |
| ------------- | ------------- |
| Tsunami Audio Board  | 0x13 |
| LCD | 0x27 |
| REON front  | 0x19 |
| REON top  | 0x1A |
| REON rear  | 0x1B |
| speaker amp | 0x58 |
| TCA9534 (i/o expander - old) | 0x20 |
| TCA9554 (i/o expander - new) | 0x38 |

---

### Serial Packet Format
- This is the packet format sent from the computer to the control board over USB serial
- The CRC is CRC16 and is computed starting at the 0xFF header up to the last payload byte

| Command  | byte 1 (header) | byte 2 (payload size) | byte 3 (command) | byte 4 | byte 5 | byte 6 | byte 7 | byte 8 | byte 9 |
| ------------- | ------------- | --- | ------------- | ------------- | --- | ------------- | ------------- | --- | -- |
| Left Motor Command  | 0xFF  | 1 | 0 | motor speed | CRC1 | CRC2 | | |
| Right Motor Command | 0xFF  | 1 | 1 | motor speed | CRC1 | CRC2 | | |
| Set Servo | 0xFF  | 2 | 2 | servo number | servo position | CRC1 | CRC2 | |
| Set LCD Cursor | 0xFF  | 1 | 3 | column (upper 6 bits), row (lower 2 bits) | CRC1 | CRC2 | |
| Print String | 0xFF  | n | 4 | n-byte payload | CRC1 | CRC2 | |
| Set Cursor & Print String | 0xFF  | n | 5 | col (upper 6 bits), row (lower 2 bits) | (n-1)-byte string | CRC1 | CRC2 | |
| Clear Screen | 0xFF  | 0 | 6 | CRC1 | CRC2 | |
| Logic Engine Preset | 0xFF  | 1 | 7 | byte 1 | CRC1 | CRC2 | |
| Logic Engine Raw Command | 0xFF  | 4 | 8 | byte 1 | byte 2 | byte 3 | byte 4 | CRC1 | CRC2 | |
| Play Tsunami Sound | 0xFF  | 2 | 9 | byte 1 | byte 2 | CRC1 | CRC2 | |
| Set Amplifier Gain | 0xFF  | 1 | 10 | byte 1 | CRC1 | CRC2 | |
