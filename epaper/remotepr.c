/**
 * Remote program example
 * 
 * Will display a round clock that moves horizontal and digital clock
 * that moves vertical. To be used incombination with epaper.
 * 
 * The default pipes used are EPD_to and EPD_from. They can be created with 
 * ./create_pipes.
 * 
 * See document epaper.odt
 * 
 * Paul van Haastrecht, July 2019
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
 ******************************************************************/
 
# include <sys/stat.h>
# include <fcntl.h>
# include <stdarg.h>
# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>     //exit()
# include <signal.h>     //signal()
# include <time.h>      // for Time or date
# include <strings.h>   // strcasecmp()
# include <string.h>    // strcpy()
# include <stdint.h>
# include <stdbool.h>
# include <getopt.h>
# include <poll.h>

// version info in usage() 
#define VERSION "1.0 July 2019"

#define MAXFILENAME 100         // maximum length file or pipename

/*hold pipe info */
char name_pipe_r[MAXFILENAME] = "./EPD_from";   // can be overruled from command line
char name_pipe_w[MAXFILENAME] = "./EPD_to";     // can be overruled from command line
int p_fd_r = -1;                // pipe handles
int p_fd_w = -1;
struct  pollfd fd;              // needed for polling

bool DEBUG = false;

/**
 * @brief System Exit
 */
void close_out(int ret)
{
    if (DEBUG) printf("closing out program\n");
    
    // close pipes (if opened)
    if (p_fd_r  > 0)  close(p_fd_r);
    if (p_fd_w  > 0)  close(p_fd_w);

    exit(ret);
}

void  Handler(int signo)
{
    //System Exit
    if (DEBUG) printf("\r\nProgram Handler:Interrupt received\r\n");
    close_out(EXIT_SUCCESS);
}

/** 
 * @brief open pipes to read and write  
 */
void connect_pipes()
{
    // open input pipe from EPD (open O_RDWR to prevent blocking)
    if ( (p_fd_r = open(name_pipe_r,O_RDWR)) < 0)
    {
        printf("can not open named pipe to read %s\n", name_pipe_r);
        close_out(EXIT_FAILURE);
    }

    // open return pipe to EPD (open O_RDWR to prevent blocking)
    if ( (p_fd_w = open(name_pipe_w,O_RDWR)) < 0)
    {
        printf("can not open named pipe to write %s\n", name_pipe_w);
        close_out(EXIT_FAILURE);
    }
   
    // initialize polling structure to check whether a new command was
    fd.fd = p_fd_r;
    fd.events = POLLIN;
}

/**
 * @brief sent a buffer to the EPD
 * 
 * @return
 * 0 = OK
 * -1 = error
 */
int sent_to_pipe(char *buf, int len)
{
    if (DEBUG) printf("sending %s, length %d\n", buf, len);
    
    // let remote program know
    if (write(p_fd_w, buf, len) != len)
    {
        printf("Error during writing to EPD\n");
        return(-1);
    }
    
    return(0);  
}

/**
 * @brief : wait for response remote program
 * 
 * @return :
 * 
 * START 2  // execution started
 * MORE  1  // more instruction needed, end not detected
 * OK :  0  // all good, done
 * 
 * ERROR ; Negative 
 *  -4 unknown responds
 *  -3 receive buffer overrun at epaper
 *  -2 syntax error
 *  -1 execution error
 */
int read_EPD()
{
    int n, ret = -1;
    char buf[20];          // received command from EPD
    
    while(1)
    {
        if (DEBUG) printf("Wait responds from EPD program\n");

        n = read(p_fd_r, buf, sizeof(buf));
        
        // Any input ?
        if (n > 0)
        {
            // terminate received buffer
            buf[n] = 0x0;
            
            if (DEBUG) printf("received %s. length %d\n", buf, n);

            // check for instruction to start all over
            if (strstr(buf,"<<OK>>") != NULL) {
                ret = 0;
            }

            else if (strstr(buf,"<<MORE>>") != NULL) {
                ret = 1;
            }
            
            else if (strstr(buf,"<<START>>") != NULL) {
                ret = 2;
            }
            else if (strstr(buf,"<<ERROR-1>>") != NULL) {
                ret = -1;
            }               
            else if (strstr(buf,"<<ERROR-2>>") != NULL) {
                ret = -2;
            }
            else if (strstr(buf,"<<OVERRUN>>") != NULL) {
                ret = -3;
            }
            else
                ret = -4;
                
            return(ret);
        }

        else if (n < 0)
        {
            if (DEBUG) printf("reset/closing pipe");
            
            // reinit... as pipe connection seems to have been lost
            close(p_fd_r);
            close(p_fd_w);
            
            // reset filedescriptors
            p_fd_r = p_fd_w = -1;
       
            connect_pipes();
        }
        
        // wait 3 seconds for retry
        sleep(3);
    }
}

