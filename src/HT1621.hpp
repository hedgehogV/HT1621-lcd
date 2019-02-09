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

#ifndef  HT1621_H_
#define HT1621_H_

#include "stdint.h"


class HT1621
{
public:

    using pPinSet = void(bool);

    /**
     * @brief Construct a new HT1621 object
     *
     * Starts the lcd with the pin assignement declared. The backlight pin is optional
     *
     * @param pCs - pointer to CS pin toggle function
     * @param pSck - pointer to SCK pin toggle function
     * @param pMosi - pointer to MOSI pin toggle function
     * @param pBacklight - pointer to backlight pin toggle function. Optional
     */
    HT1621(pPinSet *pCs, pPinSet *pSck, pPinSet *pMosi, pPinSet *pBacklight = nullptr);

    /**
     * @brief Turns on the display (doesn't affect the backlight)
     */
    void displayOn();

    /**
     * @brief Turns off the display (doesn't affect the backlight)
     */
    void displayOff();

    /**
     * @brief Turns on the backlight
     */
    void backlightOn();

    /**
     * @brief Turns off the backlight
     */
    void backlightOff();

    enum tBatteryLevel
    {
        BATTERY_NONE,
        BATTERY_LOW,
        BATTERY_MEDIUM,
        BATTERY_FULL,
    };

    /**
     * @brief Show battery level.
     *
     * @param level - battery charge state. May be NONE, LOW, MEDIUM or HIGH
     */
    void batteryLevel(tBatteryLevel level);

    /**
     * @brief Print string (up to 6 characters)
     *
     * @param str String to be displayed. Allowed: capital letters, digits, space, minus
     * Not allowed symbols will be displayed as spaces. See symbols appearence in README.md
     */
    void print(const char *str);

    /**
     * @brief Prints a signed integer between -99999 and 999999.
     * Larger and smaller values will be displayed as -99999 and 999999
     *
     * @param num - number to be printed
     */
    void print(int32_t num);

    /**
     * @brief Prints a float with 0 to 3 decimals, based on the
     * `precision` parameter. Default value is 3
     *
     * @param num  - number to be printed
     * @param precision - precision of the number
     */
    void print(float num, uint8_t precision = 3);

    /**
     * @brief Clears the display
     */
    void clear();

private:

    static const uint8_t DISPLAY_SIZE = 6; // symbols on display
    char _buffer[DISPLAY_SIZE] = {}; // buffer where display data will be stored

    // defines to set display pin to low or high level
    const bool LOW = 0;
    const bool HIGH = 1;

    pPinSet *pCsPin = nullptr; // SPI CS pin
    pPinSet *pSckPin = nullptr; // for display it is WR pin
    pPinSet *pMosiPin = nullptr; // for display it is Data pin
    pPinSet *pBacklightPin = nullptr; // display backlight pin

    // the most low-level function. Just put one bit into display
    void wrBits(uint8_t bitField, uint8_t cnt);
    // write data byte to the display at specified addr
    void wrByte(uint8_t addr, uint8_t byte);
    // write command sequance to display
    void wrCmd(uint8_t cmd);
    // set decimal separator. Used when print float numbers
    void decimalSeparator(uint8_t dpPosition);
    // takes the buffer and puts it straight into the driver
    void update();
    // remove battery symbol from display buffer
    void batteryBufferClear();
    // remove dot symbol from display buffer
    void dotsBufferClear();
    // remove all symbols from display buffer except battery and dots
    void lettersBufferClear();
};

#endif
