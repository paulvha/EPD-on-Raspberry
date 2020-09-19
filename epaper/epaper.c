/**
 * program to display information on Epaper
 * 
 * This is using the original Waveshare library which has been modified 
 * on a couple of places to extend the support of color or to support this 
 * toplayer program .
 * 
 * This program will display data on an 7.5inch Waveshare Epaper based on 
 * on formatted instructions. These instructions can be provided on the command-line,
 * or read from a file (like car-instruction) or from a remote program with named 
 * pipes, A sample program remotepr.c has been provided.
 * 
 * Version 1.0.1 September 2020 / paulvha
 * - fixed issues to compile on Pi-OS (Buster) in obj/DEV_config.h
 * - supress warning messages around missing braces in font file (is GCC bug) in Makefile
 * 
 * *****************************************************************
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/
  
# include "epaper.h"

# define VERSION "1.0,1 September 2020"

/* used as part of p_printf() */
bool NoColor=false;

/*image information */
struct image_prop IM_prop;
UBYTE   *BlackImage = 0x0;
UBYTE   *RedImage = 0x0;

/* indicate status of HW */
bool BCM_init = false;                  // BCM was initialised
bool EPD_DisplayOn = false;             // display is turned on

/* hold the provided instructions */
char Instruction[MAXINSTRUCTIONS];

/* hold pipe info */
char name_pipe_w[MAXFILENAME] = "./EPD_from";   // can be overruled from command line
char name_pipe_r[MAXFILENAME] = "./EPD_to";     // can be overruled from command line
int p_fd_r = -1;                        // pipe handles
int p_fd_w = -1;
#define BUFSIZE 512                      // internal buffer size

/*********************************************************************
 * @brief Display in color
 * @param format : Message to display and optional arguments
 *                 same as printf
 * @param level :  1 = RED, 2 = GREEN, 3 = YELLOW 4 = BLUE 5 = WHITE
 * 
 * if NoColor was set, output is always WHITE.
 *********************************************************************/
void p_printf(int level, char *format, ...) {
    
    char    *col;
    int     coll=level;
    va_list arg;
    
    //allocate memory
    col = (char *) malloc(strlen(format) + 20);
    
    if (NoColor) coll = D_WHITE;
                
    switch(coll)  {
        case D_RED:
            sprintf(col,REDSTR, format);
            break;
        case D_GREEN:
            sprintf(col,GRNSTR, format);
            break;      
        case D_YELLOW:
            sprintf(col,YLWSTR, format);
            break;      
        case D_BLUE:
            sprintf(col,BLUSTR, format);
            break;
        default:
            sprintf(col,"%s",format);
    }

    va_start (arg, format);
    vfprintf (stdout, col, arg);
    va_end (arg);

    fflush(stdout);

    // release memory
    free(col);
}

/**
 * System Exit
 * 
 * @param ret : return code on exit
 */
void close_out(int ret)
{
    if (EPD_DisplayOn) {
        printf("\r\nClosing down Epaper:Goto Sleep mode\r\n");
        EPD_Sleep();
    }
    
    if (BCM_init)    DEV_ModuleExit();

    // if open pipes
    if (p_fd_r  > 0)  close(p_fd_r);
    if (p_fd_w  > 0)  close(p_fd_w);

    // if memory allocated
    if (BlackImage != 0x0) free(BlackImage);
    if (RedImage != 0x0) free(RedImage);
    
    exit(ret);
}

/**
 * @brief : set initial variables
 */
void init_variables()
{
    IM_prop.Xstart = 0;                     // current X position
    IM_prop.Ystart = 0;                     // current Y position
    IM_prop.back_color = WHITE;             // default background color
    IM_prop.front_color = BLACK;            // default foreground color
    strcpy (IM_prop.font, "font12");        // default font
    IM_prop.Xstart_back = 0xffff;           // backup place for Xstart
    IM_prop.Ystart_back = 0xffff;           // backup place for Ystart
}

/**
 * @brief : initialise hardware
 */
void hw_init()
{
    // initialise BCM2835
    if (! BCM_init) {
        DEV_ModuleInit();
        BCM_init = true;
    }

    // initialise the epaper
    EPD_Init();
}

/**
 * @brief : fill images memory with white
 */
void reset_image()
{
    // Select Image
    Paint_SelectImage(RedImage);
    Paint_Clear(WHITE);
     
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
}

/**
 *  Create a 2 image caches
 *  BLACKIMAGE (set black and white)
 *  REDImage for color
 * 
 *  and fill them with white
 */
void image_init()
{
    UWORD Imagesize = ((EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1)) * EPD_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        close_out(EXIT_FAILURE);
    }
    
    if((RedImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for red memory...\r\n");
        close_out(EXIT_FAILURE);
    }
    
    Debug("NewImage:BlackImage and RedImage\r\n");
    Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
    Paint_NewImage(RedImage, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);

    reset_image();
}

/**
 * @brief signal handler
 */
void Handler(int signo)
{
    //System Exit
    printf("\r\nEpaper Handler:Interrupt received\r\n");
    close_out(EXIT_SUCCESS);
}

/**
 * @brief display usage
 */
