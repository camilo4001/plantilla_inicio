///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                              ST7789.c                             ////
////                                                                   ////
////               ST7789 display driver for CCS C compiler            ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////               This is a free software with NO WARRANTY.           ////
////                     https://simple-circuit.com/                   ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit 1.8" TFT Breakout w/SD card
    ----> http://www.adafruit.com/products/358
  The 1.8" TFT shield
    ----> https://www.adafruit.com/product/802
  The 1.44" TFT breakout
    ----> https://www.adafruit.com/product/2088
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams.
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional).

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"


/*
#include "../matriz8x8.h"
#include "../Macros_pines.h"
*/

#include "../principal.h"



#ifndef bool
#define bool int1
#endif

#define ST_CMD_DELAY      0x80    // special signifier for command lists

#define ST77XX_NOP        0x00
#define ST77XX_SWRESET    0x01
#define ST77XX_RDDID      0x04
#define ST77XX_RDDST      0x09

#define ST77XX_SLPIN      0x10
#define ST77XX_SLPOUT     0x11
#define ST77XX_PTLON      0x12
#define ST77XX_NORON      0x13

#define ST77XX_INVOFF     0x20
#define ST77XX_INVON      0x21
#define ST77XX_DISPOFF    0x28
#define ST77XX_DISPON     0x29
#define ST77XX_CASET      0x2A
#define ST77XX_RASET      0x2B
#define ST77XX_RAMWR      0x2C
#define ST77XX_RAMRD      0x2E

#define ST77XX_PTLAR      0x30
#define ST77XX_COLMOD     0x3A
#define ST77XX_MADCTL     0x36

#define ST77XX_MADCTL_MY  0x80
#define ST77XX_MADCTL_MX  0x40
#define ST77XX_MADCTL_MV  0x20
#define ST77XX_MADCTL_ML  0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1      0xDA
#define ST77XX_RDID2      0xDB
#define ST77XX_RDID3      0xDC
#define ST77XX_RDID4      0xDD




uint8_t
  _width,     ///< Display width as modified by current rotation
  _height,    ///< Display height as modified by current rotation
  _xstart,    ///< Internal framebuffer X offset
  _ystart,    ///< Internal framebuffer Y offset
  _colstart,  ///< Some displays need this changed to offset
  _rowstart,  ///< Some displays need this changed to offset
  rotation;   ///< Display rotation (0 thru 3)


// COMANDOS AGREGADOS DESDE LOBO

// ==== Display commands constants ====
#define TFT_INVOFF     0x20
#define TFT_INVONN     0x21
#define TFT_DISPOFF    0x28
#define TFT_DISPON     0x29
#define TFT_MADCTL	   0x36
#define TFT_PTLAR 	   0x30
#define TFT_ENTRYM 	   0xB7

#define TFT_CMD_NOP			0x00
#define TFT_CMD_SWRESET		0x01
#define TFT_CMD_RDDID		0x04
#define TFT_CMD_RDDST		0x09

#define TFT_CMD_SLPIN		0x10
#define TFT_CMD_SLPOUT		0x11
#define TFT_CMD_PTLON		0x12
#define TFT_CMD_NORON		0x13

#define TFT_CMD_RDMODE		0x0A
#define TFT_CMD_RDMADCTL	0x0B
#define TFT_CMD_RDPIXFMT	0x0C
#define TFT_CMD_RDIMGFMT	0x0D
#define TFT_CMD_RDSELFDIAG  0x0F

#define TFT_CMD_GAMMASET	0x26

#define TFT_CMD_FRMCTR1		0xB1
#define TFT_CMD_FRMCTR2		0xB2
#define TFT_CMD_FRMCTR3		0xB3
#define TFT_CMD_INVCTR		0xB4
#define TFT_CMD_DFUNCTR		0xB6

#define TFT_CMD_PWCTR1		0xC0
#define TFT_CMD_PWCTR2		0xC1
#define TFT_CMD_PWCTR3		0xC2
#define TFT_CMD_PWCTR4		0xC3
#define TFT_CMD_PWCTR5		0xC4
#define TFT_CMD_VMCTR1		0xC5
#define TFT_CMD_VMCTR2		0xC7

#define TFT_CMD_RDID1		0xDA
#define TFT_CMD_RDID2		0xDB
#define TFT_CMD_RDID3		0xDC
#define TFT_CMD_RDID4		0xDD

#define TFT_CMD_GMCTRP1		0xE0
#define TFT_CMD_GMCTRN1		0xE1

#define TFT_CMD_POWERA		0xCB
#define TFT_CMD_POWERB		0xCF
#define TFT_CMD_POWER_SEQ	0xED
#define TFT_CMD_DTCA		0xE8
#define TFT_CMD_DTCB		0xEA
#define TFT_CMD_PRC			0xF7
#define TFT_CMD_3GAMMA_EN	0xF2

