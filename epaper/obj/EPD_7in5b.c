/*****************************************************************************
* | File        :   EPD_7in5b.c
* | Author      :   Waveshare team
* | Function    :   Electronic paper driver
* | Info        :
*----------------
* | This version:   V2.0
* | Date        :   2018-11-12
* | Info        :
* 1.Remove:ImageBuff[EPD_HEIGHT * EPD_WIDTH / 8]
* 2.Change:EPD_Display(UBYTE *Image)
*   Need to pass parameters: pointer to cached data
* 3.Change:
*   EPD_RST -> EPD_RST_PIN
*   EPD_DC -> EPD_DC_PIN
*   EPD_CS -> EPD_CS_PIN
*   EPD_BUSY -> EPD_BUSY_PIN
* 
* 4. EPD_Set_Border() has been added by paulvh

#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_7in5b.h"
//#include "Debug.h"

/******************************************************************************
function :  Software reset
parameter:
******************************************************************************/
static void EPD_Reset(void)
{
    Debug("perform Reset\n");
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

/******************************************************************************
function :  send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);       // 4 wire SPI Command = 0
    DEV_Digital_Write(EPD_CS_PIN, 0);       // Chipselect
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :  send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);       // 4 wire SPI Data = 1
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :  Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_WaitUntilIdle(void)
{
    UBYTE busy;
    Debug("e-Paper busy\r\n");
    do {
        EPD_SendCommand(0x71);                  // GET_STATUS (returned BYTE is NOT used !!)
        busy = DEV_Digital_Read(EPD_BUSY_PIN);
        busy =!(busy & 0x01);
    } while(busy);
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :  Turn On Display
parameter:
******************************************************************************/
static void EPD_TurnOnDisplay(void)
{
    Debug("Turn display on\n");
    EPD_SendCommand(POWER_ON);          //POWER ON
    EPD_WaitUntilIdle();
    Debug("refresh\n");
    EPD_SendCommand(DISPLAY_REFRESH);   //display refresh
    DEV_Delay_ms(100);
    EPD_WaitUntilIdle();
    Debug("Refresh done\n");
}

/******************************************************************************
function :  Initialize the e-Paper register
parameter:
******************************************************************************/
UBYTE EPD_Init(void)
{
    EPD_Reset();                    // reset pin high/low/high

    EPD_SendCommand(POWER_SETTING); // 0x1
    EPD_SendData(0x37);             // pure driver mode:pixel bit =2`b11 output VDNS_L level (default), 1: 2-bit data mode for pure driver (default), Internal DCDC function for generate source power. (default)
    EPD_SendData(0x00);             // VGH=20V, VGL= -20V

    EPD_SendCommand(PANEL_SETTING); // 0x0
    EPD_SendData(0xCF);             // resolution 600×448, Using LUT from external Flash. Scan up. Scan right, DC-DC converter ON, Normal operation (Default)
    EPD_SendData(0x08);             // VCM_HZ, normal working

    EPD_SendCommand(PLL_CONTROL);
    EPD_SendData(0x3A);             //PLL:  0-15:0x3C (M = 7, N = 4 : 50Hz), 15+:0x3A  (M = 7, N = 2 : 100Hz)

    EPD_SendCommand(VCM_DC_SETTING);
    EPD_SendData(0x10);             // ??0x28 all temperature  range (VCOM_DC Value -voltage value)

    EPD_SendCommand(BOOSTER_SOFT_START);  //boost
    EPD_SendData (0xc7);            // VCOM: softstart period 40ms, driving strength 0, min time off 6,77us
    EPD_SendData (0xcc);            // gate: softstart period 40ms, driving strength 2, min time off 0,77us
    EPD_SendData (0x15);            // source: softstart period 10ms, driving strength 3, min time off 1,61us

    EPD_SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);  //VCOM AND DATA INTERVAL SETTING
    EPD_SendData(0x77);             // data polarity (1), border output white, CDI 10 (default)

    EPD_SendCommand(TCON_SETTING);  // temperature sensing
    EPD_SendData(0x22);             // 12 (default)

    EPD_SendCommand(SPI_FLASH_CONTROL);  //FLASH CONTROL
    EPD_SendData(0x00);                 //disable direct access external memory mode

    EPD_SendCommand(TCON_RESOLUTION);   //tres (overrules the RESOLUTION in the PANEL_SETTING)
    EPD_SendData(EPD_WIDTH >> 8);       //source 640
    EPD_SendData(EPD_WIDTH & 0xff);
    EPD_SendData(EPD_HEIGHT >> 8);      //gate 384
    EPD_SendData(EPD_HEIGHT & 0xff);

    EPD_SendCommand(0xe5);              //FLASH MODE Define the flash??
    EPD_SendData(0x03);

    return 0;
}