void usage()
{
    printf("epaper [options]  (version %s) \n\n"
    
    "-F filename    read \"Formatted instructions\" from file\n"
    "-P             read \"Formatted instructions\" from pipe\n"
    "   -r pipename read from named pipe (default %s)\n"
    "   -w pipename write to named pipe  (default %s)\n"
    "-T \"Formatted instructions\"  to display on epaper\n"
    "-D             show debug information\n\n"
    "Formatted instructions :\n"
    " <         start of instructions (always first character)\n\n"
    "       ---------  format options ----------\n"
    " d=#,      set foreground color : # = B(black), W(white) or C(color)\n"
    " b=#,      set background color : # = B(black), W(white) or C(color)\n"
    " B=#,      set Border color     : # = B(black), W(white) or C(color)\n"
    " f='#',    set font to use (# = name font as in Font-directory)\n"
    " p=x:y,    set start position to display\n"
    " r=#,      rotate display 0, 90, 270,360\n"
    " m=#,      mirror image\n"
    "           # = N MIRROR_NONE\n"
    "           # = H MIRROR_HORIZONTAL\n"
    "           # = V MIRROR_VERTICAL\n"
    "           # = O MIRROR_ORIGIN\n\n"
    "       ---------  display options ----------\n"
    " T=#,      display time (# = s (include seconds), n = (not include)\n"
    " D=#,      display date (# = n (as numbers) or w (as words)\n"
    " P=#,      draw a Point (# = pixel size: 1 - 8)\n"
    " c=r:s,    draw empty circle r = radius, s = pixel size: 1 - 8\n"
    " C=r:s,    draw filled Circle r = radius, s = pixel size: 1 - 8\n"
    " q=X:Y:s,  draw empty rectangle X end, Y end, s = pixel size: 1 - 8\n"
    " Q=X:Y:s,  draw filled rectangle X end, Y end, s = pixel size: 1 - 8\n"
    " l=X:Y:Z:s,        drawline X end, Y end, Z = style, s = size: 1 - 8\n"
    " i='filename',     load image (BMP) on current position\n"
    " t='text',         display text\n"
    " n='Number',       display number\n\n"

    " !=#,      special instructions\n"
    "           # = C   perform a Clear screen and image memory\n"
    "           # = c   perform a clear image memory\n"
    "           # = d   display the current X / Y positions\n"
    "           # = s   save the current X / Y positions\n"
    "           # = r   restore the saved X / Y positions\n"
    "           # = p   set screen to deepsleep\n"
    "           # = i   initialise screen\n\n"
    " >    end of instructions (ALWAYS)\n", VERSION,name_pipe_r,name_pipe_w);
}

/**
 *  @brief : set and check for font
 * 
 *  @param p : points to filename like 'font12' 
 * 
 *  @return :
 *  OK = pointer after last  quote
 *  Error = NULL
 */
char *set_font(char *p)
{
    char buf[25];
    int  i = 0;
    FILE *fp;
    
    if (*p++ != '\'') {
        p_printf(D_RED,"Error during setting font. Expected ' but got %c\n", *--p);
        return(NULL);
    }

    // get font filename (terminated wiht ' or >)
    while(*p != '\'' && *p != '>' && i < FONTLENGTH)
        IM_prop.font[i++] = *p++;
    
    IM_prop.font[i] = 0x0;
    
    // add font location
    sprintf(buf,"%s%s.c", FONTLOCATION, IM_prop.font);
       
    // check that font file exists
    if ( ! (fp=fopen(buf,"r")) ) {
        
        // in case font (lower case) instead of Font (upper case)
        if (IM_prop.font[0] > 'Z'){
            
            IM_prop.font[0] -= 0x20;
            
            // add font location
            sprintf(buf,"%s%s.c", FONTLOCATION, IM_prop.font);
            
            if (!(fp = fopen(buf,"r")) ) { 
                printf("can not find font %s", buf);
                return(NULL);
            }
        }
    }
    
    Debug("Set font to: %s\n",IM_prop.font);
    
    fclose(fp);
    
    return(++p);
}

/**
 * @brief display a buffer text and add the selected font
 * 
 * @param buf : text to display
 * @param number : if true text is actually a number
 *
 * @return
 * o = OK
 * -1 = error
 */ 
int display_add_font(char * buf, bool number)  
{  
    sFONT * tfont = 0 ;
    cFONT * tcfont = 0;
    
    // select font depending of IM_prop.font
    // if you add more fonts, store in the FONTLOCATION
    // update Font.h
    // update code below
    if (strcasecmp(IM_prop.font,"Font8") == 0) tfont = &Font8;
    else if (strcasecmp(IM_prop.font,"Font12") == 0) tfont = &Font12;
    else if (strcasecmp(IM_prop.font,"Font12CN") == 0) tcfont = &Font12CN;
    else if (strcasecmp(IM_prop.font,"Font16") == 0) tfont = &Font16;
    else if (strcasecmp(IM_prop.font,"Font20") == 0) tfont = &Font20;
    else if (strcasecmp(IM_prop.font,"Font24") == 0) tfont = &Font24;
    else if (strcasecmp(IM_prop.font,"Font24CN") == 0) tcfont = &Font24CN;
    else {
        printf("Could not determine %s\n", IM_prop.font);
        return(-1);
    }
    
    if (tfont != 0){
        
        if (number){
            uint32_t n;
            sscanf(buf,"%d", &n);
            ePaint_DrawNum (IM_prop.Xstart, IM_prop.Ystart, n, 
            tfont,  IM_prop.back_color, IM_prop.front_color, &IM_prop.Xstart, &IM_prop.Ystart);
        }
        else 
            ePaint_DrawString_EN (IM_prop.Xstart, IM_prop.Ystart, buf, 
            tfont, IM_prop.back_color, IM_prop.front_color,&IM_prop.Xstart, &IM_prop.Ystart);
    }
    else
        Paint_DrawString_CN(IM_prop.Xstart, IM_prop.Ystart, buf, 
        tcfont, IM_prop.back_color, IM_prop.front_color);
    
    return(0);
}        

/**
 * @brief : displays text or a number on the screen
 * 
 * @param p : pointer to 'text' or 'number'
 * @param number : true display number else text
 * 
 * @return
 * pointer after the closing '
 */
