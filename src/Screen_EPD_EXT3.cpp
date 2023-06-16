//
// Screen_EPD_EXT3.cpp
// Library C++ code
// ----------------------------------
//
// Project Pervasive Displays Library Suite
// Based on highView technology
//
// Created by Rei Vilo, 28 Jun 2016
//
// Copyright (c) Rei Vilo, 2010-2023
// Licence Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
//
// Release 509: Added eScreen_EPD_EXT3_271_Fast
// Release 527: Added support for ESP32 PSRAM
// Release 541: Improved support for ESP32
// Release 550: Tested Xiao ESP32-C3 with SPI exception
// Release 601: Added support for screens with embedded fast update
// Release 602: Improved functions structure
// Release 604: Improved stability
// Release 607: Improved screens names consistency
// Release 608: Added screen report
// Release 609: Added temperature management
//

// Library header
#include "SPI.h"
#include "Screen_EPD_EXT3.h"

#if defined(ENERGIA)
///
/// @brief Proxy for SPISettings
/// @details Not implemented in Energia
/// @see https://www.arduino.cc/en/Reference/SPISettings
///
struct _SPISettings_s
{
    uint32_t clock; ///< in Hz, checked against SPI_CLOCK_MAX = 16000000
    uint8_t bitOrder; ///< LSBFIRST, MSBFIRST
    uint8_t dataMode; ///< SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3
};
///
/// @brief SPI settings for screen
///
_SPISettings_s _settingScreen;
#else
///
/// @brief SPI settings for screen
///
SPISettings _settingScreen;
#endif // ENERGIA

#ifndef SPI_CLOCK_MAX
#define SPI_CLOCK_MAX 16000000
#endif

//
// === COG section
//
/// @cond

// Common settings
// 0x00, soft-reset, temperature, active temperature, PSR0, PSR1
uint8_t indexE5_data[] = {0x19}; // Temperature 0x19 = 25 °C
uint8_t indexE0_data[] = {0x02}; // Activate temperature
uint8_t index00_data[] = {0xff, 0x8f}; // PSR, constant
uint8_t index50a_data[] = {0x27}; // Only 154 213 266 and 370 screens, constant
uint8_t index50b_data[] = {0x07}; // Only 154 213 266 and 370 screens, constant
uint8_t index50c_data[] = {0x07}; // All screens, constant

void Screen_EPD_EXT3_Fast::COG_initial(uint8_t updateMode)
{
    // Work settings
    uint8_t indexE0_work[1]; // Activate temperature
    uint8_t indexE5_work[1]; // Temperature
    uint8_t index00_work[2]; // PSR

    indexE0_work[0] = indexE0_data[0];
    if ((_codeExtra & FEATURE_FAST) and (updateMode != UPDATE_GLOBAL)) // Specific settings for fast update
    {
        indexE5_work[0] = indexE5_data[0] | 0x40; // temperature | 0x40
        index00_work[0] = index00_data[0] | 0x10; // PSR0 | 0x10
        index00_work[1] = index00_data[1] | 0x02; // PSR1 | 0x02
    }
    else // Common settings
    {
        indexE5_work[0] = indexE5_data[0]; // Temperature
        index00_work[0] = index00_data[0]; // PSR0
        index00_work[1] = index00_data[1]; // PSR1
    } // _codeExtra updateMode

    // New algorithm
    uint8_t index00_reset[] = {0x0e};
    _sendIndexData(0x00, index00_reset, 1); // Soft-reset
    _waitBusy();

    _sendIndexData(0xe5, indexE5_work, 1); // Input Temperature: 25C
    _sendIndexData(0xe0, indexE0_work, 1); // Activate Temperature
    _sendIndexData(0x00, index00_work, 2); // PSR

    // Specific settings for fast update, all screens
    if ((_codeExtra & FEATURE_FAST) and (updateMode != UPDATE_GLOBAL))
    {
        uint8_t index50c_work[1]; // Vcom
        index50c_work[0] = index50c_data[0]; // 0x07
        _sendIndexData(0x50, index50c_work, 1); // Vcom and data interval setting
    }

    // Additional settings for fast update, 154 213 266 and 370 screens (_flag50)
    if ((_codeExtra & FEATURE_FAST) and (updateMode != UPDATE_GLOBAL) and _flag50)
    {
        uint8_t index50a_work[1]; // Vcom
        index50a_work[0] = index50a_data[0]; // 0x27
        _sendIndexData(0x50, index50a_work, 1); // Vcom and data interval setting
    }
}

