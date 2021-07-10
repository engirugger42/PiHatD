/* The actual daemon file. Based heavily on the 
 * tutorial by Devin Watson. Found here:
 * http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html
 *
 * The overall design was heavily influenced by
 * the PiBits ServoBlaster, written by Richard Hirst.
 * It's designed for use with the raw GPIO, instead of any add ons.
 * https://github.com/richardghirst/PiBits/tree/master/ServoBlaster
 *
 * This was written mostly to sharpen my C skills and document what I
 * learned in the hopes that somebody else might find this useful. As
 * such, this is going to be heavy on the comments. Enjoy the novel.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>

/* Pull in RPi libs and make definitions */
#include <wiringPiI2C.h>
#include "motorHat.h"

static volatile exit_flag = 0;
#define DEVFILE "/dev/motorDaemon"

/* Define a couple functions */

word init(){
    //Setup I2C
    word i2c = wiringPiI2CSetup(ADAFRUIT_MOTORHAT);

    //Setup PWM
    setAllPWM(i2c, 0, 0);
    wiringPiI2CWriteReg8(i2c, PWM_MODE2, PWM_OUTDRV);
    wiringPiI2CWriteReg8(i2c, PWM_MODE1, PWM_ALLCALL);
    delay(5);
    word mode1 = wiringPiI2CReadReg8(i2c, PWM_MODE1) & ~PWM_SLEEP;
    wiringPiI2CWriteReg8(i2c, PWM_MODE1, mode1);
    delay(5);

    //Set PWM frequency
    word prescale = (int)(25000000.0 / 4096.0 / PWM_FREQUENCY - 1.0);
    word oldmode = wiringPiI2CReadReg8(i2c, PWM_MODE1);
    word newmode = oldmode & 0x7F | 0x10;
    wiringPiI2CWriteReg8(i2c, PWM_MODE1, newmode);
    wiringPiI2CWriteReg8(i2c, PWM_PRESCALE, prescale);
    wiringPiI2CWriteReg8(i2c, PWM_MODE1, oldmode);
    delay(5);
    wiringPiI2CWriteReg8(i2c, PWM_MODE1, oldmode | 0x80);

    return i2c;
}

void executeCommand(word i2c, char* command) {
    /* Split string into tokens */
    int time = 0;
    word direction;
    char *c; 

    c = strtok(command, " ");
    while (c != NULL) {
        if(strcmp("killD", c) == 0) {
                syslog(LOG_INFO, "motord: Shutting down...");
                exit(EXIT_SUCCESS);
        } else if(strcmp("forward", c) == 0) {
            direction = MOTOR_FORWARD;
            syslog(LOG_INFO, "Direction: %s:%d",c, direction);
        } else if(strcmp("right", c) == 0) {
            direction = MOTOR_RIGHT;
            syslog(LOG_INFO, "Direction: %s:%d", c, direction);
        } else if(strcmp("left", c) == 0) {
            direction = MOTOR_LEFT;
            syslog(LOG_INFO, "Direction: %s:%d", c, direction);
        } else if(strcmp("backward", c) == 0) {
            direction = MOTOR_BACK;
            syslog(LOG_INFO, "Direction: %s:%d", c, direction);
        } else if(strcmp("pivotleft", c) == 0) {
            direction = MOTOR_PIVOT_LEFT;
            syslog(LOG_INFO, "Direction: %s:%d", c, direction);
        } else if(strcmp("pivotright", c) == 0) {
            direction = MOTOR_PIVOT_RIGHT;
            syslog(LOG_INFO, "Direction: %s:%d", c, direction);
        } else {
            char *endp;
            long l;
            l = strtol(c, &endp, 0);
            if (c != endp && *endp == '\0'){
                printf("It's an integer with value %ld\n", 1);
                time = l;
            } else {
                time = 0;
            }

            syslog(LOG_INFO, "Time: %d", time);
        }

        c = strtok(NULL, " ");
    }

    struct timespec tv;
    int sec = 0;
    int msec = 0;
    sec = time / 1000;
    msec = time % 1000;
    tv.tv_sec = sec;
    tv.tv_nsec = msec * 100000;
    syslog(LOG_INFO, "motord: Running the motor...");
    runMotor(i2c, 12, direction);
    int result = 0;
    do 
    {
        struct timespec ts_sleep = tv;
        result = nanosleep(&ts_sleep, &tv);
    } while (EINTR == result);

    syslog(LOG_INFO, "motord: Stopping the motor...");
    runMotor(i2c, 12, MOTOR_RELEASE);
}

