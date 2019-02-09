/*******************************************************************************
Copyright 2016-2018 anxzhu (github.com/anxzhu)
Copyright 2018 Valerio Nappi (github.com/5N44P) (changes)
Based on segment-lcd-with-ht1621 from anxzhu (2016-2018)
(https://github.com/anxzhu/segment-lcd-with-ht1621)

Partially rewritten and extended by Valerio Nappi (github.com/5N44P) in 2018
https://github.com/5N44P/ht1621-7-seg

Refactored. Removed dependency on any MCU hardware by Viacheslav Balandin
https://github.com/hedgehogV/HT1621-lcd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include "HT1621.hpp"
#include "math.h"
#include "stdio.h"
#include "string.h"

/**
 * @brief CALCULATION DEFINES BLOCK
 */
#define MAX_NUM     999999
#define MIN_NUM     -99999

#define MAX_POSITIVE_PRECISION 3
#define MAX_NEGATIVE_PRECISION 2

#define BITS_PER_BYTE 8

/**
 * @brief DISPLAY HARDWARE DEFINES BLOCK
 */
#define  BIAS     0x52             //0b1000 0101 0010  1/3duty 4com
#define  SYSDIS   0x00             //0b1000 0000 0000  Turn off both system oscillator and LCD bias generator
#define  SYSEN    0x02             //0b1000 0000 0010  Turn on system oscillator
#define  LCDOFF   0x04             //0b1000 0000 0100  Turn off LCD bias generator
#define  LCDON    0x06             //0b1000 0000 0110  Turn on LCD bias generator
#define  XTAL     0x28             //0b1000 0010 1000  System clock source, crystal oscillator
#define  RC256    0x30             //0b1000 0011 0000  System clock source, on-chip RC oscillator
#define  TONEON   0x12             //0b1000 0001 0010  Turn on tone outputs
#define  TONEOFF  0x10             //0b1000 0001 0000  Turn off tone outputs
#define  WDTDIS1  0x0A             //0b1000 0000 1010  Disable WDT time-out flag output

#define MODE_CMD  0x08
#define MODE_DATA 0x05

#define BATTERY_SEG_ADDR    0x80
#define SEPARATOR_SEG_ADDR  0x80

const char ascii[] =
{
/*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
/*      ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   '-'   ' '   ' ' */
/*2*/ 	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
/*      '0'   '1'   '2'   '3'   '4'   '5'   '6'   '7'   '8'   '9'   ' '   ' '   ' '   ' '   ' '   ' ' */
/*3*/   0x7D, 0x60, 0x3e, 0x7a, 0x63, 0x5b, 0x5f, 0x70, 0x7f, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*      ' '   'A'   'B'   'C'   'D'   'E'   'F'   'G'   'H'   'I'   'J'   'K'   'L'   'M'   'N'   'O' */
/*4*/ 	0x00, 0x77, 0x4f, 0x1d, 0x6e, 0x1f, 0x17, 0x5d, 0x47, 0x05, 0x68, 0x27, 0x0d, 0x54, 0x75, 0x4e,
/*      'P'   'Q'   'R'   'S'   'T'   'U'   'V'   'W'   'X'   'Y'   'Z'   ' '   ' '   ' '   ' '   '_' */
/*5*/ 	0x37, 0x73, 0x06, 0x59, 0x0f, 0x6d, 0x23, 0x29, 0x67, 0x6b, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00,
};

union tCmdSeq
{
    struct __attribute__((packed))
    {
#if (('1234' >> 24) == '1') // LITTLE_ENDIAN
        uint16_t padding : 4;
        uint16_t data : 8;
        uint16_t type : 4;
#elif (('4321' >> 24) == '1') // BIG_ENDIAN
        uint16_t type : 4;
        uint16_t data : 8;
        uint16_t padding : 4;
#endif
    };
    uint8_t arr[2];
};

union tDataSeq
{
    struct __attribute__((packed))
    {
#if (('1234' >> 24) == '1') // LITTLE_ENDIAN
        uint8_t padding : 7;
        uint16_t data2 : 16;
        uint16_t data1 : 16;
        uint16_t data0 : 16;
        uint8_t addr : 6;
        uint8_t type : 3;
#elif (('4321' >> 24) == '1') // BIG_ENDIAN
        uint8_t type : 3;
        uint8_t addr: 6;
        uint16_t data0 : 16;
        uint16_t data1 : 16;
        uint16_t data2 : 16;
        uint8_t padding : 7;
#endif
    };
    uint8_t arr[8];
};


// TODO: give wrapper example for GPIO toggle in README and in hpp
HT1621::HT1621(pPinSet *pCs, pPinSet *pSck, pPinSet *pMosi, pPinSet *pBacklight)
{
    pCsPin = pCs;
    pSckPin = pSck;
    pMosiPin = pMosi;
    pBacklightPin = pBacklight;

    wrCmd(BIAS);
    wrCmd(RC256);
    wrCmd(SYSDIS);
    wrCmd(WDTDIS1);
    wrCmd(SYSEN);
    wrCmd(LCDON);
}


void HT1621::backlightOn()
{
    if (pBacklightPin)
        pBacklightPin(HIGH);
}