void Screen_EPD_EXT3_Fast::COG_getUserData()
{
    uint16_t _codeSizeType = _eScreen_EPD_EXT3 & 0xffff;

    // Size cSize cType Driver
    switch (_codeSizeType)
    {
        case 0x150C: // 1.54” = 0xcf, 0x02
        case 0x210E: // 2.13” = 0xcf, 0x02
        case 0x260C: // 2.66” = 0xcf, 0x02

            index00_data[0] = 0xcf;
            index00_data[1] = 0x02;
            _flag50 = true;
            break;

        case 0x2709: // 2.71” = 0xcf, 0x8d

            index00_data[0] = 0xcf;
            index00_data[1] = 0x8d;
            _flag50 = false;
            break;

        case 0x2809: // 2.87” = 0xcf, 0x8d

            index00_data[0] = 0xcf;
            index00_data[1] = 0x8d;
            _flag50 = false;
            break;

        case 0x370C: // 3.70” = 0xcf, 0x0f

            index00_data[0] = 0xcf;
            index00_data[1] = 0x8f;
            _flag50 = true;
            break;

        case 0x410D:// 4.17” = 0x0f, 0x0e

            index00_data[0] = 0x0f;
            index00_data[1] = 0x0e;
            _flag50 = false;
            break;

        case 0x430C: // 4.37” = 0x0f, 0x0e

            index00_data[0] = 0x0f;
            index00_data[1] = 0x0e;
            _flag50 = true;
            break;

        case 0x580B:

            break;

        default:

            break;
    }
}

void Screen_EPD_EXT3_Fast::COG_sendImageDataFast()
{
    uint8_t * nextBuffer = _newImage;
    uint8_t * previousBuffer = _newImage + _pageColourSize;

    _sendIndexData(0x10, previousBuffer, _frameSize); // Previous frame
    _sendIndexData(0x13, nextBuffer, _frameSize); // Next frame
    memcpy(previousBuffer, nextBuffer, _frameSize); // Copy displayed next to previous
}

void Screen_EPD_EXT3_Fast::COG_update(uint8_t updateMode)
{
    // Specific settings for fast update, 154 213 266 and 370 screens (_flag50)
    if ((_codeExtra & FEATURE_FAST) and (updateMode != UPDATE_GLOBAL) and _flag50)
    {
        uint8_t index50b_work[1]; // Vcom
        index50b_work[0] = index50b_data[0]; // 0x07
        _sendIndexData(0x50, index50b_work, 1); // Vcom and data interval setting
    }

    _sendCommand8(0x04); // Power on
    digitalWrite(_pin.panelCS, HIGH); // CS# = 1
    _waitBusy();

    _sendCommand8(0x12); // Display Refresh
    digitalWrite(_pin.panelCS, HIGH); // CS# = 1
    _waitBusy();
}

void Screen_EPD_EXT3_Fast::COG_powerOff()
{
    _sendCommand8(0x02); // Turn off DC/DC
    digitalWrite(_pin.panelCS, HIGH); // CS# = 1
    _waitBusy();
}
/// @endcond
//
// === End of COG section
//

// Utilities

// Class
Screen_EPD_EXT3_Fast::Screen_EPD_EXT3_Fast(eScreen_EPD_EXT3_t eScreen_EPD_EXT3, pins_t board)
{
    _eScreen_EPD_EXT3 = eScreen_EPD_EXT3;
    _pin = board;
    _newImage = 0; // nullptr
}

