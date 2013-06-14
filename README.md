CAN Throttle
============

An Arduino Due CAN throttle monitor
It requests the throttle angle of a Volvo S80 2008 and displays the response as
a percentage on an LCD screen.

WARNING: This works on a Volvo S80 2008 model. You must
first find out the correct frame to query the throttle
angle and also the correct ID's. If you just test this
with your car prior to adjusting the CAN speed and frame
data, you might get into trouble. I take no responsibilty
whatsoever if you damage your car. You must know what
you are doing!

A library for the Due's CAN bus is available here:
https://github.com/collin80/due_can

For a design of a dual CAN shield, check my blog
http://s80ev.blogspot.ch/2013/05/springtime.html
or http://arduino.cc/forum/index.php/topic,131096.30.html