char *display_txt(char *p, bool number)
{
    char buf[MAXTEXTLENGTH];
    int  i = 0;
    bool escape = false;

    // check for ' start of text
    if (*p++ != '\'') {
        printf("error during display text, expect '. got %c \n", *--p);
        exit(EXIT_FAILURE);
    } 
    
    // get text between quotes
    while(i < MAXTEXTLENGTH) {
    
        /* escape character do not store */
        if (*p == '\\' && ! escape) {
            escape = true;
            p++;
            continue;
        }
        
        /* if end character, but NOT escape we are done */
        if (*p == '\'' && ! escape) break;
        
        /* store received character */
        buf[i++] = *p++;
        escape = false;
    }
    
    buf[i] = 0x0;
   
    // display text or number with selected font
    if (display_add_font(buf,number) == -1) return(NULL);
    
    return(++p);
}

/**
 * @brief set position to display next item
 * @param p : pointer to xx:yy
 * 
 * @return :
 * pointer after last y
 */ 

char *set_position(char *p)
{
    char buf[5];
    int i = 0;
    
    while (*p != ':'){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"X position to long\n");
            return(NULL);
         }
    }
    
    buf[i] = 0x0;
    IM_prop.Xstart = (UWORD) strtod(buf, NULL);
    p++; // skip :

    i = 0;
    while (*p != '>' && *p != ','){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"Y postion too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    IM_prop.Ystart= (UWORD) strtod(buf, NULL);

    return(p);
}

/** 
 * @brief Color
 * 
 * @param col : color to set (background/foreground)
 * @param p :   pointer to color (BWC)
 * 
 * @return
 * Pointer AFTER color
 */
char * set_color(UWORD *col, char *p)
{
    if (*p == 'B' || *p == 'b' )     *col = BLACK;
    else if (*p == 'W'|| *p == 'w' ) *col = WHITE;
    else if (*p == 'C' || *p == 'c') *col = COLOR;
    else {
        printf("Invalid color %c\n", *p);
        return(NULL);
    }
    
    return(++p);
} 

/**
 * @brief set display flip 
 * @param p : pointer instruction
 * N = MIRROR_NONE 
 * H = MIRROR_HORIZONTAL
 * V = MIRROR_VERTICAL
 * O = MIRROR_ORIGIN
 * 
 * @return : pointer after instruction
 */
char *set_mirror(char *p)
{
    UBYTE mirror;
    
    if (*p == 'N'|| *p == 'n')      mirror = MIRROR_NONE;
    else if (*p == 'H'|| *p == 'h') mirror = MIRROR_HORIZONTAL;
    else if (*p == 'V'|| *p == 'v') mirror = MIRROR_VERTICAL;
    else if (*p == 'O'|| *p == 'o') mirror = MIRROR_ORIGIN;
    else {
        printf("invalid mirror %c\n", *p);
        return(NULL);
    }
 
    Paint_SetMirroring(mirror);
    
    return(++p);
}  

/**
 * @brief : set display rotation
 * @param p
 * 0    ROTATE_0 
 * 90   ROTATE_90
 * 180  ROTATE_180
 * 270  ROTATE_270
 * 
 * @return : pointer after rotation instruction
 */