void Screen_EPD_EXT3_Fast::begin()
{
    _codeExtra = (_eScreen_EPD_EXT3 >> 16) & 0xff;
    _codeSize = (_eScreen_EPD_EXT3 >> 8) & 0xff;
    _codeType = _eScreen_EPD_EXT3 & 0xff;
    _screenColourBits = 2; // BWR

    switch (_codeSize)
    {
        case 0x15: // 1.54"

            _screenSizeV = 152; // vertical = wide size
            _screenSizeH = 152; // horizontal = small size
            _screenDiagonal = 154;
            break;

        case 0x21: // 2.13"

            _screenSizeV = 212; // vertical = wide size
            _screenSizeH = 104; // horizontal = small size
            _screenDiagonal = 213;
            break;

        case 0x26: // 2.66"

            _screenSizeV = 296; // vertical = wide size
            _screenSizeH = 152; // horizontal = small size
            _screenDiagonal = 266;
            break;

        case 0x27: // 2.71" and 2.71"-Touch

            _screenSizeV = 264; // vertical = wide size
            _screenSizeH = 176; // horizontal = small size
            _screenDiagonal = 271;
            break;

        case 0x28: // 2.87"

            _screenSizeV = 296; // vertical = wide size
            _screenSizeH = 128; // horizontal = small size
            _screenDiagonal = 287;
            break;

        case 0x37: // 3.70" and 3.70"-Touch

            _screenSizeV = 416; // vertical = wide size
            _screenSizeH = 240; // horizontal = small size
            _screenDiagonal = 370;
            break;

        case 0x41: // 4.17"

            _screenSizeV = 300; // vertical = wide size
            _screenSizeH = 400; // horizontal = small size
            _screenDiagonal = 417;
            break;

        case 0x43: // 4.37"

            _screenSizeV = 480; // vertical = wide size
            _screenSizeH = 176; // horizontal = small size
            _screenDiagonal = 437;
            break;

        case 0x56: // 5.65"

            _screenSizeV = 600; // v = wide size
            _screenSizeH = 448; // h = small size
            _screenDiagonal = 565;
            break;

        case 0x58: // 5.81"

            _screenSizeV = 720; // v = wide size
            _screenSizeH = 256; // h = small size
            _screenDiagonal = 581;
            break;

        case 0x74: // 7.40"

            _screenSizeV = 800; // v = wide size
            _screenSizeH = 480; // h = small size
            _screenDiagonal = 741;
            break;

        default:

            break;
    } // _codeSize

    _bufferDepth = _screenColourBits; // 2 colours
    _bufferSizeV = _screenSizeV; // vertical = wide size
    _bufferSizeH = _screenSizeH / 8; // horizontal = small size 112 / 8;

    // Force conversion for two unit16_t multiplication into uint32_t.
    // Actually for 1 colour; BWR requires 2 pages.
    _pageColourSize = (uint32_t)_bufferSizeV * (uint32_t)_bufferSizeH;

    // _frameSize = _pageColourSize, except for 9.69 and 11.98
    // 9.69 and 11.98 combine two half-screens, hence two frames with adjusted size
    switch (_codeSize)
    {
        case 0x96: // 9.69"
        case 0xB9: // 11.98"

            _frameSize = _pageColourSize / 2;
            break;

        default:

            _frameSize = _pageColourSize;
            break;
    } // _codeSize

#if defined(BOARD_HAS_PSRAM) // ESP32 PSRAM specific case

    if (_newImage == 0)
    {
        static uint8_t * _newFrameBuffer;
        _newFrameBuffer = (uint8_t *) ps_malloc(_pageColourSize * _bufferDepth);
        _newImage = (uint8_t *) _newFrameBuffer;
    }

#else // default case

    if (_newImage == 0)
    {
        static uint8_t * _newFrameBuffer;
        _newFrameBuffer = new uint8_t[_pageColourSize * _bufferDepth];
        _newImage = (uint8_t *) _newFrameBuffer;
    }

#endif // ESP32 BOARD_HAS_PSRAM

    memset(_newImage, 0x00, _pageColourSize * _bufferDepth);

    // Initialise the /CS pins
    pinMode(_pin.panelCS, OUTPUT);
    digitalWrite(_pin.panelCS, HIGH); // CS# = 1

    // New generic solution
    pinMode(_pin.panelDC, OUTPUT);
    pinMode(_pin.panelReset, OUTPUT);
    pinMode(_pin.panelBusy, INPUT); // All Pins 0

    // Initialise Flash /CS as HIGH
    if (_pin.flashCS != NOT_CONNECTED)
    {
        pinMode(_pin.flashCS, OUTPUT);
        digitalWrite(_pin.flashCS, HIGH);
    }

    // Initialise slave panel /CS as HIGH
    if (_pin.panelCSS != NOT_CONNECTED)
    {
        pinMode(_pin.panelCSS, OUTPUT);
        digitalWrite(_pin.panelCSS, HIGH);
    }

    // Initialise slave Flash /CS as HIGH
    if (_pin.flashCSS != NOT_CONNECTED)
    {
        pinMode(_pin.flashCSS, OUTPUT);
        digitalWrite(_pin.flashCSS, HIGH);
    }

    // Initialise SD-card /CS as HIGH
    if (_pin.cardCS != NOT_CONNECTED)
    {
        pinMode(_pin.cardCS, OUTPUT);
        digitalWrite(_pin.cardCS, HIGH);
    }

    // Initialise SPI
    _settingScreen = {4000000, MSBFIRST, SPI_MODE0};

#if defined(ENERGIA)

    SPI.begin();
    SPI.setBitOrder(_settingScreen.bitOrder);
    SPI.setDataMode(_settingScreen.dataMode);
    SPI.setClockDivider(SPI_CLOCK_MAX / min(SPI_CLOCK_MAX, _settingScreen.clock));

#else

#if defined(ARDUINO_XIAO_ESP32C3)

    // Board Xiao ESP32-C3 crashes if pins are specified.
    SPI.begin(8, 9, 10); // SCK MISO MOSI

#elif defined(ARDUINO_ARCH_ESP32)

    // Board ESP32-Pico-DevKitM-2 crashes if pins are not specified.
    SPI.begin(14, 12, 13); // SCK MISO MOSI

#else

    SPI.begin();

#endif // ARDUINO_ARCH_ESP32

    SPI.beginTransaction(_settingScreen);

#endif // ENERGIA

    // Reset
    switch (_codeSize)
    {
        case 0x56: // 5.65"
        case 0x58: // 5.81"
        case 0x74: // 7.40"

            _reset(200, 20, 200, 50, 5); // medium
            break;

        case 0x96: // 9.69"
        case 0xB9: // 11.98"

            _reset(200, 20, 200, 200, 5); // large
            break;

        default:

            _reset(5, 5, 10, 5, 5); // small
            break;
    } // _codeSize

    // Check screen and get tables
    COG_getUserData(); // nothing sent to panel

    _screenWidth = _screenSizeH;
    _screenHeigth = _screenSizeV;

    // Standard
    hV_Screen_Buffer::begin();

    setOrientation(0);
    if (_f_fontMax() > 0)
    {
        _f_selectFont(0);
    }
    _f_fontSolid = false;

    _penSolid = false;
    _invert = false;

    // Report
    Serial.println(formatString("= Screen %s %ix%i", WhoAmI(), screenSizeX(), screenSizeY()));
    Serial.println(formatString("= PDLS v%i", SCREEN_EPD_EXT3_RELEASE));

    clear();
}