/**
 * @brief sent a complete instruction to the EPD server
 * 
 * @param instruction : all the instructions to be sent
 * @param length : length of instructions
 * 
 * @return
 * 0 succesfull
 * -1 error
 */
#define CHUNKSIZE 300
int send_EPD(char *instruction, int length)
{    
    int     i,j;
    char    buf[500];       // send max 500 bytes in one go
    int     ret;
    i = j = 0;
  
    // remove any pending responds from pipe
    if (poll(&fd,1,50) > 0)   read(p_fd_r, buf, sizeof(buf));
    
    while (1)
    {   
        // if not all has been sent yet
        if (i + j  != length) {
            
            // read instruction into send buffer
            for (i = 0; i < CHUNKSIZE; i++) {
            
                // skip header
                buf[i] = instruction[i+j];
            
                // if all done
                if (i + j == length) break;
            }

            if (sent_to_pipe(buf, i) != 0) {
                if (DEBUG) printf("Error during sending command return code\n");
                return(-1);
            }
        }
        
        ret = read_EPD();
        
        switch (ret) {
            case 0 :
                if (DEBUG) printf("Successfull execution has been completed\n");
                return(0);
                break;
                
            case 1 :
                if (DEBUG) printf("Request for more\n");
                
                if (i +j == length) {
                    if (DEBUG) printf("there is NO more\n!");
                                        
                    // flush buffer
                    if (sent_to_pipe("<<NEW>>", 7) != 0) {
                        if (DEBUG) printf("Error during sending command <<NEW>>\n");
                    }
                    
                    return(-1);
                }
                
                // set pointers for next instruction chunk to sent
                j += i;
                i=0;

                break;
                
            case 2:
                if (DEBUG) printf("Transfer complete, waiting execution\n");
                break;
            
            case -1:
                if (DEBUG) printf("Execution error\n");
                return(-1);
                break;
                
            case -2:
                if (DEBUG) printf("Syntax error\n");
                return(-1);
                break;

            case -3:
                printf("epaper has a buffer overrun. use debugger\n");
                close_out(EXIT_FAILURE);
                break;
                               
            default:
                printf("Unknown return code %d\n",ret);
                return(-1);
                break;
        }
    }
}

/**
 *  @brief create time or date display info
 * 
 * @param
 * buf : buffer to store result
 * tm   : initialised with time information
 * date : if true return date, else return time
 * 
 */
