packet format

| Command  | byte 1 (address) | byte 2 | byte 3 | byte 4 | byte 5 | byte 6 | byte 7 | byte 8 |
| ------------- | ------------- | --- | ------------- | ------------- | --- | ------------- | ------------- | --- |
| Motor Command  | 128  | 34 | left high byte | left low byte  | right high byte | right low byte | CRC1 | CRC2 |
| Clear Screen | 128 | 0 | empty  | empty | empty | empty | CRC1  | CRC2 |
| Set Cursor | 128 | 1 | column  | row | CRC1 | CRC2 | | |
| Print String | 128 | 2 | string length (n) | n bytes... | ... | ... | CRC1  | CRC2 |
| Print Integer | 128 | 3 | lowest byte | ... | ... | highest byte | CRC1  | CRC2 |
| Backlight Off | 128 | 4 | empty | empty | empty | empty | CRC1  | CRC2 |
| Backlight On | 128 | 5 | empty | empty | empty | empty | CRC1  | CRC2 |
| Set Servo | 128 | 6 | servo num  | servo position high | servo position low | empty | CRC1  | CRC2 |
| LED On | 128 | 7 | LED number | CRC1 | CRC2 | | | |
| LED Off | 128 | 8 | LED number | CRC1 | CRC2 | | | |
