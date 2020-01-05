#include <avr/io.h>
#include <string.h> /* memcpy */

#include "oled_lib.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Transmit a byte and ack bit
//
inline void i2cByteOut(uint8_t b) // This is only called from i2Cbegin, before tones are started so don't need to worry about
{
  uint8_t i;
  //byte bOld = I2CPORT & ~((1 << BB_SDA) | (1 << BB_SCL));

  // DUNCAN  I2CPORT &= ~(1<< BB_SCL); //Toggle clock - Off - this is redundant because i2cbegin

  // #define BB_SDA 0 // (1<<0) = 1
  ///#define BB_SCL 2   // (1<<2) = 4

  CLEAR_SCL;

  for (i = 0; i < 8; i++)
  {
    // I2CPORT &= ~(1 << BB_SDA);
    CLEAR_SDA;
    if (b & 0x80)
      SET_SDA;//I2CPORT |= (1 << BB_SDA);

    //I2CPORT = bOld

    SET_SCL;
    CLEAR_SCL;
    //I2CPORT |= (1 << BB_SCL); // ON
    //I2CPORT &= ~(1<< BB_SCL); //Toggle clock - Off

    b <<= 1;
  } // for i
  // ack bit

  CLEAR_SDA;
  SET_SCL;
  CLEAR_SCL;

  //I2CPORT &= ~(1 << BB_SDA); // set data low
  //I2CPORT |= (1 << BB_SCL); // toggle clock
  //I2CPORT &= ~(1 << BB_SCL); // toggle clock


} /* i2cByteOut() */

void i2cBegin(uint8_t addr)
{
  /*I2CPORT |= ((1 << BB_SDA) + (1 << BB_SCL));
    I2CDDR |= ((1 << BB_SDA) + (1 << BB_SCL));
    I2CPORT &= ~(1 << BB_SDA); // data line low first
    I2CPORT &= ~(1 << BB_SCL); // then clock line low is a START signal
  */
  SET_SDA;
  SET_SCL;
  //I2CDDR |= ((1 << BB_SDA) + (1 << BB_SCL)); // ASM THIS...

  OUTPUT_SDA;
  OUTPUT_SCL;

  CLEAR_SDA;
  CLEAR_SCL;

  i2cByteOut(addr << 1); // send the slave address
} /* i2cBegin() */

void i2cWrite(uint8_t *pData, uint8_t bLen)
{
  uint8_t i, b;
  //byte bOld = I2CPORT & ~((1 << BB_SDA) | (1 << BB_SCL)); // PORTB with SDA and SCL off

  while (bLen--)
  {
    b = *pData++;
    if (b == 0 || b == 0xff) // special case can save time
    {
      //I2CPORT &= ~(1 << BB_SDA); // switches off SDA in bOld
      CLEAR_SDA;

      if (b & 0x80)
        SET_SDA;//I2CPORT |= (1 << BB_SDA); // switches on SDA in bOld
      //I2CPORT = bOld; // sets PORTB to bOld (SDA is set)

      //I2CPORT &= ~(1<< BB_SCL); // Toggle clock OFF
      CLEAR_SCL;

      for (i = 0; i < 8; i++)
      {
        //I2CPORT |= (1 << BB_SCL); // SCL = ON, SDA stays the same
        //I2CPORT &= ~(1<< BB_SCL); // Toggle clock on and off
        SET_SCL;
        CLEAR_SCL;
      } // for i
    }
    else // normal byte needs every bit tested
    {
      //I2CPORT &= ~(1<< BB_SCL); // Toggle clock OFF
      CLEAR_SCL;

      for (i = 0; i < 8; i++)
      {

        //I2CPORT &= ~(-1 << BB_SDA);
        CLEAR_SDA;
        if (b & 0x80)
          SET_SDA; //I2CPORT |= (1 << BB_SDA);

        //I2CPORT = bOld;

        //I2CPORT |= (1 << BB_SCL);// Turn on
        //I2CPORT &= ~(1<< BB_SCL); // Toggle clock on and off
        SET_SCL;
        CLEAR_SCL;

        b <<= 1;
      } // for i

    }
    // ACK bit seems to need to be set to 0, but SDA line doesn't need to be tri-state
    //I2CPORT &= ~(1 << BB_SDA);
    //I2CPORT |= (1 << BB_SCL); // toggle clock
    //I2CPORT &= ~(1 << BB_SCL);

    CLEAR_SDA;
    SET_SCL;
    CLEAR_SCL;
  } // for each byte
} /* i2cWrite() */