void HT1621::backlightOff()
{
    if (pBacklightPin)
        pBacklightPin(LOW);
}

void HT1621::displayOn()
{
    wrCmd(LCDON);
}

void HT1621::displayOff()
{
    wrCmd(LCDOFF);
}

void HT1621::wrBits(uint8_t bitField, uint8_t cnt)
{
    if (!pSckPin || !pMosiPin)
        return;

    for (int i = 0; i < cnt; i++)
    {
        pSckPin(LOW);
        pMosiPin((bitField & 0x80)? HIGH : LOW);
        pSckPin(HIGH);
        bitField <<= 1;
    }
}

void HT1621::wrBuffer()
{
    if (!pCsPin)
        return;

    tDataSeq dataSeq = {};
    dataSeq.type = MODE_DATA;
    dataSeq.addr = 0;

    dataSeq.data0 = (_buffer[5] << 8) + _buffer[4];
    dataSeq.data1 = (_buffer[3] << 8) + _buffer[2];
    dataSeq.data2 = (_buffer[1] << 8) + _buffer[0];

    pCsPin(LOW);

    for (int i = 0; i < (int)sizeof(tDataSeq); i++)
    {
        wrBits(dataSeq.arr[7 - i], 8);
    }

    pCsPin(HIGH);
}

void HT1621::wrCmd(uint8_t cmd)
{
    if (!pCsPin)
        return;

    tCmdSeq CommandSeq = {};
    CommandSeq.type = MODE_CMD;
    CommandSeq.data = cmd;

    pCsPin(LOW);
    wrBits(CommandSeq.arr[1], 8);
    wrBits(CommandSeq.arr[0], 8);
    pCsPin(HIGH);
}

void HT1621::batteryLevel(tBatteryLevel level)
{
    batteryBufferClear();

    switch(level)
    {
        case 3: // battery on and all 3 segments
            _buffer[0] |= BATTERY_SEG_ADDR;
            // fall through
        case 2: // battery on and 2 segments
            _buffer[1] |= BATTERY_SEG_ADDR;
            // fall through
        case 1: // battery on and 1 segment
            _buffer[2] |= BATTERY_SEG_ADDR;
            break;
        case 0: // battery indication off
            break;
        default:
            break;
    }
    wrBuffer();
}

void HT1621::batteryBufferClear()
{
    _buffer[0] &= ~BATTERY_SEG_ADDR;
    _buffer[1] &= ~BATTERY_SEG_ADDR;
    _buffer[2] &= ~BATTERY_SEG_ADDR;
}

void HT1621::dotsBufferClear()
{
    _buffer[3] &= ~SEPARATOR_SEG_ADDR;
    _buffer[4] &= ~SEPARATOR_SEG_ADDR;
    _buffer[5] &= ~SEPARATOR_SEG_ADDR;
}

void HT1621::lettersBufferClear()
{
    for (int i = 0; i < DISPLAY_SIZE; i++)
    {
        _buffer[i] &= 0x80;
    }
}

void HT1621::clear()
{
    batteryBufferClear();
    dotsBufferClear();
    lettersBufferClear();

    wrBuffer();
}

void HT1621::print(const char *str)
{
    dotsBufferClear();
    lettersBufferClear();

    for (int i = 0; i < DISPLAY_SIZE; i++)
    {
        if (i >= (int)strlen(str))
        	// show letter is it exists
            _buffer[i] |= ascii[0];
        else
        	// when no letter - show space
            _buffer[i] |= ascii[str[i] - ' '];
    }

    wrBuffer();
}

void HT1621::print(int32_t num)
{
    if (num > MAX_NUM)
        num = MAX_NUM;
    if (num < MIN_NUM)
        num = MIN_NUM;

    dotsBufferClear();
    lettersBufferClear();

    char str[DISPLAY_SIZE + 1] = {};
    snprintf(str, sizeof(str), "%6li", num);

    for (int i = 0; i < DISPLAY_SIZE; i++)
    {
        _buffer[i] |= ascii[str[i] - ' '];
    }

    wrBuffer();
}

void HT1621::print(float num, uint8_t precision)
{
    if (num >= 0 && precision > MAX_POSITIVE_PRECISION)
        precision = MAX_POSITIVE_PRECISION;
    else if (num < 0 && precision > MAX_NEGATIVE_PRECISION)
        precision = MAX_NEGATIVE_PRECISION;

    int32_t integerated = (int32_t)(num * pow(10, precision));

    if (integerated > MAX_NUM)
        integerated = MAX_NUM;
    if (integerated < MIN_NUM)
        integerated = MIN_NUM;

    lettersBufferClear();
    print(integerated);
    decimalSeparator(precision);

    wrBuffer();
}

void HT1621::decimalSeparator(uint8_t dpPosition)
{
    dotsBufferClear();

    if (dpPosition == 0 || dpPosition > 3)
        return;

    // 3 is the digit offset
    // the first three eights bits in the buffer are for the battery signs
    // the last three are for the decimal point
    _buffer[DISPLAY_SIZE - dpPosition] |= SEPARATOR_SEG_ADDR;
}
