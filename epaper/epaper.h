/**
 * epaper Library Header file for Raspberry Pi
 *
 * Copyright (c) July 2019, Paul van Haastrecht
 *
 * All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 * Initial version by paulvha version 15 July 2019
 *
 *********************************************************************
*/
#ifndef EPAPER_H
#define EPAPER_H

# include <stdarg.h>
# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>     // exit()
# include <signal.h>     // signal()
# include <time.h>       // for Time or date
# include <strings.h>    // strcasecmp()
# include <string.h>     // strcpy()
# include <stdint.h>
# include <stdbool.h>
# include <getopt.h>     // parse command line
# include <sys/stat.h>   // open call
# include <fcntl.h>      // open call

#include "./obj/GUI_Paint.h"
#include "./obj/GUI_BMPfile.h"
#include "./obj/ImageData.h"
#include "./obj/EPD_7in5b.h"

#define FONTLOCATION "./Fonts/" // directory where fonts are stored
#define FONTLENGTH 15           // maximum length name font
#define MAXTEXTLENGTH 200       // maximum length text as part of instructions
#define MAXINSTRUCTIONS 1000    // maximum length epaper instructions
#define MAXFILENAME 100         // maximum length file or pipename

// next to BLACK and WHITE also define COLOR
#define COLOR 4

struct image_prop {
    char  font[FONTLENGTH];
    UWORD Xstart;
    UWORD Ystart;
    UWORD front_color;
    UWORD back_color;
    UWORD Xstart_back;
    UWORD Ystart_back;
};

/**
 * Enhanced versions of the draw to support color display
 * The rest is the same as the original versions
 */
void ePaint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);

void ePaint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground,
                    UWORD *last_Xpoint, UWORD *last_Ypoint);

void ePaint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Background, UWORD Color_Foreground,
                   UWORD *last_Xpoint, UWORD *last_Ypoint);

void ePaint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel);

/*! to display in color  */
void p_printf (int level, char *format, ...);

// color display enable
#define D_RED     1
#define D_GREEN   2
#define D_YELLOW  3
#define D_BLUE    4
#define D_WHITE   5

#define REDSTR "\e[1;31m%s\e[00m"
#define GRNSTR "\e[1;92m%s\e[00m"
#define YLWSTR "\e[1;93m%s\e[00m"
#define BLUSTR "\e[1;34m%s\e[00m"

// disable color output
extern bool NoColor;

#endif // EPAPER
