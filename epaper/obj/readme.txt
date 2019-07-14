The following files are in this directory :

Debug.h 
	Used by the original driver to display debug messages. 
 	As this could only be set during compile where the epaper has an option this is not used.

DEV_Config.c DEV_Config.h
	routines to interface with the BCM2835 library. A new debug routine has been added.

EPD_7in5b.c EPD_7in5b.h
	Contain the API calls to communicate with the display

GUI_Paint.c GUI_Paint.h
	Achieve drawing: draw points, lines, boxes, circles and their size, solid dotted line, 
	solid rectangle hollow rectangle, solid circle hollow circle.display characters: 
	Display a single character, string, number. time display: adaptive size display time 
	minutes and seconds

GUI_BMPfile.c GUI_BMPfile.h
	Will read and display a BMP file.

ImageData.c ImageData.h
	Part of the original package and does not look to be used.

main.c
	The original main program to create the original epd file