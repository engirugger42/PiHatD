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
#include "servoHat.h"

#define DEVFILE "/dev/servoDaemon"

/* Define a couple functions */

word init(){
    //Setup I2C
    word i2c = wiringPiI2CSetup(ADAFRUIT_SERVOHAT);

    //Setup PWM
    setAllPWM(i2c, 0, 0);
    wiringPiI2CWriteReg8(i2c, _MODE2, _OUTDRV);
    wiringPiI2CWriteReg8(i2c, _MODE1, _ALLCALL);
    delay(5);
    word mode1 = wiringPiI2CReadReg8(i2c, _MODE1);
    mode1 = mode1 & ~_SLEEP;
    wiringPiI2CWriteReg8(i2c, _MODE1, mode1);
    delay(5);

    //Set PWM frequency
    word prescale = (int)(25000000.0 / 4096.0 / PWM_FREQUENCY - 1.0);
    word oldmode = wiringPiI2CReadReg8(i2c, _MODE1);
    word newmode = oldmode & 0x7F | 0x10;
    wiringPiI2CWriteReg8(i2c, _MODE1, newmode);
    wiringPiI2CWriteReg8(i2c, _PRESCALE, prescale);
    wiringPiI2CWriteReg8(i2c, _MODE1, oldmode);
    delay(5);
    wiringPiI2CWriteReg8(i2c, _MODE1, oldmode | 0x80);

    return i2c;
}

void setServoPulse(word i2c, int channel, int pulse) {
    int pulseLength = (int)(1000000 / 60 / 4096);
    pulse *= 100;
    pulse /+ pulseLength;
    setPWM(i2c, 0, 0, pulse);
}

void executeCommand(word i2c, char* command) {
    /* Split string into tokens */
    int pulse = 0;
    word direction;
    char *c; 

    c = strtok(command, " ");
    while (c != NULL) {
        if(strcmp("tilt", c) == 0) {
            direction = SERVO_TILT;
            syslog(LOG_INFO, "Direction: %s:%d",c, direction);
        } else if(strcmp("pan", c)== 0) {
            direction = SERVO_PAN;
            syslog(LOG_INFO, "Direction: %s:%d", c, direction);
        } else {
            char *endp;
            long l;
            l = strtol(c, &endp, 0);
            if (c != endp && *endp == '\0'){
                printf("It's an integer with value %ld\n", 1);
                pulse = l;
            } else {
                pulse = 550;
            }

            syslog(LOG_INFO, "Position: %d", pulse);
        }

        c = strtok(NULL, " ");
    }

    setServoPulse(i2c, direction, pulse);
}

static void hit_the_road(void)
{
    /* Initialize the motors */
    word i2c = init(100);
    //setPWMFreq(i2c, 100);
    int ready = 0;
    int fd;
    FILE * fp;
    //char * line = NULL;
    char line[14];

    syslog(LOG_INFO, "Starting the big loop...");
    /* The Big Loop */
    /* This will look oddly similar to ServoBlaster */
    if ((fd = open(DEVFILE, O_RDWR)) == -1){
        syslog(LOG_ERR, "servod: Failed to open %s: %m\n", DEVFILE); 
        exit(EXIT_FAILURE);
    } else {
        syslog(LOG_INFO, "servod: opened FIFO at %s for reading", DEVFILE);
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
            syslog(LOG_INFO, "servod: FIFO ready for reading...");
            int i = 0;
            int bytesread = 0;
            memset(line, 0, sizeof line);
            while ((bytesread = read(fd, line, 13)) > 0) {
                line[bytesread] = '\0';
                syslog(LOG_INFO, "servod: Command string: %s", line);
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
            syslog(LOG_ERR, "servoD: Failed to create %s: %m\n", DEVFILE);
            exit(EXIT_FAILURE);
        }
        if(chmod(DEVFILE, 0666) < 0) {
            syslog(LOG_ERR, "servoD: failed to set permissions on %s: %m\n", DEVFILE);
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
        openlog("servoDaemonLog", LOG_PID|LOG_CONS|LOG_NDELAY, LOG_USER);
        syslog(LOG_INFO, "Starting servo daemon...");

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