#define ST_CMD_VCOMS       0xBB
#define ST_CMD_FRCTRL2      0xC6
#define ST_CMD_PWCTR1		0xD0

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13
#define ST7735_PWCTR6  0xFC
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_MH  0x04

#define TFT_CASET		0x2A
#define TFT_PASET		0x2B
#define TFT_RAMWR		0x2C
#define TFT_RAMRD		0x2E
#define TFT_CMD_PIXFMT	0x3A

#define TFT_CMD_DELAY	0x80


static void commandList(uint8_t *addr);
//**********************************
/*
uint8_t
  cmd_240x240[]= {
  15,                   					        // 15 commands in list
  TFT_CMD_FRMCTR2, 5, 0x0c, 0x0c, 0x00, 0x33, 0x33,
  TFT_ENTRYM, 1, 0x45,
  ST_CMD_VCOMS, 1, 0x2B,
  TFT_CMD_PWCTR1, 1, 0x2C,
  TFT_CMD_PWCTR3, 2, 0x01, 0xff,
  TFT_CMD_PWCTR4, 1, 0x11,
  TFT_CMD_PWCTR5, 1, 0x20,
  ST_CMD_FRCTRL2, 1, 0x0f,
  ST_CMD_PWCTR1, 2, 0xA4, 0xA1,
  TFT_CMD_GMCTRP1, 14, 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19,
  TFT_CMD_GMCTRN1, 14, 0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19,
  TFT_MADCTL, 1, (MADCTL_MX | 0x08),			// Memory Access Control (orientation)
  TFT_CMD_PIXFMT, 1, 0x66,            // *** INTERFACE PIXEL FORMAT: 0x66 -> 18 bit; 0x55 -> 16 bit
  TFT_CMD_SLPOUT, TFT_CMD_DELAY, 120,				//  Sleep out,	//  120 ms delay
  TFT_DISPON, TFT_CMD_DELAY, 120,
};
*/



// SCREEN INITIALIZATION ***************************************************

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.

#define ST7789_240x240_XSTART 0
#define ST7789_240x240_YSTART 80

uint8_t
  cmd_240x240[] =  {                // Init commands for 7789 screens
    9,                              //  9 commands in list:
    ST77XX_SWRESET,   ST_CMD_DELAY, //  1: Software reset, no args, w/delay
      150,                          //    150 ms delay
    ST77XX_SLPOUT ,   ST_CMD_DELAY, //  2: Out of sleep mode, no args, w/delay
      255,                          //     255 = 500 ms delay
    ST77XX_COLMOD , 1+ST_CMD_DELAY, //  3: Set color mode, 1 arg + delay:
      0x55,                         //     16-bit color
      10,                           //     10 ms delay
    ST77XX_MADCTL , 1,              //  4: Mem access ctrl (directions), 1 arg:
      0x08,                         //     Row/col addr, bottom-top refresh
    ST77XX_CASET  , 4,              //  5: Column addr set, 4 args, no delay:
      0x00,
      ST7789_240x240_XSTART,        //     XSTART = 0
      (240+ST7789_240x240_XSTART)>>8,
      (240+ST7789_240x240_XSTART)&0xFF,  //     XEND = 240
    ST77XX_RASET  , 4,              //  6: Row addr set, 4 args, no delay:
      0x00,
      ST7789_240x240_YSTART,             //     YSTART = 0
      (240+ST7789_240x240_YSTART)>>8,
      (240+ST7789_240x240_YSTART)&0xFF,  //     YEND = 240
    ST77XX_INVON  ,   ST_CMD_DELAY,  //  7: hack
      10,
    ST77XX_NORON  ,   ST_CMD_DELAY, //  8: Normal display on, no args, w/delay
      10,                           //     10 ms delay
    ST77XX_DISPON ,   ST_CMD_DELAY, //  9: Main screen turn on, no args, delay
    255 };                          //     255 = max (500 ms) delay



//*************************** User Functions ***************************//
//void tft_init(void);

//void drawPixel(uint8_t x, uint8_t y, uint16_t color);
//void drawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color);
//void drawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color);
//void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
//void fillScreen(uint16_t color);
void setRotation(uint8_t m);
void invertDisplay(bool i);
void pushColor(uint16_t color);

//************************* Non User Functions *************************//
void startWrite(void);
void endWrite(void);
void displayInit(uint8_t *addr);
void writeCommand(uint8_t cmd);
void setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**************************************************************************/
/*!
    @brief  Call before issuing command(s) or data to display. Performs
            chip-select (if required). Required
            for all display types; not an SPI-specific function.
*/
/**************************************************************************/
void startWrite(void) {
  //  output_low(TFT_CS);
  //#endif
}

/**************************************************************************/
/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required). Required
            for all display types; not an SPI-specific function.