char *set_rotation(char *p)
{
    char buf[5];
    int i = 0;
    UWORD Rotate;
    
    while (*p != 0x0 && *p != ',' && *p != '>' ){
         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"Rotation too long %s\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    
    i = (int) strtod(buf, NULL);
    
    if(i == 0)        Rotate = ROTATE_0;
    else if(i == 90)  Rotate = ROTATE_90;
    else if(i == 180) Rotate = ROTATE_180;
    else if(i == 270) Rotate = ROTATE_270;

    Paint_SetRotate(Rotate);

    return(p);
}

/**
 * @brief : perform special instruction
 * @param p:
 * 
 * C/c clear screen
 * D/d display current X and Y positions
 * S/s save current X and Y positions
 * R/r restore saved X and Y positions
 * P/p pauze screen power (deelsleep)
 * I/i initialize screen (wake from deepsleep)
 * 
 * @return : pointer after instruction
 */
char * special_instruction(char *p)
{
    char c = *p;

    switch (c) {
        
        case 'C':   // perform complete clear
            Debug("clear...\r\n");
            EPD_DisplayOn = true;
            EPD_Clear();
            DEV_Delay_ms(500); 
            // fall through
        case 'c':   
            reset_image();
            init_variables();
            break;
        
        case 'D':    
        case 'd':
            printf("Current X-position %d, Y position %d\n",IM_prop.Xstart, IM_prop.Ystart);
            break;
        
        case 'S':    
        case 's': // save current x/y position
            IM_prop.Xstart_back = IM_prop.Xstart;
            IM_prop.Ystart_back = IM_prop.Ystart;
            break;
        
        case 'R':    
        case 'r': // restore save X/Y position
            if (IM_prop.Xstart_back != 0xffff) {
                IM_prop.Xstart = IM_prop.Xstart_back;
                IM_prop.Ystart = IM_prop.Ystart_back;
            }
            else
                printf("X/Y positions not save before");
            break;
   
        case 'P':
        case 'p': // set screeen in deepsleep
            if (EPD_DisplayOn) EPD_Sleep();
            EPD_DisplayOn = false;
            break;
 
        case 'I':
        case 'i': // start screeen from deepsleep
            EPD_Init();
            break;
            
    }
    return(++p);
}

/**
 * @brief display a line on th epaper
 * 
 * l=X:Y;Z:s   
 * X position end of line, 
 * Y position end of line 
 * Z : 0 solid 1 = dotted
 * s : 1 to 8
 * 
 * @return : pointer after instruction
 */
char *display_line(char *p)
{
    UWORD Xend, Yend;
    LINE_STYLE Line_Style; 
    DOT_PIXEL Dot_Pixel = DOT_PIXEL_DFT;
    char buf[5];
    int i = 0;
    
    while (*p != ':'){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"line Xend position to long\n");
            return(NULL);
         }
    }
    
    buf[i] = 0x0;
    Xend = (UWORD)strtod(buf, NULL);
    p++; // skip :

    i = 0;
    while (*p != ':') {

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"line Yend postion too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    Yend= (UWORD)strtod(buf, NULL);

    p++; // skip :
    i = 0;
    
    while (*p != ':') {

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"line style too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    
    if (strtod(buf, NULL) > 0) Line_Style = LINE_STYLE_DOTTED;
    else Line_Style = LINE_STYLE_SOLID;
    
    // get pixel size
    p++; // skip :
    i = 0;
    while (*p != '>' && *p != ','){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"line pixel too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    
    i = (int) strtod(buf, NULL);
    
    switch (i) {
        case 1 : Dot_Pixel = DOT_PIXEL_1X1; break;
        case 2 : Dot_Pixel = DOT_PIXEL_2X2; break;
        case 3 : Dot_Pixel = DOT_PIXEL_3X3; break;
        case 4 : Dot_Pixel = DOT_PIXEL_4X4; break;
        case 5 : Dot_Pixel = DOT_PIXEL_5X5; break;
        case 6 : Dot_Pixel = DOT_PIXEL_6X6; break;
        case 7 : Dot_Pixel = DOT_PIXEL_7X7; break;
        case 8 : Dot_Pixel = DOT_PIXEL_8X8; break;
        default :
                p_printf(RED,"In valid Pixel %d\n", i);
                return(NULL);
                break;
    }
    
    Paint_SelectImage(BlackImage);
    ePaint_DrawLine(IM_prop.Xstart, IM_prop.Ystart, Xend, Yend, IM_prop.front_color, Line_Style, Dot_Pixel);

    // set the new X and Y positions.
    IM_prop.Xstart = Xend + 1;
    IM_prop.Ystart = Yend;
    
    return(++p);
}

/**
 * @brief Display a point
 * P=s    draw a point s = size
 */
char *display_point(char *p)
{
    DOT_PIXEL Dot_Pixel = DOT_PIXEL_DFT;
    char buf[5];
    int i = 0;
    
    i = 0;
    
    while (*p != '>' && *p != ','){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"Point pixel too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    
    i = (int) strtod(buf, NULL);
    
    switch (i) {
        case 1 : Dot_Pixel = DOT_PIXEL_1X1; break;
        case 2 : Dot_Pixel = DOT_PIXEL_2X2; break;
        case 3 : Dot_Pixel = DOT_PIXEL_3X3; break;
        case 4 : Dot_Pixel = DOT_PIXEL_4X4; break;
        case 5 : Dot_Pixel = DOT_PIXEL_5X5; break;
        case 6 : Dot_Pixel = DOT_PIXEL_6X6; break;
        case 7 : Dot_Pixel = DOT_PIXEL_7X7; break;
        case 8 : Dot_Pixel = DOT_PIXEL_8X8; break;
        default :
                p_printf(RED,"In valid point size %d\n", i);
                return(NULL);
                break;
    }
    
    // support colors
    if (IM_prop.front_color == COLOR)  Paint_SelectImage(RedImage);
        
    Paint_DrawPoint(IM_prop.Xstart, IM_prop.Ystart, BLACK, Dot_Pixel, DOT_STYLE_DFT);
    
    Paint_SelectImage(BlackImage);
    
    // set next start position after POINT
    IM_prop.Xstart += (UWORD) i;
    
    return(++p);
}

/** 
 * @brief display circle
 * 
 * c or C = R:s
 * R = radius
 * s = size of pixel
 */
char *display_circle(char *p, bool filled)
{
    char buf[7];
    int i = 0;
    UWORD radius;
    DOT_PIXEL Dot_Pixel = DOT_PIXEL_DFT;
    
    // get radius
    while (*p != ':') {

         buf[i++] = *p++;

         if (i > sizeof(buf) -1){
            p_printf(RED,"Circle Radius too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    radius = (UWORD)strtod(buf, NULL);
    
    // get pixel size
    p++; // skip :
    i = 0;
    while (*p != '>' && *p != ','){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"circle pixel too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    
    i = (int) strtod(buf, NULL);
    
    switch (i) {
        case 1 : Dot_Pixel = DOT_PIXEL_1X1; break;
        case 2 : Dot_Pixel = DOT_PIXEL_2X2; break;
        case 3 : Dot_Pixel = DOT_PIXEL_3X3; break;
        case 4 : Dot_Pixel = DOT_PIXEL_4X4; break;
        case 5 : Dot_Pixel = DOT_PIXEL_5X5; break;
        case 6 : Dot_Pixel = DOT_PIXEL_6X6; break;
        case 7 : Dot_Pixel = DOT_PIXEL_7X7; break;
        case 8 : Dot_Pixel = DOT_PIXEL_8X8; break;
        default :
            p_printf(RED,"In valid Pixel %d\n", i);
            return(NULL);
            break;
    }
    
    // support colors
    if (IM_prop.front_color == COLOR)  Paint_SelectImage(RedImage);
    else Paint_SelectImage(BlackImage);
    
    if (filled)
        Paint_DrawCircle(IM_prop.Xstart, IM_prop.Ystart, radius, BLACK, DRAW_FILL_FULL, Dot_Pixel);
    else
        Paint_DrawCircle(IM_prop.Xstart, IM_prop.Ystart, radius, BLACK, DRAW_FILL_EMPTY, Dot_Pixel);

    Paint_SelectImage(BlackImage);
    
    return(++p);    
}

/** 
 * @brief display rectangle
 * 
 *  q or Q=X:Y:s draw filled rectangle X end, Y end, s = pixel size
 */
char * display_rectangle(char *p,bool filled)
{
    char buf[7];
    int i = 0;
    UWORD Xend, Yend;
    DOT_PIXEL Dot_Pixel = DOT_PIXEL_DFT;
    
    // get Xend
    while (*p != ':'){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"rectangle Xend position to long\n");
            return(NULL);
         }
    }
    
    buf[i] = 0x0;
    Xend = (UWORD)strtod(buf, NULL);
    p++; // skip :

    i = 0;
    while (*p != ':') {

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"rectangle Yend postion too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    Yend= (UWORD)strtod(buf, NULL);

    // get pixel size
    p++; // skip :
    i = 0;
    while (*p != '>' && *p != ','){

         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"rectangle pixel too long\n");
            return(NULL);
         }
    }

    buf[i] = 0x0;
    
    i = (int) strtod(buf, NULL);
    
    switch (i) {
        case 1 : Dot_Pixel = DOT_PIXEL_1X1; break;
        case 2 : Dot_Pixel = DOT_PIXEL_2X2; break;
        case 3 : Dot_Pixel = DOT_PIXEL_3X3; break;
        case 4 : Dot_Pixel = DOT_PIXEL_4X4; break;
        case 5 : Dot_Pixel = DOT_PIXEL_5X5; break;
        case 6 : Dot_Pixel = DOT_PIXEL_6X6; break;
        case 7 : Dot_Pixel = DOT_PIXEL_7X7; break;
        case 8 : Dot_Pixel = DOT_PIXEL_8X8; break;
        default :
            p_printf(RED,"In valid Pixel %d\n", i);
            return(NULL);
            break;
    }
    
    // support colors
    if (IM_prop.front_color == COLOR)  Paint_SelectImage(RedImage);
    else Paint_SelectImage(BlackImage);
    
    if (filled)
        Paint_DrawRectangle(IM_prop.Xstart, IM_prop.Ystart, Xend, Yend, BLACK, DRAW_FILL_FULL, Dot_Pixel);
     else
        Paint_DrawRectangle(IM_prop.Xstart, IM_prop.Ystart, Xend, Yend, BLACK, DRAW_FILL_EMPTY, Dot_Pixel);

    Paint_SelectImage(BlackImage);
    
    return(++p);    
}

/**
 *  @brief : display time or date info
 *  @param p : 
 *      n numeric date
 *      s add seconds to timestamp
 *  @param date : 
 *      true :add date to timestamp
 */

char *display_time_day(char *p, bool date)
{
    char buf[MAXTEXTLENGTH];
    
    time_t ltime;
    struct tm *tm ;
    
    ltime = time(NULL);
    tm = localtime(&ltime);
    
    static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // if date is requested
    if (date){
        
        if (*p == 'n' || *p == 'N' )     // display as number mm:dd:yy
            sprintf(buf, " %d-%d-%d ", tm->tm_mday, tm->tm_mon +1, 1900 + tm->tm_year);
        else
            sprintf(buf, "%.3s %3d %.3s %d ",wday_name[tm->tm_wday], tm->tm_mday,
                mon_name[tm->tm_mon], 1900 + tm->tm_year);
    }
    else {
        if (*p == 's' || *p == 'S')     // display as number HH:MM:SS
            sprintf(buf, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
        else                            // display as number HH:MM
            sprintf(buf, "%.2d:%.2d", tm->tm_hour, tm->tm_min);
    }
    
    // display (not as number)
    if (display_add_font(buf,false) == -1) return(NULL);
    
    return(++p);
}

/**
 * during EPD_init() EPD_SendCommand(VCOM_AND_DATA_INTERVAL_SETTING); 
 * the border is set as white

 */
char * set_border_color(char *p)
{
    if (EPD_Set_Border(*p))
    {
        printf("Invalid color %c\n", *p);
        return(NULL);
    }
    
    return(++p);
} 

/**
 *  @brief : Display BMP file
 * 
 *  @return :
 *  OK = pointer after last  quote
 *  Error = NULL
 */
char * display_BMP(char *p)
{
    char buf[MAXFILENAME];
    int i = 0;
 
    // check for ' start of image name
    if (*p++ != '\'') {
        printf("error during obtaining BMP file, expect '. got %c \n", *--p);
        return(NULL);
    }     

    // extract BMP filename
    while (*p != '\'' && *p != '>' && *p != ',' ){
         buf[i++] = *p++;

         if (i > sizeof(buf)){
            p_printf(RED,"BMP filename to long\n");
            return(NULL);
         }
    }
    
    buf[i] = 0x0;
    
    Debug("BMP-file to display: %s\n",buf);
    
    // 
    if (GUI_ReadBmp(buf, IM_prop.Xstart, IM_prop.Ystart) == 1) {
        p_printf(RED,"Could not handle BMP filename %s\n", buf);
        return(NULL);
    }

    return(++p);
}

/**
 * @brief parse formatted string
 * 
 * <    start of instructions
 *  B=#     set border color (BWC)
 * 
 *  f=#     set font to use
 *  p=x:y   set start position to display
 * 
 *  d=#     set foreground color (BWC)
 *  b=#     set background color (BWC)
 * 
 *  i=''    load image on current position
 *  t=''    display text
 *  n=''    display number
 * 
 *  l=X:Y;Z:s   drawline X end, Y end, Z = style, s = size
 * 
 *  m=#     mirror image (N,O,H,V)
 * 
 *  P=s     draw a point s = size
 * 
 *  r=#     rotate 0,90,270, 360
 * 
 *  c=r:s   draw empty circle R = radius pixelsize s
 *  C=r:s   draw filled circle R = radius pixelsize s
 * 
 *  q=X:Y:s draw empty rectangle X end, Y end, s = pixel size
 *  Q=X:Y:s draw filled rectangle X end, Y end, s = pixel size
 * 
 *  T=n or s display time (s means include seconds, n = not include)
 *  D=n or w display date as numbers(n) or words(w)
 * 
 *  !=      special instructions
 *      c   perform a clear screen
 *      d   display the current X / Y positions
 *      s   save the current X / Y positions
 *      r   restore the saved X / Y positions
 *      more here..............
 * 
 * >    end of instructions
 * 
 * @return
 * -2 : syntax error
 * -1 : error during execution
 * 
 *  0 : all good
 *  else offset to *P where end was found
 */
int parse_string_instruction()
{
   char c;
   char *s, *p = Instruction;
   bool turn_display_on = false;
   
    // check for start of instruction
    if (*p++ != '<') {
        p_printf(D_RED," missing start of instruction : '<' \n");
        return(-2);
    }
    
    while (*p != '>' && *p != 0x0)
    {
        c = *p++;
   
        // if space, CR, NL  or comma skip rest
        if (c == 0x20 || c== ',' || c == 0x0d || c== 0x0a) continue;
        
        if (*p++ != '=') {
            p--;
            p_printf(D_RED, "Parseline : sequence error expected '=' got %c, 0x%x\n", *p, *p);
            printf("Parsed sofar :");
            s=Instruction;
            while ( p != s) printf("%c", *s++);
            printf("\n");
            return(-2);
        }
        
        switch(c) {

            case 'f':        // set font
                    if ((p = set_font(p)) == NULL) return(-1);
                    break;
            
            case 'p':       // set position
                    if ((p = set_position(p)) == NULL) return(-1);
                    break;
                    
            case 'd':       // set display / foreground color
                    if ((p = set_color(&IM_prop.front_color,p)) == NULL) return(-1);
                    break;

            case 'b':       // set background color
                    if ((p = set_color(&IM_prop.back_color,p)) == NULL) return(-1);
                    break;

            case 'B':       // set border color
                    if ((p = set_border_color(p)) == NULL) return(-1);
                    turn_display_on = true;  
                    break;
                    
            case 'm':       // set image mirror
                    if ((p = set_mirror(p)) == NULL) return(-1);
                    break;
                    
            case 'r':       // set rotation
                    if ((p = set_rotation(p)) == NULL) return(-1);
                    break;
            
            case '!':      // special instructions
                    if ((p = special_instruction(p)) == NULL) return(-1);
                    break;
                               
            case 'P':      // display point
                    if ((p = display_point(p)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                    
            case 'c':       // display OPEN circle
                    if ((p = display_circle(p,false)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                                      
            case 'C':       // display FILLED circle
                    if ((p = display_circle(p,true)) == NULL) return(-1);
                    turn_display_on = true;
                    break;  
                                            
            case 'q':       // display OPEN rectangle
                    if ((p = display_rectangle(p,false)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                                      
            case 'Q':       // display FILLED rectangle
                    if ((p = display_rectangle(p,true)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                    
            case 'i':       // display BMP image
                    if((p = display_BMP(p)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                    
            case 'T':       // display time
                    if ((p = display_time_day(p, false)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                                      
            case 'D':       // display date
                    if ((p = display_time_day(p, true)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                    
            case 't':         // display text
                    if ((p = display_txt(p, false)) == NULL) return(-1);
                    turn_display_on = true;
                    break;

            case 'l':          // draw line
                    if ((p = display_line(p)) == NULL) return(-1);
                    turn_display_on = true;
                    break;
                    
            case 'n':         // display number 
                    if ((p = display_txt(p, true)) == NULL) return(-1);  
                    turn_display_on = true;
                    break;
                        
            default:
                    printf("INVALID command %c\n", c);
                    return(-2);
                    break;
        }
    }
    
    // if any command to display (text, number or bitmap)
    if (turn_display_on) { 
        EPD_DisplayOn = true;   
        EPD_Display(BlackImage, RedImage);
        DEV_Delay_ms(2000);
    }
    
    return(0);
}
/**
 * @brief : read instructions from file
 * @param optarg: the instruction filename
 */
void read_from_file(char * optarg)
{
    FILE    *fp;
    bool    in_quotes = false;
    char    line[MAXTEXTLENGTH], last_char_added;
    bool    header_char = true;          // waiting for header of instruction
    bool    escape = false;             // escape character \ detected
    int     i, s = 0, j=0 ;
  
    // open instruction file
    if ( ! (fp=fopen(optarg,"r")) ) {
        p_printf(D_RED, "Can not open instruction file %s \n", optarg);
        close_out(EXIT_FAILURE);
    }
    
    // get line by line
    while (fgets(line, MAXTEXTLENGTH, fp)) {
        
        // count lines
        j++;
                
        // parse line
        for(i = 0; i < MAXTEXTLENGTH; i++) {
            
            // skip spaces and tabs (if not in quotes)
            if (line[i] == 0x20 || line[i] == 0x09)
                if (! in_quotes) continue;
            
            // if end of line
            if (line[i] == 0x0) break;
            
            // check for comments or NL/CR             
            if (line[i] == '#'  || line[i] == 0x0a || line[i] == 0x0d)     
                if (! in_quotes) break;
            
            // check for right starting character /    
            if (header_char) {       
                
                if (line[i] != '<') {
                    p_printf(D_RED,"invalid start character. expected '<' but got %c, 0X%x\n", line[i], line[i]);
                    fclose(fp);
                    close_out(EXIT_FAILURE);
                }
                
                header_char = false;
            } 
 
            Instruction[s++] = line[i]; 
            
            // save last character added for check later
            last_char_added = line[i];
            
            // check on buffer overrun
            if (s == MAXINSTRUCTIONS) {
                p_printf(D_RED,"Instructions exceeded maximum instruction length : %d\n", MAXINSTRUCTIONS);
                fclose(fp);
                close_out(EXIT_FAILURE);
            }                        
        
            // check for escape character when in quotes
            // this will allow '\''  as ' and '\\' as '\'
            // it is handled in later routines, this is enabling pass through
            if (in_quotes && line[i] == '\\')  {
                escape = true;
                continue;
            }
            
            // if no escape character was captured
            if (! escape)  {
                               
                // toggle in quotes
                if (line[i] == '\'') in_quotes = ! in_quotes;
            }
            else    // reset escape character
                escape = false;
        } // total line check
        
        // do a number of checks after end of line
        if (in_quotes)
        {
            p_printf(D_RED,"Closing quote missing in line: %s\n", line);
            fclose(fp);
            close_out(EXIT_FAILURE);
        }           

        // weak check, but still... if not start or end indicator
        // last valid character on a line MUST be a comma ,
        if (last_char_added!= '<' &&  last_char_added != '>' && i != 0) {
            
            if (Instruction[s-1] != ',') {
                p_printf(D_RED,"Expected comma at end of line, but got '%c' , 0x%x\n", line[s-1], line [s-1]);
                printf("line %d in question is %s\n",j, line);
                fclose(fp);
                close_out(EXIT_FAILURE);
            }
        }
    } // done all lines from file   
     
    fclose(fp);
    
    // the last character added should hold the terminator or the instruction   
    if (last_char_added != '>') {
        p_printf (D_RED,"Expected > end of line, but got '%c', 0x%x", last_char_added, last_char_added);
        close_out(EXIT_FAILURE);
    } 
    
    Instruction[s] = 0x0;
    
    // debug only
    Debug("instruction : %s\n", Instruction);
}

/**
 * @brief open named pipes in case of inter-process communication
 */
void connect_pipes()
{
    // open input pipe from other program (open O_RDWR to prevent blocking)
    if ( (p_fd_r = open(name_pipe_r,O_RDWR)) < 0)
    {
        printf("can not open named pipe %s\n", name_pipe_r);
        close_out(EXIT_FAILURE);
    }

    // open return pipe to other program (open O_RDWR to prevent blocking)
    if ( (p_fd_w = open(name_pipe_w,O_RDWR)) < 0)
    {
        printf("can not open named pipe %s\n", name_pipe_w);
        close_out(EXIT_FAILURE);
    }
    
    Debug("pipes have been connected : %s, %s\n", name_pipe_r, name_pipe_w);
    
}

/**
 * @brief sent a buffer to the remote program
 */
int sent_to_pipe(char *buf)
{
    // let remote program know
    if (write(p_fd_w, buf, strlen(buf)) != strlen(buf))
    {
        printf("Error during writing to remote program\n");
        return(-1);
    }
    
    Debug("sent to pipe : %s\n", buf);
    
    return(0);  
}

/**
 * @brief read and parse and communicate with remote program
 */
void Comm_Over_Pipe()
{
    // connect read and write pipe
    connect_pipes();
    
    int  ret, j, i, n;
    char buf[BUFSIZE];     // received command from remote
    char ret_buf[20];      // sent to program

    i = 0;
    
    memset(Instruction, 0x0, MAXINSTRUCTIONS);
    
    while(1)
    {
        printf("EPD server: wait input from remote program\n");

        n = read(p_fd_r, buf, BUFSIZE);
      
        // Any input ?
        if (n > 0)
        {
            // terminate received buffer
            buf[n] = 0x0;

            Debug("received %s, with length %d\n", buf, n);

            // check for instruction to start all over
            if (strstr(buf,"<<NEW>>") != NULL) {
                i = 0;
                continue;
            }
            
            // check for instruction to close down
            if (strstr(buf,"<<CLOSE>>") != NULL) {
                close_out(EXIT_SUCCESS);
            }

            // parse incoming EPD instruction   
            for (j = 0; j < n; j++) {
                
                if (i == MAXINSTRUCTIONS) {
                    Instruction[i] = 0x0;
                    printf("Too many instructions for buffer : %s\n", Instruction);
                    if (sent_to_pipe("<<OVERRUN>>") == -1)
                        close_out(EXIT_FAILURE);
                }
                
                Instruction[i++] = buf[j];
             
                // check for last character
                if (buf[j] == '>') {
                    
                    // let remote know execution is starting
                    if (sent_to_pipe("<<START>>") == -1)
                        close_out(EXIT_FAILURE);                   
                    
                    // set hardware and EPD correct
                    hw_init();
                    
                    Debug("Pipe got instruct %s\n", Instruction);                   
                    
                    // execute received instruction
                    ret = parse_string_instruction();
                   
                    // set EPD to deepsleep
                    if (EPD_DisplayOn) {
                        EPD_Sleep();
                        EPD_DisplayOn = false;
                    }
                    
                    // check for result
                    if (ret == 0) {
                        Debug("execution succesfull\n");
                        sprintf(ret_buf, "<<OK>>");
                    }
                    else {
                        Debug("Error during execution : %s\n", Instruction);
                        sprintf(ret_buf, "<<ERROR%d>>",ret);
                    }
                    
                    if (sent_to_pipe(ret_buf) == -1)
                        close_out(EXIT_FAILURE);
                   
                    // reset index instructions
                    i=0;
                    
                    // indicate we got all and break
                    j=n+2;
                    
                    // reset buffers
                    memset(Instruction, 0x0, MAXINSTRUCTIONS);
                                        
                    break;
                }
            }
            
            if( j != n+2) {
                
                // let remote know more data is needed
                Debug("request for more data\n");
                if (sent_to_pipe("<<MORE>>") == -1)  close_out(EXIT_FAILURE);
            }
        }
        
        else if (n < 0)
        {
            printf("lost connection ? Closing pipes and try to reconnect");
            
            // reinit... as pipe connection seems to have been lost
            close(p_fd_r);
            close(p_fd_w);
            
            // reset filedescriptors
            p_fd_r = p_fd_w = -1;
            
            // reset offset instruction
            i=0;
            
            connect_pipes();
        }
        
    } // while
}

/***********************
 *  program starts here
 **********************/
int main(int argc, char *argv[])
{
    int opt;
    bool Pipe_Comm = false;

    // Exception handling:ctrl + c
    signal(SIGINT, Handler);

    init_variables();
    
    while ((opt = getopt(argc, argv, "dhHF:T:Pr:w:")) != -1) {
        
        switch(opt){
            case 'F':           // read instruction from file
                read_from_file(optarg);
                break;
            
            case 'T':           // instruction on the command line
                strncpy(Instruction, optarg, sizeof(Instruction));
                break;
            
            case 'P':           // perform pipe communications
                Pipe_Comm = true;
                break;
                
            case 'r':           // pipe to read from
              strncpy(name_pipe_r, optarg,MAXFILENAME);
              break;

            case 'w':           // pipe to write to 
              strncpy(name_pipe_w, optarg,MAXFILENAME);
              break;

            case 'h':           // display help
            case 'H':
                usage();
                close_out(EXIT_SUCCESS);
                break;
                
            case 'd':           // debugger on
            case 'D':
                // enable debug messages
                Set_Debug(1);
                break;
                                        
            default:
                p_printf(D_RED, "unknown option %c, 0x%x\n", opt,opt);
                p_printf(D_YELLOW,"Obtain help-info with -h or -H option\n");
                close_out(EXIT_FAILURE);
        }
    }
 
    if (geteuid() != 0)  {
        p_printf(RED,(char *) "You must be super user\n");
        exit(EXIT_FAILURE);
    }  
    
    // initialize the hardware
    hw_init();
    
    // create in memory IMAGE
    image_init();
    
    if (Pipe_Comm) Comm_Over_Pipe();
    
    // parse command string
    else parse_string_instruction();
     
    close_out(EXIT_SUCCESS);
    
    // stop -WALL complaining
    exit(0);
}

/******************************************************************************
function:   Show English characters
parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
    * 
based on the original Paint_DrawChar, but now support colors in a single line
******************************************************************************/
void ePaint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground)
{
    UWORD Page, Column;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        printf("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page ++ ) {
        
        for (Column = 0; Column < Font->Width; Column ++ ) {
        // add color display to the DrawChar
            if (*ptr & (0x80 >> (Column % 8))) {
                
                 if (Color_Foreground == COLOR) {
                    Paint_SelectImage(RedImage);
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, RED);
                    Paint_SelectImage(BlackImage);
                }
                else {
                    Paint_SelectImage(BlackImage);
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                }
            } 
                
            else {
                
                if (Color_Background == COLOR) {
                    Paint_SelectImage(RedImage);
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, RED);
                    Paint_SelectImage(BlackImage);
                }
                else  {
                    Paint_SelectImage(BlackImage);
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                }                       
            }
            
            //One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        }// Write a line
        if (Font->Width % 8 != 0)
            ptr++;
    }// Write all
}

/******************************************************************************
function:   Display the string
parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the English string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
    * 
    * 
    * 
Is the same as Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Background, UWORD Color_Foreground )
BUT supports colored text by calling ePaint_DrawChar()
******************************************************************************/
void ePaint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Background, UWORD Color_Foreground,
                         UWORD *last_Xpoint, UWORD *last_Ypoint )
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        printf("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }

        ePaint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
    }
    
    *last_Xpoint = Xpoint;
    *last_Ypoint = Ypoint;
}

/******************************************************************************
function:   Display number
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
Same as Paint_DrawNum, only calls ePaint_DrawString_EN to support color
******************************************************************************/
#define  ARRAY_LEN1 255
void ePaint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Background, UWORD Color_Foreground,
                   UWORD *last_Xpoint, UWORD *last_Ypoint)
{

    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN1] = {0}, Num_Array[ARRAY_LEN1] = {0};
    uint8_t *pStr = Str_Array;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        printf("Paint_DisNum Input exceeds the normal display range\r\n");
        return;
    }

    //Converts a number to a string
    while (Nummber) {
        Num_Array[Num_Bit] = Nummber % 10 + '0';
        Num_Bit++;
        Nummber /= 10;
    }

    //The string is inverted
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }

    //show
    ePaint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Background, Color_Foreground, last_Xpoint, last_Ypoint);
}

/******************************************************************************
function:   Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment

same as Paint_DrawLine BUT support color line
******************************************************************************/
void ePaint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
{
    UWORD Color1 = Color;
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        printf("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    // support color
    if (Color == COLOR) {
        Paint_SelectImage(RedImage);
        Color1=BLACK;
    }
    else
        Paint_SelectImage(BlackImage);

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            //Debug("LINE_DOTTED\r\n");
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Dot_Pixel, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color1, Dot_Pixel, DOT_STYLE_DFT);
        }
        
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
            
    Paint_SelectImage(BlackImage);
}
