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
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

/**
 * @brief CALCULATION DEFINES BLOCK
 */
constexpr int MAX_NUM = 999999;
constexpr int MIN_NUM = -99999;

constexpr size_t PRECISION_MAX_POSITIVE = 3;  // TODO: find better names
constexpr size_t PRECISION_MAX_NEGATIVE = 2;
constexpr size_t PRECISION_MIN = 1;

constexpr size_t BITS_PER_BYTE = 8;

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

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

#define BATTERY_SEG 0x80
#define DOT_SEG     0x80

static const char ascii[] =
{
/*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
/*      ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   ' '   '-'   ' '   ' ' */
/*2*/   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
/*      '0'   '1'   '2'   '3'   '4'   '5'   '6'   '7'   '8'   '9'   ' '   ' '   ' '   ' '   ' '   ' ' */
/*3*/   0x7D, 0x60, 0x3e, 0x7a, 0x63, 0x5b, 0x5f, 0x70, 0x7f, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*      ' '   'A'   'B'   'C'   'D'   'E'   'F'   'G'   'H'   'I'   'J'   'K'   'L'   'M'   'N'   'O' */
/*4*/   0x00, 0x77, 0x4f, 0x1d, 0x6e, 0x1f, 0x17, 0x5d, 0x47, 0x05, 0x68, 0x27, 0x0d, 0x54, 0x75, 0x4e,
/*      'P'   'Q'   'R'   'S'   'T'   'U'   'V'   'W'   'X'   'Y'   'Z'   ' '   ' '   ' '   ' '   '_' */
/*5*/   0x37, 0x73, 0x06, 0x59, 0x0f, 0x6d, 0x23, 0x29, 0x67, 0x6b, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x08,
/*      ' '   'A'   'B'   'C'   'D'   'E'   'F'   'G'   'H'   'I'   'J'   'K'   'L'   'M'   'N'   'O' */
/*6*/   0x00, 0x77, 0x4f, 0x1d, 0x6e, 0x1f, 0x17, 0x5d, 0x47, 0x05, 0x68, 0x27, 0x0d, 0x54, 0x75, 0x4e,
/*      'P'   'Q'   'R'   'S'   'T'   'U'   'V'   'W'   'X'   'Y'   'Z'   ' '   ' '   ' '   ' '   ' ' */
/*7*/   0x37, 0x73, 0x06, 0x59, 0x0f, 0x6d, 0x23, 0x29, 0x67, 0x6b, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#define ASCII_SPACE_SYMBOL 0x00

#if (('1234' >> 24) == '1')
    #ifndef LITTLE_ENDIAN
    #define LITTLE_ENDIAN
    #endif
#elif (('4321' >> 24) == '1')
    #ifndef BIG_ENDIAN
    #define BIG_ENDIAN
    #endif
#else
    #error "Unable to determine endian. Set it manually"
#endif

union tCmdSeq
{
    struct __attribute__((packed))
    {
#if defined LITTLE_ENDIAN
        uint16_t padding : 4;
        uint16_t data : 8;
        uint16_t type : 4;
#elif defined BIG_ENDIAN
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
#if defined LITTLE_ENDIAN
        uint8_t padding : 7;
        uint16_t data2 : 16;
        uint16_t data1 : 16;
        uint16_t data0 : 16;
        uint8_t addr : 6;
        uint8_t type : 3;
#elif defined BIG_ENDIAN
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

HT1621::HT1621(pInterface *pSpi, pPinSet *pCs, pPinSet *pBacklight)
{
    pSpiInterface = pSpi;
    pCsPin = pCs;
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

void HT1621::wrBytes(uint8_t *ptr, uint8_t size)
{
    // probably need to use microsecond delays in this method
    // after every pin toggle function to give display
    // driver time for data reading. But current solution works for me

    // TODO: check wrong size
    // lcd driver expects data in reverse order
    std::reverse(ptr, ptr + size);

    if (pCsPin)
        pCsPin(LOW);

    if (pSpiInterface)
        pSpiInterface(ptr, size);
    else if (pSckPin && pMosiPin)
    {
        for (size_t k = 0; k < size; k++)
        {
            // send bits into display one by one
            for (size_t i = 0; i < BITS_PER_BYTE; i++)
            {
                pSckPin(LOW);
                pMosiPin((ptr[k] & (0x80 >> i))? HIGH : LOW);
                pSckPin(HIGH);
            }
        }
    }

    if (pCsPin)
        pCsPin(HIGH);
}

void HT1621::wrBuffer()
{
    tDataSeq dataSeq = {};
    dataSeq.type = MODE_DATA;
    dataSeq.addr = 0;
    // unable to use memcpy function with bit fields. So fill data manually
    dataSeq.data0 = (buffer[5] << 8) + buffer[4];
    dataSeq.data1 = (buffer[3] << 8) + buffer[2];
    dataSeq.data2 = (buffer[1] << 8) + buffer[0];

    wrBytes(dataSeq.arr, sizeof(tDataSeq));
}

void HT1621::wrCmd(uint8_t cmd)
{
    tCmdSeq CommandSeq = {};
    CommandSeq.type = MODE_CMD;
    CommandSeq.data = cmd;

    wrBytes(CommandSeq.arr, sizeof(tCmdSeq));
}

void HT1621::batteryLevel(uint8_t percents)
{
    batteryBufferClear();

    if (percents > 75)
    {
        buffer[0] |= BATTERY_SEG;
    }
    if (percents > 50)
    {
        buffer[1] |= BATTERY_SEG;
    }
    if (percents > 25)
    {
        buffer[2] |= BATTERY_SEG;
    }
    wrBuffer();
}

void HT1621::batteryBufferClear()
{
    buffer[0] &= ~BATTERY_SEG;
    buffer[1] &= ~BATTERY_SEG;
    buffer[2] &= ~BATTERY_SEG;
}

void HT1621::dotsBufferClear()
{
    buffer[3] &= ~DOT_SEG;
    buffer[4] &= ~DOT_SEG;
    buffer[5] &= ~DOT_SEG;
}

void HT1621::lettersBufferClear()
{
    for (size_t i = 0; i < DISPLAY_SIZE; i++)
    {
        buffer[i] &= BATTERY_SEG | DOT_SEG;
    }
}

void HT1621::clear()
{
    batteryBufferClear();
    dotsBufferClear();
    lettersBufferClear();

    wrBuffer();
}

void HT1621::bufferToAscii(const char *in, char *out)
{
    for (size_t i = 0; i < MIN(DISPLAY_SIZE, strlen(in)); i++)
    {
        char c = in[i];
        // Handle situation when char is out of displayable ascii table part.
        // Show space instead
        if (c < ' ')
            out[i] |= ASCII_SPACE_SYMBOL;
        else if (c - ' ' > (int)sizeof(ascii))
            out[i] |= ASCII_SPACE_SYMBOL;
        else
            out[i] |= ascii[c - ' '];
    }
}

void HT1621::print(const char *str)
{
    dotsBufferClear();
    lettersBufferClear();
    bufferToAscii(str, buffer);
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

    bufferToAscii(str, buffer);

    wrBuffer();
}

void HT1621::print(float num, uint8_t precision)
{
    if (num >= 0 && precision > PRECISION_MAX_POSITIVE)
        precision = PRECISION_MAX_POSITIVE;
    else if (num < 0 && precision > PRECISION_MAX_NEGATIVE)
        precision = PRECISION_MAX_NEGATIVE;

    int32_t integerated = (int32_t)(num * pow(10, precision));

    if (integerated > MAX_NUM)
        integerated = MAX_NUM;
    if (integerated < MIN_NUM)
        integerated = MIN_NUM;

    print(integerated);
    decimalSeparator(precision);

    wrBuffer();
}

// TODO: make multiplier more strict.
void HT1621::print(int32_t multiplied_float, uint32_t multiplier)
{
    uint8_t precision = 0;

    if (multiplier == 1000)
        precision = 3;
    else if (multiplier == 100)
    	precision = 2;
    else if (multiplier == 10)
    	precision = 1;

    if (multiplied_float > MAX_NUM)
        multiplied_float = MAX_NUM;
    if (multiplied_float < MIN_NUM)
        multiplied_float = MIN_NUM;

    print((int32_t)multiplied_float);
    decimalSeparator(precision);

    wrBuffer();
}

void HT1621::decimalSeparator(uint8_t dpPosition)
{
    dotsBufferClear();

    if (dpPosition < PRECISION_MIN || dpPosition > PRECISION_MAX_POSITIVE)
    	// selected dot position not supported by display hardware
        return;

    // the first three eights bits in the buffer are for the battery signs
    // the last three are for the decimal point
    buffer[DISPLAY_SIZE - dpPosition] |= DOT_SEG;
}