void display_time_day(char *buf, struct tm *tm, bool date)
{
    
    static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // if date is requested
    if (date){
        sprintf(buf, "%.3s %3d %.3s %d ",wday_name[tm->tm_wday], tm->tm_mday,
                mon_name[tm->tm_mon], 1900 + tm->tm_year);
    }
    else {
        sprintf(buf, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
}

/**
 *  @brief main continuous loop
 */

void start_clock()
{
    char    buf[250], date_buf[20], time_buf[20];
    time_t  ltime;
    struct  tm *tm;
    int     ind, h_xend, h_yend, m_xend, m_yend;
    int     clock_xoffset = 0;             // offset difference from 150
    int     time_offset = 0;
    float   xend_next, yend_next;
    float   xoffset, yoffset;
    
    // 5 minute positions:  Xend, Yend assuming X=150, Y=150 middle point
    int minpos[12][2] = {
        {200, 55},  //1
        {240,100},  //2
        {250,150},  //3 
        {240,200},  //4
        {200,245},  //5
        {150,255},  //6
        {100,245},  //7
        { 60,200},  //8
        { 50,150},  //9
        { 60,100},  //10
        {100, 55},  //11
        {150, 45}   //12
    };
    
    // hour positions : Xend, Yend assuming X=150, Y=150 middle point
    int hourpos[12][2] = {
        {185, 85},  //1 +x 
        {215,110},  //2
        {220,150},  //3 
        {215,190},  //4 
        {185,215},  //5
        {150,225},  //6
        {115,215},  //7 -x
        { 85,190},  //8
        { 70,150},  //9
        { 85,110},  //10
        {115, 85},  //11
        {150, 75}   //12
   };

   printf("Starting Clock\n");
    
   while (1)
   {
        // clear buffer
        memset(buf, 0x0, sizeof(buf));
       
        // get current time
        ltime = time(NULL);
        tm = localtime(&ltime);  
        
        /** get hour info **/
        ind = tm->tm_hour;
        if (ind > 12) ind -= 12;
        if (ind == 0) ind = 11;     // 12 clock
        else ind -= 1;
     
        /** if not exactly on the full hour, the hour indicator needs to
         * move between the current and next hour */
        // get base/starting position
        h_xend = hourpos[ind][0];
        h_yend = hourpos[ind][1];
        
        // get next position
        ind++;
    
        if (ind == 12) ind = 0;
        xend_next = (float) hourpos[ind][0];
        yend_next = (float) hourpos[ind][1];
    
        // determine how much of the hour has passed to fine tune position
        xoffset = ((xend_next - (float) h_xend) / 60) * tm->tm_min;
        yoffset = ((yend_next - (float) h_yend) / 60) * tm->tm_min;
    
     //printf("h_xend %d, h_yend %d, xend_next %f, yend_next %f, xoffset %f, yoffset %f\n",
     //h_xend, h_yend, xend_next, yend_next, xoffset, yoffset);
      
        // add offset to hour position
        h_xend += (int) xoffset;
        h_yend += (int) yoffset;
        
        /** determine minutes pointer position **/
        
        /* if not exactly on the 5 minute line, the minutes pointer needs to be
         * in between the passed 5 min and next 5 min line */
         
        ind = tm->tm_min/5;
        if (ind == 0) ind = 11;     // 12.00 clock
        else ind -= 1;
        
        // get starting position
        m_xend = minpos[ind][0];
        m_yend = minpos[ind][1];    
        
        // get next position
        ind++;
        
        if (ind == 12) ind = 0;
        xend_next = (float) minpos[ind][0];
        yend_next = (float) minpos[ind][1];
        
        // determine how much of 5 min has passed and impact on position
        xoffset = ((xend_next - (float) m_xend) / 5) * (tm->tm_min - ((tm->tm_min/5)*5));
        yoffset = ((yend_next - (float) m_yend) / 5) * (tm->tm_min - ((tm->tm_min/5)*5));
     
     //printf("m_xend %d, m_yend %d, xend_next %f, yend_next %f, xoffset %f, yoffset %f\n",
     //m_xend, m_yend, xend_next, yend_next, xoffset, yoffset);
     
        // add offset to hour position
        m_xend += (int) xoffset;
        m_yend += (int) yoffset;

        // create readable time and date
        display_time_day(date_buf, tm, true);
        display_time_day(time_buf, tm, false);
          
        // draw clock and add hour and minutes
        sprintf(buf,
        "<!=c, \
        m=n,r=0,p=%d:150, d=b,C=120:5,\
        p=%d:150,d=c,l=%d:%d:0:3,\
        p=%d:150,l=%d:%d:0:2, \
        d=c, p=85:%d, Q=550:%d:5, \
        f='font24',p=90:%d,d=b,t='%s,  %s'>",
        clock_xoffset + 150,
        clock_xoffset + 150, h_xend + clock_xoffset, h_yend, 
        clock_xoffset + 150, m_xend + clock_xoffset, m_yend, 
        310 + time_offset, 315+time_offset, 
        280 + time_offset, time_buf, date_buf);

        if (send_EPD(buf,strlen(buf)) != 0) {
            if (DEBUG) printf("stopping clock\n");
            close_out(EXIT_FAILURE);
        }

        // define the offset from 150 to move clock
        clock_xoffset += 50;
        if (clock_xoffset >= 400) clock_xoffset = 0; 
        
        // define the offset from 300 to move the time
        time_offset += 24;
        if (time_offset > 60 ) time_offset = 0;
          
        // update every minute
        ind = tm->tm_min;
        while (ind == tm->tm_min) {
            sleep(10);
            // get current time
            ltime = time(NULL);
            tm = localtime(&ltime);
        } // wait one minute
        
    } // for ever loop  
}

/**
 * @brief display usage information
 */
void usage()
{
    printf("remote [options]  (version %s) \n\n"
    
    "-r     pipename read from named pipe (default %s)\n"
    "-w     pipename write to named pipe  (default %s)\n"
    "-d     show debug information\n"
    "-h     show this help information\n",
    VERSION,name_pipe_r,name_pipe_w);
}

/***********************
 *  program starts here
 **********************/
int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "dhHr:w:")) != -1) {
        
        switch(opt){
            case 'r':           // pipe to read from
              strncpy(name_pipe_r, optarg,MAXFILENAME);
              break;

            case 'w':           // pipe to write to 
              strncpy(name_pipe_w, optarg,MAXFILENAME);
              break;
                
            case 'd':           // debugger on
            case 'D':
                DEBUG=true;
                break;

            case 'h':           // display help
            case 'H':
                usage();
                close_out(EXIT_SUCCESS);
                break;
                                                        
            default:
                fprintf(stderr,"unknown option %c, 0x%x\n", opt,opt);
                fprintf(stderr,"Obtain help-info with -h or -H option\n");
                close_out(EXIT_FAILURE);
        }
    }

    // Exception handling:ctrl + c
    signal(SIGINT, Handler);

    /* connect the named pipes */
    connect_pipes();

    /* start program */
    start_clock();
  
    /* stop -Wall complaining */
    exit(0);
}