//
// Send I2C STOP condition
//
void i2cEnd()
{
  /*I2CPORT &= ~(1 << BB_SDA);
    I2CPORT |= (1 << BB_SCL);
    I2CPORT |= (1 << BB_SDA);*/

  CLEAR_SDA;
  SET_SCL;
  SET_SDA;

  //I2CDDR &= ((1 << BB_SDA) | (1 << BB_SCL)); // let the lines float (tri-state)

  asm("cbi 0x17, 2\n");   // 0x17 = DDRB &= ~(1<<2);
  asm("cbi 0x18, 0\n");  // DDRB &= ~(1<<0);
} /* i2cEnd() */

// Wrapper function to write I2C data on Arduino
void I2CWrite(int iAddr, unsigned char *pData, int iLen)
{
  i2cBegin(oled_addr);
  i2cWrite(pData, iLen);
  i2cEnd();
} /* I2CWrite() */


void oledInit(uint8_t bAddr, int bFlip, int bInvert)
{
  unsigned char uc[4];
  unsigned char oled_initbuf[] = {0x00, 0xae, 0xa8, 0x3f, 0xd3, 0x00, 0x40, 0xa1, 0xc8,
                                  0xda, 0x12, 0x81, 0xff, 0xa4, 0xa6, 0xd5, 0x80, 0x8d, 0x14,
                                  0xaf, 0x20, 0x02
                                 };
  // unsigned char oled_initbuf[]={0x00, 0xA3, 0xC8,0xA1,0xA8, 0x3F,0xDA, 0x12, 0x8D, 0x14, 0xA5,0xAF};

  oled_addr = bAddr;
  I2CDDR &= ~(1 << BB_SDA);
  I2CDDR &= ~(1 << BB_SCL); // let them float high
  I2CPORT |= (1 << BB_SDA); // set both lines to get pulled up
  I2CPORT |= (1 << BB_SCL);

  I2CWrite(oled_addr, oled_initbuf, sizeof(oled_initbuf));
  if (bInvert)
  {
    uc[0] = 0; // command
    uc[1] = 0xa7; // invert command
    I2CWrite(oled_addr, uc, 2);
  }
  if (bFlip) // rotate display 180
  {
    uc[0] = 0; // command
    uc[1] = 0xa0;
    I2CWrite(oled_addr, uc, 2);
    uc[1] = 0xc0;
    I2CWrite(oled_addr, uc, 2);
  }
} /* oledInit() */

// Send a single byte command to the OLED controller
void oledWriteCommand(unsigned char c)
{
  unsigned char buf[2];

  buf[0] = 0x00; // command introducer
  buf[1] = c;
  I2CWrite(oled_addr, buf, 2);
} /* oledWriteCommand() */

void oledWriteCommand2(unsigned char c, unsigned char d)
{
  unsigned char buf[3];

  buf[0] = 0x00;
  buf[1] = c;
  buf[2] = d;
  I2CWrite(oled_addr, buf, 3);
} /* oledWriteCommand2() */

//
// Sets the brightness (0=off, 255=brightest)
//
void oledSetContrast(unsigned char ucContrast)
{
  oledWriteCommand2(0x81, ucContrast);
} /* oledSetContrast() */

//
// Send commands to position the "cursor" (aka memory write address)
// to the given row and column
//
void oledSetPosition(int x, int y)
{

  ////ssd1306_send_command3(renderingFrame | (y >> 3), 0x10 | ((x & 0xf0) >> 4), x & 0x0f);
  /* oledWriteCommand(0xb0 | (y >> 3));  //x); // go to page Y
    oledWriteCommand(0x10 | ((x & 0xf)>>4)); // upper col addr
    oledWriteCommand(0x00 | (x  & 0xf)); // // lower col addr*/


  oledWriteCommand(0xb0 | y >> 3); // go to page Y
  oledWriteCommand(0x00 | (x & 0xf)); // // lower col addr
  oledWriteCommand(0x10 | ((x >> 4) & 0xf)); // upper col addr
  //iScreenOffset = (y*128)+x;
}

void oledWriteDataBlock(const uint8_t *ucBuf, int iLen)
{
  unsigned char ucTemp[17]; // 16 bytes max width + 1

  ucTemp[0] = 0x40; // data command
  memcpy(&ucTemp[1], ucBuf, iLen);

  I2CWrite(oled_addr, ucTemp, iLen + 1);
  // Keep a copy in local buffer
}

void oledFill(unsigned char ucData)
{
  int x, y;
  unsigned char temp[16] = {0};

  memset(temp, ucData, 16);
  for (y = 0; y < 8; y++)
  {
    oledSetPosition(0, y * 8); // set to (0,Y)
    for (x = 0; x < 8; x++)
    {
      oledWriteDataBlock(temp, 16);
    } // for x
  } // for y
} /* oledFill() */
/////////////////////////////////////////////////////////////////////////////////////////////////////////
