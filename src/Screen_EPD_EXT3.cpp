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
// Copyright (c) Rei Vilo, 2010-2024
// Licence Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
// Portions (c) Pervasive Displays, 2010-2024
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
// Release 610: Removed partial update
// Release 700: Refactored screen and board functions
// Release 701: Improved functions names consistency
// Release 701: Added support for eScreen_EPD_EXT3_290_0F_Wide xE2290KS0Fx
// Release 702: Added support for eScreen_EPD_EXT3_206_0E_Wide xE2206KS0Ex
// Release 703: Added support for eScreen_EPD_EXT3_152_0J_Wide xE2152KS0Jx
// Release 800: Read OTP memory
// Release 801: Improved OTP implementation
//

// Library header
#include "Screen_EPD_EXT3.h"

#define DEBUG_OTP 1

//
// === COG section
//
/// @cond

void Screen_EPD_EXT3_Fast::COG_reset()
{
    // Application note § 2. Power on COG driver
    switch (u_codeSize)
    {
        case SIZE_565: // 5.65"
        case SIZE_581: // 5.81"
        case SIZE_741: // 7.41"

            b_reset(200, 20, 200, 50, 5); // medium
            break;

        case SIZE_969: // 9.69"
        case SIZE_1198: // 11.98"

            b_reset(200, 20, 200, 200, 5); // large
            break;

        default:

            b_reset(5, 5, 10, 5, 5); // small
            break;
    } // u_codeSize
}