void Screen_EPD_EXT3_Fast::_reset(uint32_t ms1, uint32_t ms2, uint32_t ms3, uint32_t ms4, uint32_t ms5)
{
    delay_ms(ms1); // delay_ms 5ms
    digitalWrite(_pin.panelReset, HIGH); // RES# = 1
    delay_ms(ms2); // delay_ms 5ms
    digitalWrite(_pin.panelReset, LOW);
    delay_ms(ms3);
    digitalWrite(_pin.panelReset, HIGH);
    delay_ms(ms4);
    digitalWrite(_pin.panelCS, HIGH); // CS# = 1
    delay_ms(ms5);
}

String Screen_EPD_EXT3_Fast::WhoAmI()
{
    String text = "iTC ";
    text += String(_screenDiagonal / 100);
    text += ".";
    text += String(_screenDiagonal % 100);
    text += "\" -";

#if (FONT_MODE == USE_FONT_HEADER)

    text += "H";

#elif (FONT_MODE == USE_FONT_FLASH)

    text += "F";

#elif (FONT_MODE == USE_FONT_TERMINAL)

    text += "T";

#else

    text += "?";

#endif // FONT_MODE

    if (_codeExtra > 0)
    {
        if (_codeExtra & FEATURE_FAST)
        {
            text +=  "F";
        }
        if (_codeExtra & FEATURE_TOUCH)
        {
            text +=  "T";
        }
        if (_codeExtra & FEATURE_OTHER)
        {
            text +=  "A";
        }
        if (_codeExtra & FEATURE_WIDE_TEMPERATURE)
        {
            text +=  "W";
        }
        if (_codeExtra & FEATURE_RED)
        {
            text +=  "R";
        }
    }

    return text;
}

void Screen_EPD_EXT3_Fast::flush()
{
    flushMode(UPDATE_FAST);
}

void Screen_EPD_EXT3_Fast::_flushFast()
{
    // Configure
    COG_initial(UPDATE_FAST);

    // Send image data
    COG_sendImageDataFast();

    // Update
    COG_update(UPDATE_FAST);
    COG_powerOff();
}

