/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any redistribution


--11/20/2012--
Modified to use hardware SPI on both Arduino Uno and Akafuino X by Nathan Duprey
Only for 128x64 SSD1306 OLED Displays
*********************************************************************/

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "Adafruit_GFX.h"
#include "vSPI_SSD1306.h"

#include "glcdfont.c"

// the most basic function, set a single pixel
void vSPI_SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
		return;

	// check rotation, move pixel around if necessary
	switch (getRotation())
	{
		case 1:
			swap(x, y);
			x = WIDTH - x - 1;
			break;
		case 2:
			x = WIDTH - x - 1;
			y = HEIGHT - y - 1;
			break;
		case 3:
			swap(x, y);
			y = HEIGHT - y - 1;
			break;
	}  

	// x is which column
	if (color == WHITE)
		buffer[x+ (y / 8) * WIDTH] |= _BV((y%8));
	else
		buffer[x+ (y / 8) * WIDTH] &= ~_BV((y%8));
}

vSPI_SSD1306::vSPI_SSD1306(int8_t CS, int8_t RESET, int8_t DC, uint8_t* BUFFER, uint8_t width, uint8_t height) : Adafruit_GFX(width, height)
{
	cs = CS;
	rst = RESET;
	dc = DC;
	buffer = BUFFER;
}

void vSPI_SSD1306::begin(uint8_t vccstate)
{
	// set pin directions
	// SPI
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
	
	pinMode(dc, OUTPUT);
	pinMode(cs, OUTPUT);
	
	// Setup reset pin direction (used by both SPI and I2C)
	pinMode(rst, OUTPUT);
	digitalWrite(rst, HIGH);
	// VDD (3.3V) goes high at start, lets just chill for a ms
	delay(1);
	// bring reset low
	digitalWrite(rst, LOW);
	// wait 10ms
	delay(10);
	// bring out of reset
	digitalWrite(rst, HIGH);
	
	// Init sequence for 128x64 OLED module
	SPItransfer(SSD1306_DISPLAYOFF);                    // 0xAE
	SPItransfer(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
	SPItransfer(0x80);                                  // the suggested ratio 0x80
	SPItransfer(SSD1306_SETMULTIPLEX);                  // 0xA8
	SPItransfer(HEIGHT - 1);	// 0x3F or 0x1F (HEIGHT - 1)
	SPItransfer(SSD1306_SETDISPLAYOFFSET);              // 0xD3
	SPItransfer(0x0);                                   // no offset
	SPItransfer(SSD1306_SETSTARTLINE | 0x0);            // line #0
	SPItransfer(SSD1306_CHARGEPUMP);                    // 0x8D

	if (vccstate == SSD1306_EXTERNALVCC)
		SPItransfer(0x10);
	else
		SPItransfer(0x14);
	
	SPItransfer(SSD1306_MEMORYMODE);                    // 0x20
	SPItransfer(0x00);                                  // 0x0 act like ks0108
	SPItransfer(SSD1306_SEGREMAP | 0x1);
	SPItransfer(SSD1306_COMSCANDEC);
	SPItransfer(SSD1306_SETCOMPINS);                    // 0xDA
	// horrible hack but whatever
	if(HEIGHT==32)
		SPItransfer(0x02); // 0x12 or 0x02 (18 or 2)
	else
		SPItransfer(0x12);

	SPItransfer(SSD1306_SETCONTRAST);                   // 0x81

	if (vccstate == SSD1306_EXTERNALVCC)
		SPItransfer(0x9F);
	else
		SPItransfer(0xCF);

	SPItransfer(SSD1306_SETPRECHARGE);                  // 0xd9
	
	if (vccstate == SSD1306_EXTERNALVCC)
		SPItransfer(0x22);
	else
		SPItransfer(0xF1);

	SPItransfer(SSD1306_SETVCOMDETECT);                 // 0xDB
	SPItransfer(0x40);
	SPItransfer(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
	SPItransfer(SSD1306_NORMALDISPLAY);                  // 0xA6
	
	SPItransfer(SSD1306_DISPLAYON);//--turn on oled panel
}

void vSPI_SSD1306::invertDisplay(uint8_t i)
{
	if(i)
		SPItransfer(SSD1306_INVERTDISPLAY);
	else
		SPItransfer(SSD1306_NORMALDISPLAY);
}

void vSPI_SSD1306::SPItransfer(uint8_t command)
{
	digitalWrite(cs, HIGH);
	//*csport |= cspinmask;
	digitalWrite(dc, LOW);
	//*dcport &= ~dcpinmask;
	digitalWrite(cs, LOW);
	//*csport &= ~cspinmask;

	SPI.transfer(command);

	digitalWrite(cs, HIGH);
	*csport |= cspinmask;
}

// startscrollright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void vSPI_SSD1306::startscrollright(uint8_t start, uint8_t stop)
{
	SPItransfer(SSD1306_RIGHT_HORIZONTAL_SCROLL);
	SPItransfer(0X00);
	SPItransfer(start);
	SPItransfer(0X00);
	SPItransfer(stop);
	SPItransfer(0X01);
	SPItransfer(0XFF);
	SPItransfer(SSD1306_ACTIVATE_SCROLL);
}

// startscrollleft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void vSPI_SSD1306::startscrollleft(uint8_t start, uint8_t stop)
{
	SPItransfer(SSD1306_LEFT_HORIZONTAL_SCROLL);
	SPItransfer(0X00);
	SPItransfer(start);
	SPItransfer(0X00);
	SPItransfer(stop);
	SPItransfer(0X01);
	SPItransfer(0XFF);
	SPItransfer(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagright
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void vSPI_SSD1306::startscrolldiagright(uint8_t start, uint8_t stop)
{
	SPItransfer(SSD1306_SET_VERTICAL_SCROLL_AREA);
	SPItransfer(0X00);
	SPItransfer(HEIGHT);
	SPItransfer(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
	SPItransfer(0X00);
	SPItransfer(start);
	SPItransfer(0X00);
	SPItransfer(stop);
	SPItransfer(0X01);
	SPItransfer(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagleft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void vSPI_SSD1306::startscrolldiagleft(uint8_t start, uint8_t stop)
{
	SPItransfer(SSD1306_SET_VERTICAL_SCROLL_AREA);
	SPItransfer(0X00);
	SPItransfer(HEIGHT);
	SPItransfer(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
	SPItransfer(0X00);
	SPItransfer(start);
	SPItransfer(0X00);
	SPItransfer(stop);
	SPItransfer(0X01);
	SPItransfer(SSD1306_ACTIVATE_SCROLL);
}

void vSPI_SSD1306::stopscroll(void)
{
	SPItransfer(SSD1306_DEACTIVATE_SCROLL);
}

void vSPI_SSD1306::display(void)
{
	SPItransfer(SSD1306_SETLOWCOLUMN | 0x0);  // low col = 0
	SPItransfer(SSD1306_SETHIGHCOLUMN | 0x0);  // hi col = 0
	SPItransfer(SSD1306_SETSTARTLINE | 0x0); // line #0

	digitalWrite(cs, HIGH);
	//*csport |= cspinmask;
	digitalWrite(dc, HIGH);
	//*dcport |= dcpinmask;
	digitalWrite(cs, LOW);
	//*csport &= ~cspinmask;

	for (uint16_t i=0; i<(WIDTH * HEIGHT / 8); i++)
	{
		SPI.transfer(buffer[i]);
	}
  
	digitalWrite(cs, HIGH);
	//*csport |= cspinmask;
}


// clear everything
void vSPI_SSD1306::clearDisplay(void)
{
	memset(buffer, 0, (WIDTH * HEIGHT/8));
}