void Screen_EPD_EXT3_Fast::COG_initial(uint8_t updateMode)
{
    // Application note § 4. Input initial command
    if ((u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        // Soft reset
        b_sendCommand8(0x12);
        digitalWrite(b_pin.panelDC, LOW); // Select
        b_waitBusy(LOW); // 150 and 152 specific

        // Work settings
        b_sendCommandData8(0x1a, u_temperature);

        if (updateMode == UPDATE_GLOBAL)
        {
            b_sendCommandData8(0x22, 0xd7);
        }
        else if (updateMode == UPDATE_FAST)
        {
            b_sendCommandData8(0x3c, 0xc0);
            b_sendCommandData8(0x22, 0xdf);
        }
    }
    else
    {
        // Work settings
        uint8_t indexE5_work; // Temperature
        uint8_t index00_work[2]; // PSR

        // FILM_P and FILM_K already checked
        if (updateMode != UPDATE_GLOBAL) // Specific settings for fast update
        {
            indexE5_work = u_temperature | 0x40; // temperature | 0x40
            index00_work[0] = COG_initialData[0] | 0x10; // PSR0 | 0x10
            index00_work[1] = COG_initialData[1] | 0x02; // PSR1 | 0x02
        }
        else // Common settings
        {
            indexE5_work = u_temperature; // Temperature
            index00_work[0] = COG_initialData[0]; // PSR0
            index00_work[1] = COG_initialData[1]; // PSR1
        } // u_codeExtra updateMode

        // New algorithm
        b_sendCommandData8(0x00, 0x0e); // Soft-reset
        b_waitBusy();

        b_sendCommandData8(0xe5, indexE5_work); // Input Temperature
        b_sendCommandData8(0xe0, 0x02); // Activate Temperature

        if (u_codeSize == SIZE_290) // No PSR
        {
            b_sendCommandData8(0x4d, 0x55);
            b_sendCommandData8(0xe9, 0x02);
        }
        else
        {
            b_sendIndexData(0x00, index00_work, 2); // PSR
        }

        // Specific settings for fast update, all screens
        // if (((u_codeFilm == FILM_P) or (u_codeFilm == FILM_K)) and (updateMode != UPDATE_GLOBAL))
        // FILM_P and FILM_K already checked
        if (updateMode != UPDATE_GLOBAL)
        {
            b_sendCommandData8(0x50, 0x07); // Vcom and data interval setting
        }

        // Additional settings for fast update, 154 213 266 and 370 screens (s_flag50)
        // if (((u_codeFilm == FILM_P) or (u_codeFilm == FILM_K)) and (updateMode != UPDATE_GLOBAL) and s_flag50)
        // FILM_P and FILM_K already checked
        if ((updateMode != UPDATE_GLOBAL) and s_flag50)
        {
            b_sendCommandData8(0x50, 0x27); // Vcom and data interval setting
        }
    }
}

void Screen_EPD_EXT3_Fast::COG_getDataOTP()
{
#if defined(ARDUINO_FEATHER_ESP32)

    hV_HAL_SPI3_define(SCK, MOSI) // SCK SDA

#elif defined(ARDUINO_XIAO_ESP32C3)

    // Board Xiao ESP32-C3 crashes if pins are not specified.
    hV_HAL_SPI3_define(8, 9) // SCK SDA

#elif defined(ARDUINO_NANO_ESP32)

    // Board Arduino Nano ESP32 arduino_nano_nora v2.0.11
    hV_HAL_SPI3_define(SCK, MOSI) // SCK SDA

#elif defined(ARDUINO_ARCH_ESP32)

    // void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
    // Board ESP32-Pico-DevKitM-2 crashes if pins are not specified.
    hV_HAL_SPI3_define(14, 12) // SCK SDA

#else

    hV_HAL_SPI3_define(SCK, MOSI); // SCK SDA

#endif // ARDUINO

    // Application note § 3. Read OTP memory
    // Register 0x50 flag
    // Additional settings for fast update, 154 206 213 266 271A 370 and 437 screens (s_flag50)
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_154_PS_0C:
        case eScreen_EPD_154_KS_0C:
        case eScreen_EPD_206_KS_0E:
        case eScreen_EPD_213_PS_0E:
        case eScreen_EPD_213_KS_0E:
        case eScreen_EPD_266_PS_0C:
        case eScreen_EPD_266_KS_0C:
        case eScreen_EPD_271_KS_0C: // 2.71(A)
        case eScreen_EPD_370_PS_0C:
        case eScreen_EPD_370_PS_0C_Touch:
        case eScreen_EPD_370_KS_0C:
        case eScreen_EPD_437_PS_0C:
        case eScreen_EPD_437_KS_0C:

            s_flag50 = true;
            break;

        default:

            s_flag50 = false;
            break;
    }

    // Screens with no OTP
    if ((u_eScreen_EPD == eScreen_EPD_290_KS_0F) or (u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        u_flagOTP = true;
        return; // No PSR
    }

    // GPIO
    COG_reset(); // Although not mentioned, reset to ensure stable state

    // Read OTP
    uint8_t ui8 = 0;
    uint16_t offsetA5 = 0x0000;
    uint16_t offsetPSR = 0x0000;
    uint16_t u_readBytes = 2;

    digitalWrite(b_pin.panelDC, LOW); // Command
    digitalWrite(b_pin.panelCS, LOW); // Select
    hV_HAL_SPI3_write(0xa2);
    digitalWrite(b_pin.panelCS, HIGH); // Unselect
    delay(10);

    digitalWrite(b_pin.panelDC, HIGH); // Data
    digitalWrite(b_pin.panelCS, LOW); // Select
    ui8 = hV_HAL_SPI3_read(); // Dummy
    digitalWrite(b_pin.panelCS, HIGH); // Unselect
    // hV_HAL_log(LEVEL_DEBUG, "Dummy read 0x%02x", ui8);

    digitalWrite(b_pin.panelCS, LOW); // Select
    ui8 = hV_HAL_SPI3_read(); // First byte to be checked
    digitalWrite(b_pin.panelCS, HIGH); // Unselect
    // hV_HAL_log(LEVEL_INFO, "ui8= 0x%02x", ui8);

    // Check bank
    uint8_t bank = ((ui8 == 0xa5) ? 0 : 1);

    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_271_KS_09:
        case eScreen_EPD_271_KS_09_Touch:

            offsetPSR = 0x004b;
            offsetA5 = 0x0000;

            if (bank > 0)
            {
                COG_initialData[0] = 0xcf;
                COG_initialData[1] = 0x82;
                return;
            }
            break;

        case eScreen_EPD_271_PS_09:
        case eScreen_EPD_271_PS_09_Touch:
        // case eScreen_EPD_287_KS_09:
        case eScreen_EPD_287_PS_09:

            offsetPSR = 0x004b;
            offsetA5 = 0x0000;

            if (bank > 0)
            {
                mySerial.println();
                mySerial.println(formatString("hV * OTP check failed - Bank %i, first 0x%02x, expected 0x%02x", bank, ui8, 0xa5));
                mySerial.flush();
                while (true);
            }
            break;

        case eScreen_EPD_154_KS_0C:
        case eScreen_EPD_154_PS_0C:
        case eScreen_EPD_266_KS_0C:
        case eScreen_EPD_266_PS_0C:
        case eScreen_EPD_271_KS_0C: // 2.71(A)
        case eScreen_EPD_370_KS_0C:
        case eScreen_EPD_370_PS_0C:
        case eScreen_EPD_370_PS_0C_Touch:
        case eScreen_EPD_437_PS_0C:

            offsetPSR = (bank == 0) ? 0x0fb4 : 0x1fb4;
            offsetA5 = (bank == 0) ? 0x0000 : 0x1000;

            break;

        case eScreen_EPD_206_KS_0E:
        case eScreen_EPD_213_KS_0E:
        case eScreen_EPD_213_PS_0E:

            offsetPSR = (bank == 0) ? 0x0b1b : 0x171b;
            offsetA5 = (bank == 0) ? 0x0000 : 0x0c00;
            break;

        case eScreen_EPD_417_PS_0D:
        case eScreen_EPD_417_KS_0D:

            offsetPSR = (bank == 0) ? 0x0b1f : 0x171f;
            offsetA5 = (bank == 0) ? 0x0000 : 0x0c00;
            break;

        default:

            mySerial.println(formatString("hV * OTP check failed - Screen %i-%cS-0%c not supported", u_codeSize, u_codeFilm, u_codeDriver));
            mySerial.flush();
            while (true);
            break;
    }

    // Check second bank
    if (offsetA5 > 0x0000)
    {
        for (uint16_t index = 1; index < offsetA5; index += 1)
        {
            digitalWrite(b_pin.panelCS, LOW); // Select
            ui8 = hV_HAL_SPI3_read();
            digitalWrite(b_pin.panelCS, HIGH); // Unselect
        }

        digitalWrite(b_pin.panelCS, LOW); // Select
        ui8 = hV_HAL_SPI3_read(); // First byte to be checked
        digitalWrite(b_pin.panelCS, HIGH); // Unselect

        if (ui8 != 0xa5)
        {
            mySerial.println();
            mySerial.println(formatString("hV * OTP check failed - Bank %i, first 0x%02x, expected 0x%02x", bank, ui8, 0xa5));
            mySerial.flush();
            while (true);
        }
    }

    mySerial.println(formatString("hV . OTP check passed - Bank %i, first 0x%02x as expected", bank, ui8));

    // Ignore bytes 1..offsetPSR
    for (uint16_t index = offsetA5 + 1; index < offsetPSR; index += 1)
    {
        digitalWrite(b_pin.panelCS, LOW); // Select
        ui8 = hV_HAL_SPI3_read();
        digitalWrite(b_pin.panelCS, HIGH); // Unselect
    }

    // Populate COG_initialData
    for (uint16_t index = 0; index < u_readBytes; index += 1)
    {
        digitalWrite(b_pin.panelCS, LOW); // Select
        ui8 = hV_HAL_SPI3_read(); // Read OTP
        COG_initialData[index] = ui8;
        digitalWrite(b_pin.panelCS, HIGH); // Unselect
    }

    u_flagOTP = true;

    // Debug COG_initialData
#if (DEBUG_OTP == 1)
    uint8_t debugIndex = u_readBytes;

    mySerial.print(formatString("const uint8_t COG_initialData[%i] =", debugIndex));
    mySerial.println();
    mySerial.print("{");
    for (uint16_t index = 0; index < debugIndex; index += 1)
    {
        if ((index % 8) == 0)
        {
            mySerial.println();
            mySerial.print("   ");
        }

        mySerial.print(formatString("0x%02x", COG_initialData[index]));
        mySerial.print(formatString("%s ", (index + 1 < debugIndex ? "," : " "))); // no comma on last value

        if (((index + 1) % 8) == 0)
        {
            mySerial.print(formatString(" // 0x%04x..%04x", index - 7, index));
        }
    }
    mySerial.println();

    mySerial.print(formatString("} // %i", debugIndex));
    mySerial.println();
#endif // DEBUG_OTP
}