void Screen_EPD_EXT3_Fast::clear(uint16_t colour)
{
    if (colour == myColours.grey)
    {
        for (uint16_t i = 0; i < _bufferSizeV; i++)
        {
            uint16_t pattern = (i % 2) ? 0b10101010 : 0b01010101;
            for (uint16_t j = 0; j < _bufferSizeH; j++)
            {
                _newImage[i * _bufferSizeH + j] = pattern;
            }
        }
    }
    else if ((colour == myColours.white) xor _invert)
    {
        // physical black 00
        memset(_newImage, 0x00, _pageColourSize);
    }
    else
    {
        // physical white 10
        memset(_newImage, 0xff, _pageColourSize);
    }
}

void Screen_EPD_EXT3_Fast::regenerate()
{
    clear(myColours.black);
    flush();
    delay(100);

    clear(myColours.white);
    flush();
    delay(100);
}

void Screen_EPD_EXT3_Fast::invert(bool flag)
{
    _invert = flag;
}

void Screen_EPD_EXT3_Fast::_setPoint(uint16_t x1, uint16_t y1, uint16_t colour)
{
    // Orient and check coordinates are within screen
    // _orientCoordinates() returns false = success, true = error
    if (_orientCoordinates(x1, y1))
    {
        return;
    }

    uint32_t z1 = _getZ(x1, y1);

    // Convert combined colours into basic colours
    bool flagOdd = ((x1 + y1) % 2 == 0);

    if (colour == myColours.grey)
    {
        if (flagOdd)
        {
            colour = myColours.black; // black
        }
        else
        {
            colour = myColours.white; // white
        }
    }

    // Basic colours
    if ((colour == myColours.white) xor _invert)
    {
        // physical black 00
        bitClear(_newImage[z1], 7 - (y1 % 8));
    }
    else if ((colour == myColours.black) xor _invert)
    {
        // physical white 10
        bitSet(_newImage[z1], 7 - (y1 % 8));
    }
}

void Screen_EPD_EXT3_Fast::_setOrientation(uint8_t orientation)
{
    _orientation = orientation % 4;
}

bool Screen_EPD_EXT3_Fast::_orientCoordinates(uint16_t & x, uint16_t & y)
{
    bool flagError = true; // false = success, true = error
    switch (_orientation)
    {
        case 3: // checked, previously 1

            if ((x < _screenSizeV) and (y < _screenSizeH))
            {
                x = _screenSizeV - 1 - x;
                flagError = false;
            }
            break;

        case 2: // checked

            if ((x < _screenSizeH) and (y < _screenSizeV))
            {
                x = _screenSizeH - 1 - x;
                y = _screenSizeV - 1 - y;
                swap(x, y);
                flagError = false;
            }
            break;

        case 1: // checked, previously 3

            if ((x < _screenSizeV) and (y < _screenSizeH))
            {
                y = _screenSizeH - 1 - y;
                flagError = false;
            }
            break;

        default: // checked

            if ((x < _screenSizeH) and (y < _screenSizeV))
            {
                swap(x, y);
                flagError = false;
            }
            break;
    }

    return flagError;
}

uint32_t Screen_EPD_EXT3_Fast::_getZ(uint16_t x1, uint16_t y1)
{
    uint32_t z1 = 0;
    // According to 11.98 inch Spectra Application Note
    // at http:// www.pervasivedisplays.com/LiteratureRetrieve.aspx?ID=245146

    z1 = (uint32_t)x1 * _bufferSizeH + (y1 >> 3);

    return z1;
}

uint16_t Screen_EPD_EXT3_Fast::_getPoint(uint16_t x1, uint16_t y1)
{
    // Orient and check coordinates are within screen
    // _orientCoordinates() returns false = success, true = error
    if (_orientCoordinates(x1, y1))
    {
        return 0;
    }

    uint16_t result = 0;
    uint8_t value = 0;

    uint32_t z1 = _getZ(x1, y1);

    value = bitRead(_newImage[z1], 7 - (y1 % 8));
    value <<= 4;
    value &= 0b11110000;

    // red = 0-1, black = 1-0, white 0-0
    switch (value)
    {
        case 0x10:

            result = myColours.black;
            break;

        default:

            result = myColours.white;
            break;
    }

    return result;
}

void Screen_EPD_EXT3_Fast::point(uint16_t x1, uint16_t y1, uint16_t colour)
{
    _setPoint(x1, y1, colour);
}

