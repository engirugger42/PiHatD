# MotorHatD
Linux daemon for controlling motors on the Raspberry pi via the Adafruit motor control Pi Hat.
When compiling by hand, run with the -lwiringPi flag to pull in the wiringPi libraries, i.e.
gcc -lwiringPi ./motorDaemon.c motorHat.c

Another note, it's pretty rudimentary and accepts commands in 15 byte blocks. I'm none too good at this named pipe thing yet, so it's hard coded to read 15 bytes, tack on a '\0' to terminate the C string, and then goes on it's merry way. So, if you feed it 14 bytes or 16 bytes, it won't parse the command correctly. Maybe one of these days I'll figure out how to do this properly.