static void hit_the_road(void)
{
    /* Initialize the motors */
    word i2c = init();
    word motor = 1;
    initMotor(i2c, motor);
    setSpeed(i2c, motor, 255);
    motor = 2;
    initMotor(i2c, motor);
    setSpeed(i2c, motor, 255);
    int ready = 0;
    int fd;
    FILE * fp;
    /* Somewhat arbitrary command size of 2 bytes
     * It's just enough space for the largest command
     * and a time of up to 9999 milliseconds.
     * Prasing input to the named pipe needs
     * more research, but this works for now.
     * (Because that temporary code NEVER gets left
     * in permanently...)
     */ 
    char line[15];

    syslog(LOG_INFO, "Starting the big loop...");
    /* The Big Loop */
    /* This will look oddly similar to ServoBlaster */
    if ((fd = open(DEVFILE, O_RDWR)) == -1){
        syslog(LOG_ERR, "servod: Failed to open %s: %m\n", DEVFILE); 
        exit(EXIT_FAILURE);
    } else {
        syslog(LOG_INFO, "motord: opened FIFO at %s for reading", DEVFILE);
    }

    for (;;) {
        /* Listen for motor command */
        /* While coding this up, discovered that while(true) and while(1)
         * can cause compiler warnings. So, while it's really a matter of
         * preference, I'll use this over while(true)
         */
        fd_set ifds;
        FD_ZERO(&ifds);
        FD_SET(fd, &ifds);

        if((ready == select(fd+1, &ifds, NULL, NULL, NULL)) != -1){
            syslog(LOG_INFO, "motord: FIFO ready for reading...");
            int i = 0;
            int bytesread = 0;
            memset(line, 0, sizeof line);
            while ((bytesread = read(fd, line, 15)) > 0) {
                line[bytesread] = '\0';
                syslog(LOG_INFO, "motord: Command string: %s", line);
                executeCommand(i2c, line);
            }
        }
    }
}

int main(int argc, char **argv) {
    int c;
    int digit_optind = 0;

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument,       0,  'h' },
            {0,         0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "h", long_options, &option_index);

        if (c == 'h') {
            printf("\nUsage: %s <options>\n\n"
                    "Simply run ./motorDaemon.d\n"
                    "If you're having trouble getting MotorHatD to run, make sure you have\n"
                    "wiringPi installed before running MotorHatD.\n"
                    "For instructions on getting wiringPi, visit"
                    "http://wiringpi.com/download-and-install/\n", argv[0]);
            exit(0);
        }

        /* Make the named pipe we'll be using */
        unlink(DEVFILE);
        if (mkfifo(DEVFILE, 0666) < 0) {
            syslog(LOG_ERR, "motorD: Failed to create %s: %m\n", DEVFILE);
            exit(EXIT_FAILURE);
        }
        if(chmod(DEVFILE, 0666) < 0) {
            syslog(LOG_ERR, "motorD: failed to set permissions on %s: %m\n", DEVFILE);
            exit(EXIT_FAILURE);
        }

        syslog(LOG_INFO, "Opened FIFO at %s and set permisions.", DEVFILE);

        /* Manually daemonizing for learning purposes
         * This is where it most closely reflects Hirst's
         * tutorial 
         */
        pid_t pid, sid;

        /* Fork off the parent process */
        pid = fork();

        if (pid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }

        /* If we got a good PID, then
         * we can exit the parent process */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask 
         * Setting the umask to 0 gives the
         * daemon access to files */
        umask(0);

        /* Start logging */
        openlog("motorDaemonLog", LOG_PID|LOG_CONS|LOG_NDELAY, LOG_USER);
        syslog(LOG_INFO, "Starting motor daemon...");

        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
            syslog(LOG_ERR, "Failed to set new SID for the child process.");
            syslog(LOG_ERR, "Error Message: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            syslog(LOG_ERR, "Failed to change working directory.");
            syslog(LOG_ERR, "Error message: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* Close the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        hit_the_road();
    }

    closelog();
    exit(EXIT_SUCCESS);
}
