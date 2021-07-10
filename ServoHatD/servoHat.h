/* Constants and methodswe'll need for the pi hat.
 * Methods and header definitions from a RPI forum thread,
 * specifically, the post by garagebrewer:
 * https://www.raspberrypi.org/forums/viewtopic.php?t=112415&p=770234
 */
#define word    unsigned short

#define ADAFRUIT_SERVOHAT       0x40

#define _MODE1       0x00
#define _MODE2       0x01
#define _SUBADR1     0x02
#define _SUBADR2     0x03
#define _SUBADR3     0x04
#define _PRESCALE    0xFE
#define _LED0_ON_L   0x06
#define _LED0_ON_H   0x07
#define _LED0_OFF_L  0x08
#define _LED0_OFF_H  0x09

#define PWM_FREQUENCY   100.0

#define _RESTART     0x80
#define _SLEEP       0x10
#define _ALLCALL     0x01
#define _INVRT       0x10
#define _OUTDRV      0x04

#define _ALL_LED_ON_L        0xFA
#define _ALL_LED_ON_H        0xFB
#define _ALL_LED_OFF_L       0xFC
#define _ALL_LED_OFF_H       0xFD

#define SERVO_PAN          0x00
#define SERVO_TILT         0x01

void setAllPWM(word i2c, word on, word off);
void setPWM(word i2c, word pin, word on, word off);
void setPWMFreq(word i2c, word freq);