uint16_t Screen_EPD_EXT3_Fast::readPixel(uint16_t x1, uint16_t y1)
{
    return _getPoint(x1, y1);
}

// Utilities
void Screen_EPD_EXT3_Fast::_sendCommand8(uint8_t command)
{
    digitalWrite(_pin.panelDC, LOW);
    digitalWrite(_pin.panelCS, LOW);

    SPI.transfer(command);

    digitalWrite(_pin.panelCS, HIGH);
}

void Screen_EPD_EXT3_Fast::_waitBusy()
{
    // LOW = busy, HIGH = ready
    while (digitalRead(_pin.panelBusy) != HIGH)
    {
        delay(32); // non-blocking
    }
}

void Screen_EPD_EXT3_Fast::_sendIndexData(uint8_t index, const uint8_t * data, uint32_t size)
{
    digitalWrite(_pin.panelDC, LOW); // DC Low = Command
    digitalWrite(_pin.panelCS, LOW); // CS Low = Select

    delayMicroseconds(50);
    SPI.transfer(index);
    delayMicroseconds(50);

    digitalWrite(_pin.panelCS, HIGH); // CS High = Unselect
    digitalWrite(_pin.panelDC, HIGH); // DC High = Data
    digitalWrite(_pin.panelCS, LOW); // CS Low = Select

    delayMicroseconds(50);
    for (uint32_t i = 0; i < size; i++)
    {
        SPI.transfer(data[i]);
    }
    delayMicroseconds(50);

    digitalWrite(_pin.panelCS, HIGH); // CS High = Unselect
}

//
// === Temperature section
//
void Screen_EPD_EXT3_Fast::setTemperatureC(int8_t temperatureC)
{
    _temperature = temperatureC;

    uint8_t _temperature2;
    if (_temperature < 0)
    {
        _temperature2 = -_temperature;
        _temperature2 = (uint8_t)(~_temperature2) + 1; // 2's complement
    }
    else
    {
        _temperature2 = _temperature;
    }
    indexE5_data[0] = _temperature2;
}

void Screen_EPD_EXT3_Fast::setTemperatureF(int16_t temperatureF)
{
    int8_t temperatureC = ((temperatureF - 32) * 5) / 9; // C = (F - 32) * 5 / 9
    setTemperatureC(temperatureC);
}

uint8_t Screen_EPD_EXT3_Fast::checkTemperatureMode(uint8_t updateMode)
{
    // #define FEATURE_FAST 0x01 ///< With embedded fast update
    // #define FEATURE_TOUCH 0x02 ///< With capacitive touch panel
    // #define FEATURE_OTHER 0x04 ///< With other feature
    // #define FEATURE_WIDE_TEMPERATURE 0x08 ///< With wide operating temperature
    // #define FEATURE_RED 0x10 ///< With red colour
    // #define FEATURE_RED_YELLOW 0x20 ///< With red and yellow colours

    updateMode = UPDATE_FAST;

    switch (_codeExtra & 0x19)
    {
        case FEATURE_FAST: // PS series

            // Fast 	PS 	Embedded fast update 	FU: +15 to +30 °C 	GU: 0 to +50 °C
            if ((_temperature < 15) or (_temperature > 30))
            {
                updateMode = UPDATE_NONE;
            }
            break;

        case (FEATURE_FAST | FEATURE_WIDE_TEMPERATURE): // KS series

            // Wide 	KS 	Wide temperature and embedded fast update 	FU: 0 to +50 °C 	GU: -15 to +60 °C
            if ((_temperature < -15) or (_temperature > 60))
            {
                updateMode = UPDATE_NONE;
            }
            break;

        default: // CS series

            // Normal 	CS 	Global update above 0 °C 	FU: - 	GU: 0 to +50 °C
            updateMode = UPDATE_NONE;
            break;
    }

    return updateMode;
}

uint8_t Screen_EPD_EXT3_Fast::flushMode(uint8_t updateMode)
{
    updateMode = checkTemperatureMode(updateMode);

    switch (updateMode)
    {
        case UPDATE_FAST:
        case UPDATE_PARTIAL:
        case UPDATE_GLOBAL:

            _flushFast();
            break;

        default:

            Serial.println("* PDLS - UPDATE_NONE invoked");
            break;
    }

    return updateMode;
}
//
// === End of Temperature section
//