*/
/**************************************************************************/
void endWrite(void) {
  //#ifdef TFT_CS
  //  output_high(TFT_CS);
  //#endif
}

/**************************************************************************/
/*!
    @brief  Write a single command byte to the display. Chip-select and
            transaction must have been previously set -- this ONLY sets
            the device to COMMAND mode, issues the byte and then restores
            DATA mode. There is no corresponding explicit writeData()
            function -- just use enviar_valor_spi().
    @param  cmd  8-bit command to write.
*/
/**************************************************************************/
void writeCommand(uint8_t cmd) {

  //output_low(TFT_DC);
    gpio_set_level(DC_PIN, 0);

  //enviar_valor_spi(cmd); // escribe por spi cmd
    enviar_valor_spi(cmd);

  gpio_set_level(DC_PIN, 1);
  //output_high(TFT_DC);
}

/**************************************************************************/
/*!
    @brief  Companion code to the initiliazation tables. Reads and issues
            a series of LCD commands stored in ROM byte array.
    @param  addr  Flash memory array with commands and data to send
*/
/**************************************************************************/
void displayInit(uint8_t *addr){
  uint8_t  numCommands, numArgs;
  uint32_t ms;
  TickType_t ticksDelay;

  startWrite();

  numCommands = *addr++;   // Number of commands to follow
  
  //ESP_LOGI(SPI_TAG,"ENVIO DE COMANDOS DE INICIALIZACION");


  while(numCommands--) {                 // For each command...

    writeCommand(*addr++); // Read, issue command // envia comando
    numArgs  = *addr++;    // Number of args to follow
    ms       = numArgs & ST_CMD_DELAY;   // If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;            // Mask out delay bit
    while(numArgs--) {                   // For each argument...
      //enviar_valor_spi(*addr++);   // Read, issue argument // envia valor
    	enviar_valor_spi(*addr++);
    }

    if(ms) {
      ms = *addr++; // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      //delay_ms(ms);
      ticksDelay = pdMS_TO_TICKS(ms);
      vTaskDelay(ticksDelay);
    }
  }

  ESP_LOGI(SPI_TAG,"TERMINA ENVIO DE COMANDOS DE INI");

  endWrite();
}


//*******************OTRO METODO
static void commandList(uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;				// Number of commands to follow
  while(numCommands--) {				// For each command...
    cmd = *addr++;						// save command
    numArgs  = *addr++;					// Number of args to follow
    ms       = numArgs & TFT_CMD_DELAY;	// If high bit set, delay follows args
    numArgs &= ~TFT_CMD_DELAY;			// Mask out delay bit

	//disp_spi_transfer_cmd_data(cmd, (uint8_t *)addr, numArgs);
	writeCommand(*addr++);

	while(numArgs--) {                   // For each argument...
	      //enviar_valor_spi(*addr++);   // Read, issue argument // envia valor
	    	enviar_valor_spi(*addr++);
	}

	addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
	  vTaskDelay(ms / portTICK_RATE_MS);
    }
  }
}

/**************************************************************************/
/*!
    @brief  Initialization code for ST7789 display
*/
/**************************************************************************/
void tft_init(void) {
  //#ifdef TFT_RST
    //output_high(TFT_RST);
    gpio_set_level(RST_PIN, 1);
    //output_drive(TFT_RST);
    //delay_ms(100);
    vTaskDelay(100 / portTICK_RATE_MS);

    //output_low(TFT_RST);
    gpio_set_level(RST_PIN, 0);
    //delay_ms(100);
    vTaskDelay(20 / portTICK_RATE_MS);
    //output_high(TFT_RST);
    gpio_set_level(RST_PIN, 1);

    //delay_ms(200);
    vTaskDelay(150 / portTICK_RATE_MS);

    //ESP_LOGI(SPI_TAG,"COMPLETA RESET");


  //#endif

  /*
  #ifdef TFT_CS
    output_high(TFT_CS);
    output_drive(TFT_CS);
  #endif
  */

  //output_drive(TFT_DC);

  displayInit(cmd_240x240);
  //commandList(cmd_240x240);

  _colstart = ST7789_240x240_XSTART;
  _rowstart = ST7789_240x240_YSTART;
  _height   = 240;
  _width    = 240;
  setRotation(2);
}

/**************************************************************************/
/*!
  @brief  SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
/**************************************************************************/
void setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  x += _xstart;
  y += _ystart;

  writeCommand(ST77XX_CASET); // Column addr set
  enviar_valor_spi(0);
  enviar_valor_spi(x);
  enviar_valor_spi(0);
  enviar_valor_spi(x+w-1);

  writeCommand(ST77XX_RASET); // Row addr set
  enviar_valor_spi(0);
  enviar_valor_spi(y);
  enviar_valor_spi(0);
  enviar_valor_spi(y+h-1);

  writeCommand(ST77XX_RAMWR); // write to RAM
}

