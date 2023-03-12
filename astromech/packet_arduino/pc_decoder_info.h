#ifndef PC_DECODER_INFO_H
#define PC_DECODER_INFO_H

#define PAYLOAD_LEN_IDX 1
#define CMD_IDX 2
#define HEADER_SIZE 3

/** Commands */
/** Motor Servo 0-9 */
#define LEFT_MOTOR_CMD 0
#define RIGHT_MOTOR_CMD 1
#define SERVO_CMD 2
/** LCD 10-19 */
#define LCD_CURSOR_CMD 10
#define LCD_PRINT_STR_CMD 11
#define LCD_CURSOR_PRINT_CMD 12
#define LCD_CLEAR_CMD 13
/** Logic Engine 20-29 */
#define LOGIC_PRESET_CMD 20
#define LOGIC_RAW_CMD 21
/** Audio Tsunami 30-39 */
#define TSUN_SOUND_CMD 30
#define TSUN_AMP_CMD 31
/** PSI PRO 40-49 */
#define PSI_PRO_CMD 40
/** REON 50-59*/
#define REON_CMD 50

#endif