/******************************************************************************
function :  Clear screen
parameter:
******************************************************************************/
void EPD_Clear(void)
{
    UWORD Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    EPD_SendCommand(DATA_START_TRANSMISSION_1);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            for(UBYTE k = 0; k < 4; k++) {
                EPD_SendData(0x33);             // dummy(0) white(3) dummy(0) white(3)
            }
        }
    }

    EPD_TurnOnDisplay();
}

/******************************************************************************
 function :  Set border color
 parameter: color either B (black), W (white) or C (color)
 
 during EPD_init() EPD_SendCommand(VCOM_AND_DATA_INTERVAL_SETTING); 
 the border is set as white
 
 0111 0111
    ! !!!!
    ! Vcom/data interval 10
    DDX = 1 (data polarity)
 
 011 = WHITE   0x77
 000 = BLACK   0x17
 100 = Red0    0x97 (red or yellow)
 
 return : 0 = OK, 1 = error 
 
******************************************************************************/
int EPD_Set_Border(char color)
{
    UBYTE data;
    
    if (color == 'B' || color == 'b' )      data = 0x17;
    else if (color == 'W'|| color == 'w' )  data = 0x77;
    else if (color == 'C' || color== 'c' )  data = 0x97;
    else  return 1;
 
    EPD_SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);  //VCOM AND DATA INTERVAL SETTING
    EPD_SendData(data);             // data polarity (1), border output white, CDI 10 (default)

    return 0;
}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_Display(UBYTE *Imageblack, UBYTE *Imagered)
{
    UBYTE Data_Black, Data_Red, Data;
    UDOUBLE i, j, Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    EPD_SendCommand(DATA_START_TRANSMISSION_1);
    for (j = 0; j < Height; j++) {
        for (i = 0; i < Width; i++) {
            Data_Black = Imageblack[i + j * Width];
            Data_Red = Imagered[i + j * Width];
            for(UBYTE k = 0; k < 8; k++) {
                if ((Data_Red & 0x80) == 0x00) {
                    Data = 0x04;               //red0
                } else if ((Data_Black & 0x80) == 0x00) {
                    Data = 0x00;               //black
                } else {
                    Data = 0x03;               //white
                }
                Data = (Data << 4) & 0xFF;
                Data_Black = (Data_Black << 1) & 0xFF;
                Data_Red = (Data_Red << 1) & 0xFF;
                k += 1;

                if((Data_Red & 0x80) == 0x00) {
                    Data |= 0x04;              //red
                } else if ((Data_Black & 0x80) == 0x00) {
                    Data |= 0x00;              //black
                } else {
                    Data |= 0x03;              //white
                }
                Data_Black = (Data_Black << 1) & 0xFF;
                Data_Red = (Data_Red << 1) & 0xFF;
                EPD_SendData(Data);
            }
        }
    }

    EPD_TurnOnDisplay(); 
}

/******************************************************************************
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_Sleep(void)
{
    Debug("Set to Sleep\n");
    EPD_SendCommand(POWER_OFF);
    EPD_WaitUntilIdle();
    EPD_SendCommand(DEEP_SLEEP);
    EPD_SendData(0XA5);
}