void Screen_EPD_EXT3_Fast::COG_sendImageDataFast()
{
    // Application note § 5. Input image to the EPD
    uint8_t * nextBuffer = u_newImage;
    uint8_t * previousBuffer = u_newImage + u_pageColourSize;

    if ((u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        b_sendIndexData(0x24, previousBuffer, u_frameSize); // Previous frame
        b_sendIndexData(0x26, nextBuffer, u_frameSize); // Next frame
    }
    else
    {
        b_sendIndexData(0x10, previousBuffer, u_frameSize); // Previous frame
        b_sendIndexData(0x13, nextBuffer, u_frameSize); // Next frame
    }
    memcpy(previousBuffer, nextBuffer, u_frameSize); // Copy displayed next to previous
}

void Screen_EPD_EXT3_Fast::COG_update(uint8_t updateMode)
{
    // Application note § 6. Send updating command
    if ((u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        b_waitBusy(LOW); // 152 specific
        b_sendCommand8(0x20); // Display Refresh
        digitalWrite(b_pin.panelCS, HIGH); // CS# = 1
        b_waitBusy(LOW); // 152 specific
    }
    else
    {
        // Specific settings for fast update, 154 213 266 and 370 screens (s_flag50)
        // FILM_P and FILM_K already checked
        if ((updateMode != UPDATE_GLOBAL) and s_flag50)

        {
            b_sendCommandData8(0x50, 0x07); // Vcom and data interval setting
        }

        b_sendCommand8(0x04); // Power on
        digitalWrite(b_pin.panelCS, HIGH); // CS# = 1
        b_waitBusy();

        b_sendCommand8(0x12); // Display Refresh
        digitalWrite(b_pin.panelCS, HIGH); // CS# = 1
        b_waitBusy();
    }
}

void Screen_EPD_EXT3_Fast::COG_powerOff()
{
    // Application note § 7. Turn-off DC/DC
    if ((u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        // Empty
    }
    else
    {
        b_sendCommand8(0x02); // Turn off DC/DC
        digitalWrite(b_pin.panelCS, HIGH); // CS# = 1
        b_waitBusy();
    }
}
/// @endcond
//
// === End of COG section
//

//
// === Class section
//
Screen_EPD_EXT3_Fast::Screen_EPD_EXT3_Fast(eScreen_EPD_t eScreen_EPD_EXT3, pins_t board)
{
    u_eScreen_EPD = eScreen_EPD_EXT3;
    b_pin = board;
    u_newImage = 0; // nullptr
}

void Screen_EPD_EXT3_Fast::begin()
{
    u_eScreen_EPD = u_eScreen_EPD;
    u_codeSize = SCREEN_SIZE(u_eScreen_EPD);
    u_codeFilm = SCREEN_FILM(u_eScreen_EPD);
    u_codeDriver = SCREEN_DRIVER(u_eScreen_EPD);
    u_codeExtra = SCREEN_EXTRA(u_eScreen_EPD);
    v_screenColourBits = 2; // BWR and BWRY

    // Checks
    switch (u_codeFilm)
    {
        case FILM_P: // BWR, fast update
        case FILM_K: // BWR, fast update and wide temperature

            break;

        default:

            mySerial.println();
            mySerial.println(formatString("hV * Screen %i-%cS-0%c with no fast update", u_codeSize, u_codeFilm, u_codeDriver));
            while (0x01);
            break;
    }

    // Configure board
    switch (u_codeSize)
    {
        case SIZE_581: // 5.81"
        case SIZE_741: // 7.41"

            b_begin(b_pin, FAMILY_MEDIUM, 0);
            break;

        case SIZE_969: // 9.69"
        case SIZE_1198: // 11.98"

            b_begin(b_pin, FAMILY_LARGE, 50);
            break;

        default:

            b_begin(b_pin, FAMILY_SMALL, 0);
            break;
    }

    // Sizes
    switch (u_codeSize)
    {
        case SIZE_150: // 1.50"
        case SIZE_152: // 1.52"

            v_screenSizeV = 200; // vertical = wide size
            v_screenSizeH = 200; // horizontal = small size
            break;

        case SIZE_154: // 1.54"

            v_screenSizeV = 152; // vertical = wide size
            v_screenSizeH = 152; // horizontal = small size
            break;

        case SIZE_206: // 2.06"

            v_screenSizeV = 248; // vertical = wide size
            v_screenSizeH = 128; // horizontal = small size
            break;

        case SIZE_213: // 2.13"

            v_screenSizeV = 212; // vertical = wide size
            v_screenSizeH = 104; // horizontal = small size
            break;

        case SIZE_266: // 2.66"

            v_screenSizeV = 296; // vertical = wide size
            v_screenSizeH = 152; // horizontal = small size
            break;

        case SIZE_271: // 2.71" and 2.71"-Touch

            v_screenSizeV = 264; // vertical = wide size
            v_screenSizeH = 176; // horizontal = small size
            break;

        case SIZE_287: // 2.87"

            v_screenSizeV = 296; // vertical = wide size
            v_screenSizeH = 128; // horizontal = small size
            break;

        case SIZE_290: // 2.90"

            v_screenSizeV = 384; // vertical = wide size
            v_screenSizeH = 168; // horizontal = small size
            break;

        case SIZE_370: // 3.70" and 3.70"-Touch

            v_screenSizeV = 416; // vertical = wide size
            v_screenSizeH = 240; // horizontal = small size
            break;

        case SIZE_417: // 4.17"

            v_screenSizeV = 300; // vertical = wide size
            v_screenSizeH = 400; // horizontal = small size
            break;

        case SIZE_437: // 4.37"

            v_screenSizeV = 480; // vertical = wide size
            v_screenSizeH = 176; // horizontal = small size
            break;

        // Those screens are not available with embedded fast update
        //         case SIZE_565: // 5.65"
        //
        //             v_screenSizeV = 600; // v = wide size
        //             v_screenSizeH = 448; // h = small size
        //             break;
        //
        //         case SIZE_581: // 5.81"
        //
        //             v_screenSizeV = 720; // v = wide size
        //             v_screenSizeH = 256; // h = small size
        //             break;
        //
        //         case SIZE_741: // 7.41"
        //
        //             v_screenSizeV = 800; // v = wide size
        //             v_screenSizeH = 480; // h = small size
        //             break;
        //
        //         case SIZE_969: // 9.69"
        //
        //             v_screenSizeV = 672; // v = wide size
        //             v_screenSizeH = 960; // Actually, 960 = 480 x 2, h = small size
        //             break;
        //
        //         case SIZE_1198: // 11.98"
        //
        //             v_screenSizeV = 768; // v = wide size
        //             v_screenSizeH = 960; // Actually, 960 = 480 x 2, h = small size
        //             break;

        default:

            mySerial.println();
            mySerial.println(formatString("hV * Screen %i-%cS-0%c is not supported", u_codeSize, u_codeFilm, u_codeDriver));
            while (0x01);
            break;
    } // u_codeSize
    v_screenDiagonal = u_codeSize;

    // Report
    mySerial.println(formatString("hV = Screen %s %ix%i", WhoAmI().c_str(), screenSizeX(), screenSizeY()));
    mySerial.println(formatString("hV = Number %i-%cS-0%c", u_codeSize, u_codeFilm, u_codeDriver));
    mySerial.println(formatString("hV = PDLS %s v%i.%i.%i", SCREEN_EPD_EXT3_VARIANT, SCREEN_EPD_EXT3_RELEASE / 100, (SCREEN_EPD_EXT3_RELEASE / 10) % 10, SCREEN_EPD_EXT3_RELEASE % 10));
    mySerial.println();

    u_bufferDepth = v_screenColourBits; // 2 colours
    u_bufferSizeV = v_screenSizeV; // vertical = wide size
    u_bufferSizeH = v_screenSizeH / 8; // horizontal = small size 112 / 8, 1 bit per pixel

    // Force conversion for two unit16_t multiplication into uint32_t.
    // Actually for 1 colour; BWR requires 2 pages.
    u_pageColourSize = (uint32_t)u_bufferSizeV * (uint32_t)u_bufferSizeH;

    // u_frameSize = u_pageColourSize, except for 9.69 and 11.98
    // 9.69 and 11.98 combine two half-screens, hence two frames with adjusted size
    switch (u_codeSize)
    {
        case SIZE_969: // 9.69"
        case SIZE_1198: // 11.98"

            u_frameSize = u_pageColourSize / 2;
            break;

        default:

            u_frameSize = u_pageColourSize;
            break;
    } // u_codeSize

#if defined(BOARD_HAS_PSRAM) // ESP32 PSRAM specific case

    if (u_newImage == 0)
    {
        static uint8_t * _newFrameBuffer;
        _newFrameBuffer = (uint8_t *) ps_malloc(u_pageColourSize * u_bufferDepth);
        u_newImage = (uint8_t *) _newFrameBuffer;
    }

#else // default case

    if (u_newImage == 0)
    {
        static uint8_t * _newFrameBuffer;
        _newFrameBuffer = new uint8_t[u_pageColourSize * u_bufferDepth];
        u_newImage = (uint8_t *) _newFrameBuffer;
    }

#endif // ESP32 BOARD_HAS_PSRAM

    memset(u_newImage, 0x00, u_pageColourSize * u_bufferDepth);

    // Turn SPI on, initialise GPIOs and set GPIO levels
    // Reset panel and get tables
    resume();

    // Fonts
    hV_Screen_Buffer::begin(); // Standard
    if (f_fontMax() > 0)
    {
        f_selectFont(0);
    }
    f_fontSolid = false;

    // Orientation
    setOrientation(0);

    v_penSolid = false;
    u_invert = false;

    setTemperatureC(25); // 25 Celsius = 77 Fahrenheit
}

String Screen_EPD_EXT3_Fast::WhoAmI()
{
    char work[64] = {0};
    u_WhoAmI(work);

    return formatString("iTC %i.%02i\"%s", v_screenDiagonal / 100, v_screenDiagonal % 100, work);
}

void Screen_EPD_EXT3_Fast::resume()
{
    b_resume();

    // Check type and get tables
    if (u_flagOTP == false)
    {
        hV_HAL_SPI3_begin(); // Define 3-wire SPI pins
        COG_getDataOTP(); // 3-wire SPI read OTP memory
    }

    // Reset
    COG_reset();

    // Check after reset
    if ((u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        if (digitalRead(b_pin.panelBusy) == HIGH)
        {
            mySerial.println();
            mySerial.println("hV * Incorrect type for 1.52-Wide");
            while(0x01);
        }
    }

    // Start SPI and Wire
    if ((u_eScreen_EPD == eScreen_EPD_150_KS_0J) or (u_eScreen_EPD == eScreen_EPD_152_KS_0J))
    {
        hV_HAL_SPI_begin(16000000); // 1.52" tested with 4, 8, 16 and 32 MHz
    }
    else
    {
        hV_HAL_SPI_begin(); // Standard 8 MHz
    }
    // hV_HAL_Wire_begin();
}

uint8_t Screen_EPD_EXT3_Fast::flushMode(uint8_t updateMode)
{
    updateMode = checkTemperatureMode(updateMode);

    switch (updateMode)
    {
        case UPDATE_FAST:
        case UPDATE_GLOBAL:

            s_flushFast();
            break;

        default:

            mySerial.println();
            mySerial.println("hV ! PDLS - UPDATE_NONE invoked");
            break;
    }

    return updateMode;
}

void Screen_EPD_EXT3_Fast::flush()
{
    flushMode(UPDATE_FAST);
}

void Screen_EPD_EXT3_Fast::s_flushFast()
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
        // black = 0-1, white = 0-0
        for (uint16_t i = 0; i < u_bufferSizeV; i++)
        {
            uint8_t pattern = (i % 2) ? 0b10101010 : 0b01010101;
            for (uint16_t j = 0; j < u_bufferSizeH; j++)
            {
                u_newImage[i * u_bufferSizeH + j] = pattern;
            }
        }
    }
    else if ((colour == myColours.white) xor u_invert)
    {
        // physical black 0-0
        memset(u_newImage, 0x00, u_pageColourSize);
    }
    else
    {
        // physical white 1-0
        memset(u_newImage, 0xff, u_pageColourSize);
    }
}

void Screen_EPD_EXT3_Fast::regenerate(uint8_t mode)
{
    clear(myColours.black);
    flush();
    delay(100);

    clear(myColours.white);
    flush();
    delay(100);
}

void Screen_EPD_EXT3_Fast::s_setPoint(uint16_t x1, uint16_t y1, uint16_t colour)
{
    // Orient and check coordinates are within screen
    if (s_orientCoordinates(x1, y1) == RESULT_ERROR)
    {
        return;
    }

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

    // Coordinates
    uint32_t z1 = s_getZ(x1, y1);
    uint16_t b1 = s_getB(x1, y1);

    // Basic colours
    if ((colour == myColours.white) xor u_invert)
    {
        // physical black 0-0
        bitClear(u_newImage[z1], b1);
    }
    else if ((colour == myColours.black) xor u_invert)
    {
        // physical white 1-0
        bitSet(u_newImage[z1], b1);
    }
}

void Screen_EPD_EXT3_Fast::s_setOrientation(uint8_t orientation)
{
    v_orientation = orientation % 4;
}

bool Screen_EPD_EXT3_Fast::s_orientCoordinates(uint16_t & x, uint16_t & y)
{
    bool _flagResult = RESULT_ERROR; // false = success, true = error
    switch (v_orientation)
    {
        case 3: // checked, previously 1

            if ((x < v_screenSizeV) and (y < v_screenSizeH))
            {
                x = v_screenSizeV - 1 - x;
                _flagResult = RESULT_SUCCESS;
            }
            break;

        case 2: // checked

            if ((x < v_screenSizeH) and (y < v_screenSizeV))
            {
                x = v_screenSizeH - 1 - x;
                y = v_screenSizeV - 1 - y;
                swap(x, y);
                _flagResult = RESULT_SUCCESS;
            }
            break;

        case 1: // checked, previously 3

            if ((x < v_screenSizeV) and (y < v_screenSizeH))
            {
                y = v_screenSizeH - 1 - y;
                _flagResult = RESULT_SUCCESS;
            }
            break;

        default: // checked

            if ((x < v_screenSizeH) and (y < v_screenSizeV))
            {
                swap(x, y);
                _flagResult = RESULT_SUCCESS;
            }
            break;
    }

    return _flagResult;
}

uint32_t Screen_EPD_EXT3_Fast::s_getZ(uint16_t x1, uint16_t y1)
{
    uint32_t z1 = 0;
    // According to 11.98 inch Spectra Application Note
    // at http://www.pervasivedisplays.com/LiteratureRetrieve.aspx?ID=245146
    switch (u_codeSize)
    {
        case SIZE_969:
        case SIZE_1198:

            if (y1 >= (v_screenSizeH >> 1))
            {
                y1 -= (v_screenSizeH >> 1); // rebase y1
                z1 += (u_pageColourSize >> 1); // buffer second half
            }
            z1 += (uint32_t)x1 * (u_bufferSizeH >> 1) + (y1 >> 3);
            break;

        default:

            z1 = (uint32_t)x1 * u_bufferSizeH + (y1 >> 3);
            break;
    }
    return z1;
}

uint16_t Screen_EPD_EXT3_Fast::s_getB(uint16_t x1, uint16_t y1)
{
    uint16_t b1 = 0;

    b1 = 7 - (y1 % 8);

    return b1;
}

uint16_t Screen_EPD_EXT3_Fast::s_getPoint(uint16_t x1, uint16_t y1)
{
    // Orient and check coordinates are within screen
    if (s_orientCoordinates(x1, y1) == RESULT_ERROR)
    {
        return 0x0000;
    }

    uint16_t _result = 0;
    uint8_t _value = 0;

    // Coordinates
    uint32_t z1 = s_getZ(x1, y1);
    uint16_t b1 = s_getB(x1, y1);

    _value = bitRead(u_newImage[z1], b1);
    _value <<= 4;
    _value &= 0b11110000;

    // red = 0-1, black = 1-0, white 0-0
    switch (_value)
    {
        case 0x10:

            _result = myColours.black;
            break;

        default:

            _result = myColours.white;
            break;
    }

    return _result;
}
//
// === End of Class section
//

//
// === Touch section
//

//
// === End of Touch section
//

