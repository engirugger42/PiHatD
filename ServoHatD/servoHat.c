/* Methods for setting up the i2c channel to the 
 * motor board and some methods for initializing 
 * running the motors. i2c setup uses wiringPi library.
 * Methods and header definitions from a RPI forum thread,
 * specifically, the post by garagebrewer:
 * https://www.raspberrypi.org/forums/viewtopic.php?t=112415&p=770234
 */
#include <wiringPiI2C.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "servoHat.h"

void setAllPWM(word i2c, word on, word off){
        wiringPiI2CWriteReg8(i2c, _ALL_LED_ON_L, on & 0xFF);
        wiringPiI2CWriteReg8(i2c, _ALL_LED_ON_H, on >> 8);
        wiringPiI2CWriteReg8(i2c, _ALL_LED_OFF_L, off & 0xFF);
        wiringPiI2CWriteReg8(i2c, _ALL_LED_OFF_H, off >> 8);
}
void setPWMFreq(word i2c, word freq){
    //Set PWM frequency
    word prescale = (int)(25000000.0 / 4096.0 / PWM_FREQUENCY - 1.0);
    word oldmode = wiringPiI2CReadReg8(i2c, _MODE1);
    word newmode = oldmode & 0x7F | 0x10;
    wiringPiI2CWriteReg8(i2c, _MODE1, newmode);
    wiringPiI2CWriteReg8(i2c, _PRESCALE, prescale);
    wiringPiI2CWriteReg8(i2c, _MODE1, oldmode);
    delay(5);
    wiringPiI2CWriteReg8(i2c, _MODE1, oldmode | 0x80);
}
void setPWM(word i2c, word pin, word on, word off){
        wiringPiI2CWriteReg8(i2c, _LED0_ON_L + 4 * pin, on & 0xFF);
        wiringPiI2CWriteReg8(i2c, _LED0_ON_H + 4 * pin, on >> 8);
        wiringPiI2CWriteReg8(i2c, _LED0_OFF_L + 4 * pin, off & 0xFF);
        wiringPiI2CWriteReg8(i2c, _LED0_OFF_H + 4 * pin, off >> 8);
}