/**************************************************************************/
/*!
    @brief  Set origin of (0,0) and orientation of TFT display
    @param  m  The index for rotation, from 0-3 inclusive
*/
/**************************************************************************/
void setRotation(uint8_t m) {
  uint8_t madctl = 0;

  rotation = m & 3; // can't be higher than 3

  switch (rotation) {
   case 0:
     madctl  = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
     _xstart = _colstart;
     _ystart = _rowstart;
     break;
   case 1:
     madctl  = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
     _xstart = _rowstart;
     _ystart = _colstart;
     break;
  case 2:
     madctl  = ST77XX_MADCTL_RGB;
     _xstart = 0;
     _ystart = 0;
     break;
   case 3:
     madctl  = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
     _xstart = 0;
     _ystart = 0;
     break;
  }
  startWrite();
  writeCommand(ST77XX_MADCTL);
  enviar_valor_spi(madctl);
  endWrite();
}

void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
  if((x < _width) && (y < _height)) {
    startWrite();
    setAddrWindow(x, y, 1, 1);
    enviar_valor_spi(color >> 8);
    enviar_valor_spi(color & 0xFF);
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly horizontal line (this is often optimized in a subclass!)
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void drawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color) {
  if( (x < _width) && (y < _height) && w) {   
    uint8_t hi = color >> 8, lo = color;

    if((x >= _width) || (y >= _height))
      return;
    if((x + w - 1) >= _width)  
      w = _width  - x;
    startWrite();
    setAddrWindow(x, y, w, 1);
    while (w--) {
      enviar_valor_spi(hi);
      enviar_valor_spi(lo);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly vertical line (this is often optimized in a subclass!)
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void drawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color) {
  if( (x < _width) && (y < _height) && h) {  
    uint8_t hi = color >> 8, lo = color;
    if((y + h - 1) >= _height) 
      h = _height - y;
    startWrite();
    setAddrWindow(x, y, 1, h);
    while (h--) {
      enviar_valor_spi(hi);
      enviar_valor_spi(lo);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
//fillRect(0, 0, _width, _height, color);
    //ESP_LOGI(SPI_TAG,"ENVIO DE COLOR PANTALLA");
	uint32_t conteo = 0;


  if(w && h) {                            // Nonzero width and height?  
    uint8_t hi = color >> 8, lo = color;
    if((x >= _width) || (y >= _height))
      return;
    if((x + w - 1) >= _width)  
      w = _width  - x;
    if((y + h - 1) >= _height) 
      h = _height - y;
    startWrite();
    setAddrWindow(x, y, w, h);
    uint32_t px = (uint32_t)w * h;
    
    //uint8_t valor_enviar[240*2];
    //printf("************valor (%d) reading!***********+\n", px); // si se quita geenrar error dde rxdata transfer > host maximum

	
    //for ( int linea=0;linea<240;linea++){
		//px = 240*2;
	    //printf("valor (%d) reading!\n", px); // si se quita geenrar error dde rxdata transfer > host maximum
		//vTaskDelay(1000 / portTICK_RATE_MS);NO FUNCIONA
		while (px--) {
		  //valor_enviar[conteo]=0xFF;
		  //valor_enviar[conteo+1]=lo;
		  enviar_valor_spi(hi);
		  enviar_valor_spi(lo);
		  //conteo++;
		  //conteo++;
		  //disp_spi_transfer_cmd_data(hi,0);
		  //disp_spi_transfer_cmd_data(lo,0);
		}
		
		//printf("valor (%d) reading conteo!\n", conteo);
	    //disp_spi_transfer_cmd_data(valor_enviar,px);
		
		//conteo= 0;
		//px = (uint32_t)w * 2;//h;
		
	//}
	
	
	
    //ESP_LOGI(SPI_TAG,"TERMINA ENVIO DE COLOR A PANTALLA");

    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Fill the screen completely with one color. Update in subclasses if desired!
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

/**************************************************************************/
/*!
    @brief  Invert the colors of the display (if supported by hardware).
            Self-contained, no transaction setup required.
    @param  i  true = inverted display, false = normal display.
*/
/**************************************************************************/
void invertDisplay(bool i) {
    startWrite();
    writeCommand(i ? ST77XX_INVON : ST77XX_INVOFF);
    endWrite();
}

/*!
    @brief  Essentially writePixel() with a transaction around it. I don't
            think this is in use by any of our code anymore (believe it was
            for some older BMP-reading examples), but is kept here in case
            any user code relies on it. Consider it DEPRECATED.
    @param  color  16-bit pixel color in '565' RGB format.
*/
void pushColor(uint16_t color) {
    uint8_t hi = color >> 8, lo = color;
    startWrite();
    enviar_valor_spi(hi);
    enviar_valor_spi(lo);
    endWrite();
}

// end of code.
