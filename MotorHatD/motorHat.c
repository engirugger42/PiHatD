/* Methods for setting up the i2c channel to the 
 * motor board and some methods for initializing 
 * running the motors. i2c setup uses wiringPi library.
 * Methods and header definitions from a RPI forum thread,
 * specifically, the post by garagebrewer:
 * https://www.raspberrypi.org/forums/viewtopic.php?t=112415&p=770234
 */
#include <wiringPiI2C.h>
#include <stdio.h>
#include "motorHat.h"

void setAllPWM(word i2c, word on, word off){
        wiringPiI2CWriteReg8(i2c, PWM_ALL_LED_ON_L, on & 0xFF);
        wiringPiI2CWriteReg8(i2c, PWM_ALL_LED_ON_H, on >> 8);
        wiringPiI2CWriteReg8(i2c, PWM_ALL_LED_OFF_L, off & 0xFF);
        wiringPiI2CWriteReg8(i2c, PWM_ALL_LED_OFF_H, off >> 8);
}

void setPWM(word i2c, word pin, word on, word off){
        wiringPiI2CWriteReg8(i2c, PWM_LED0_ON_L + 4 * pin, on & 0xFF);
        wiringPiI2CWriteReg8(i2c, PWM_LED0_ON_H + 4 * pin, on >> 8);
        wiringPiI2CWriteReg8(i2c, PWM_LED0_OFF_L + 4 * pin, off & 0xFF);
        wiringPiI2CWriteReg8(i2c, PWM_LED0_OFF_H + 4 * pin, off >> 8);
}

void setPin(word i2c, word pin, word value){
        if(pin < 0 || pin > 15){
                printf("PWM pin must be between 0 and 15 inclusive.  Received '%d'\n", pin);
                return;
        }

        switch(value){
                case 0:
                        setPWM(i2c, pin, 0, 4096);
                        break;
                case 1:
                        setPWM(i2c, pin, 4096, 0);
                        break;
                default:
                        printf("PWM pin value must be 0 or 1.  Received '%d'\n", pin);
                        return;
        }
}

void runMotor(word i2c, word motor, word command){
        word in1, in2, in3, in4;

        switch(motor){
                case 1:
                        in1 = PWM_M1_IN1;
                        in2 = PWM_M1_IN2;
                        break;
                case 2:
                        in1 = PWM_M2_IN1;
                        in2 = PWM_M2_IN2;
                        break;
                case 3:
                        in1 = PWM_M3_IN1;
                        in2 = PWM_M3_IN2;
                        break;
                case 4:
                        in1 = PWM_M4_IN1;
                        in2 = PWM_M4_IN2;
                        break;
                case 12:
                case 21:
                        in1 = PWM_M1_IN1;
                        in2 = PWM_M1_IN2;
                        in3 = PWM_M2_IN1;
                        in4 = PWM_M2_IN2;
                        break;
                default:
                        printf("Invalid motor number '%d'\n", motor);
                        return;
        }

        switch(command){
                case MOTOR_FORWARD:
                        setPin(i2c, in2, 0);
                        setPin(i2c, in1, 1);
                        setPin(i2c, in4, 0);
                        setPin(i2c, in3, 1);
                        break;
                case MOTOR_BACK:
                        setPin(i2c, in1, 0);
                        setPin(i2c, in2, 1);
                        setPin(i2c, in4, 0);
                        setPin(i2c, in3, 1);
                        break;
                case MOTOR_RIGHT:
                        setPin(i2c, in1, 1);
                        setPin(i2c, in2, 0);
                        setPin(i2c, in3, 0);
                        setPin(i2c, in4, 0);
                        break;
                case MOTOR_LEFT:
                        setPin(i2c, in1, 0);
                        setPin(i2c, in2, 0);
                        setPin(i2c, in3, 1);
                        setPin(i2c, in4, 0);
                        break;
                case MOTOR_PIVOT_RIGHT:
                        setPin(i2c, in1, 1);
                        setPin(i2c, in2, 0);
                        setPin(i2c, in3, 0);
                        setPin(i2c, in4, 1);
                        break;
                case MOTOR_PIVOT_LEFT:
                        setPin(i2c, in1, 0);
                        setPin(i2c, in2, 1);
                        setPin(i2c, in3, 1);
                        setPin(i2c, in4, 0);
                        break;
                case MOTOR_RELEASE:
                        setPin(i2c, in1, 0);
                        setPin(i2c, in2, 0);
                        setPin(i2c, in4, 0);
                        setPin(i2c, in3, 0);
                        break;
                default:
                        printf("Unsupported command '%d'\n", command);
                        return;
        }
}

void setSpeed(word i2c, word motor, word speed){
        if(speed < 0 || speed > 255){
                printf("Speed must be between 0 and 255 inclusive.  Received '%d'\n", speed);
                return;
        }

        word pwm;
        switch(motor){
                case 1:
                        pwm = PWM_M1_PWM;
                        break;
                case 2:
                        pwm = PWM_M2_PWM;
                        break;
                case 3:
                        pwm = PWM_M3_PWM;
                        break;
                case 4:
                        pwm = PWM_M4_PWM;
                        break;
                default:
                        printf("Unsupported motor '%s'\n", motor);
                        break;
        }
        setPWM(i2c, pwm, 0, speed * 16);
}

void initMotor(word i2c, word motor){
        runMotor(i2c, motor, MOTOR_RELEASE);
        setSpeed(i2c, motor, 150);
        runMotor(i2c, motor, MOTOR_FORWARD);
        runMotor(i2c, motor, MOTOR_RELEASE);
}
