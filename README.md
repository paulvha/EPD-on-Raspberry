# Eink/ EPD Waveshare 7.5 inch on Raspberry Pi
## ===========================================================

## Background
Time to better understand Epaper or E-ink paper. At least for a project.
Many of the E-readers use this display as it is stable, does not require power
after updating and is mild for your eyes.
So how does that work in practice and how does software and hardware work together.

I have downloaded the Waveshare code (version 2019-04-08), from
https://www.waveshare.com/wiki/7.5inch_e-Paper_HAT which relies on
the BCM2835 library to connect to the hardware.
I have used that BCM2835 library many times before as it is fast and stable.

The top level is demo software, just clears the screen and show some boxes etc.

The top level software that I have developed is making use of the
underlying libraries and takes instructions from a user to position
and display circles, lines etc.

<br>A detailed document with the experience and description is epaper.odt

## Prerequisites
The program depends on the BCM2835 Library

## Software installation

Make your self superuser : sudo bash
Install latest from BCM2835 from : http://www.airspayce.com/mikem/bcm2835/

1. wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.56.tar.gz
2. tar -zxf bcm2835-1.56.tar.gz     // 1.56 was version number at the time of writing
3. cd bcm2835-1.56
4. ./configure
5. sudo make check
6. sudo make install

cd epaper
To create the epaper executable : make
To clean  :   make clean

note: you might can missing braces around initializer for font12CN and font24CN.
Ignore those that is a known compiler error

## Program usage
The BCM2835 library needs to run as Root for the I2C communication to work.

sudo ./epaper -h will display help
A detailed document with the experience and description is epaper.odt

## Versioning
### version 1.0 / July 2019
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0
