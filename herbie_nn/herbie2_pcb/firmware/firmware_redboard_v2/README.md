packet format

| Command  | byte 1 | byte 2 | byte 3 | byte 4 | byte 5 | byte 6 | byte 7 | byte 8 |
| ------------- | ------------- | --- | ------------- | ------------- | --- | ------------- | ------------- | --- |
| Motor Command  | 128  | 34 | left high byte | left low byte  | right high byte | right low byte | CRC1 | CRC2 |
| Clear Screen | 128 | 0 | empty  | empty | empty | empty | CRC1  | CRC2 |
| Set Cursor | 128 | 1 | row  | column | empty | empty | CRC1  | CRC2 |
| Print String | 128 | 2 | string length (n) | n bytes... | ... | ... | CRC1  | CRC2 |
| Set Servo | 128 | 6 | servo num  | servo position high | servo position low | empty | CRC1  | CRC2 |
| LED On | 128 | 7 | LED number | empty | empty | empty | CRC1  | CRC2 |
| LED Off | 128 | 8 | LED number | empty | empty | empty | CRC1  | CRC2 |
