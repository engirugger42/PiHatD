#!/usr/bin/python
f = open('/dev/servoDaemon', 'w')
string = "pan 650"
f.write(string.ljust(13))
string = "tilt 450"
f.write(string.ljust(13))
f.close()
