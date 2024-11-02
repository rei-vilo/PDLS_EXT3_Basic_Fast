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
// For exclusive use with Pervasive Displays screens
// Portions (c) Pervasive Displays, 2010-2024
//
// Release 509: Added support for 271_PS_09
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
// Release 701: Added support for 290_KS_0F
// Release 702: Added support for 206_KS_0E
// Release 703: Added support for 152_KS_0J
// Release 800: Read OTP memory
// Release 801: Improved OTP implementation
// Release 802: Added support for 343_KS_0B
// Release 802: Added references to application notes
// Release 802: Refactored CoG functions
// Release 803: Added types for string and frame-buffer
// Release 804: Improved power management
// Release 805: Improved stability
// Release 806: New library for Wide temperature only
//

// Library header
#include "Screen_EPD_EXT3.h"

//
// === COG section
//
/// @cond
/// @see
/// * ApplicationNote_smallSize_fast-update_v02_20220907
/// * ApplicationNote_Small_Size_wide-Temperature_EPD_v03_20231031_B
/// * ApplicationNote_Small_Size_wide-Temperature_EPD_v01_20231225_A
/// * ApplicationNote_EPD343_Mono(E2343PS0Bx)_240320a
/// * ApplicationNote_for_5.8inch_fast-update_EPDE2581PS0B1_20230206b
/// * ApplicationNote_152_Size_wide-Temperature_EPD_v01_20231225_A
//

//
// --- Medium screens with P film
//
void Screen_EPD_EXT3_Fast::COG_MediumP_reset()
{
    // Application note § 2. Power on COG driver
    b_reset(5, 2, 4, 20, 5); // Medium
}

void Screen_EPD_EXT3_Fast::COG_MediumP_getDataOTP()
{
    // Read OTP
    uint16_t _readBytes = 0;
    uint8_t ui8 = 0; // dummy
    u_flagOTP = false;

    COG_MediumP_reset();
    if (b_family == FAMILY_LARGE)
    {
        digitalWrite(b_pin.panelCSS, HIGH); // Unselect slave panel
    }

    // Read OTP
    switch (u_codeDriver)
    {
        case DRIVER_B:

            _readBytes = 128;

            digitalWrite(b_pin.panelDC, LOW); // Command
            digitalWrite(b_pin.panelCS, LOW); // Select
            hV_HAL_SPI3_write(0xb9);
            delay(5);
            break;

        default:

            mySerial.println();
            mySerial.println(formatString("hV * OTP failed for screen %i-%cS-0%c", u_codeSize, u_codeFilm, u_codeDriver));
            while (0x01);
            break;
    }

    digitalWrite(b_pin.panelDC, HIGH); // Data
    ui8 = hV_HAL_SPI3_read(); // Dummy

    // Populate COG_data
    for (uint16_t index = 0; index < _readBytes; index += 1)
    {
        COG_data[index] = hV_HAL_SPI3_read(); // Read OTP
    }

    // End of OTP reading
    digitalWrite(b_pin.panelCS, HIGH); // Unselect

    // Check
    uint8_t _chipId;
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_343_PS_0B:
        case eScreen_EPD_343_PS_0B_Touch:
        case eScreen_EPD_581_PS_0B:

            _chipId = 0x10;
            u_flagOTP = (COG_data[0x00] == _chipId);
            break;

        case eScreen_EPD_581_KS_0B:

            _chipId = 0x16;
            u_flagOTP = (COG_data[0x00] == _chipId);
            break;

        default:

            _chipId = 0x00;
            u_flagOTP = false;
            break;
    }

    if (u_flagOTP == true)
    {
        mySerial.println("hV . OTP check passed");
    }
    else
    {
        mySerial.println();
        mySerial.println(formatString("hV * OTP check failed - First byte 0x%02x, expected 0x%04x", COG_data[0x00], _chipId));
        while (0x01);
    }
}

void Screen_EPD_EXT3_Fast::COG_MediumP_initial(uint8_t updateMode)
{
    uint8_t workDCTL[2];
    workDCTL[0] = COG_data[0x10]; // DCTL
    workDCTL[1] = 0x00;

    // FILM_P already checked
    b_sendIndexData(0x01, workDCTL, 2); // Fast
}

void Screen_EPD_EXT3_Fast::COG_MediumP_sendImageData(uint8_t updateMode)
{
    // Application note § 3.2 Input image to the EPD
    FRAMEBUFFER_TYPE nextBuffer = s_newImage;
    FRAMEBUFFER_TYPE previousBuffer = s_newImage + u_pageColourSize;

    // Send image data
    b_sendIndexData(0x13, &COG_data[0x15], 6); // DUW
    b_sendIndexData(0x90, &COG_data[0x0c], 4); // DRFW

    // Next frame
    b_sendIndexData(0x12, &COG_data[0x12], 3); // RAM_RW
    b_sendIndexData(0x10, nextBuffer, u_pageColourSize); // Next frame

    b_sendIndexData(0x12, &COG_data[0x12], 3); // RAM_RW
    switch (updateMode)
    {
        case UPDATE_GLOBAL:

            // Previous frame = dummy
            b_sendIndexFixed(0x11, 0x00, u_pageColourSize); // Previous frame = dummy

            break;

        case UPDATE_FAST:

            // Previous frame
            b_sendIndexData(0x11, previousBuffer, u_pageColourSize); // Next frame
            break;

        default:

            break;
    }

    // Copy next frame to previous frame
    memcpy(previousBuffer, nextBuffer, u_pageColourSize); // Copy displayed next to previous
}

void Screen_EPD_EXT3_Fast::COG_MediumP_update(uint8_t updateMode)
{
    // Initial COG
    // Application note § 3.1 Initial flow chart
    b_sendCommandData8(0x05, 0x7d);
    delay(50);
    b_sendCommandData8(0x05, 0x00);
    delay(1);
    b_sendCommandData8(0xd8, COG_data[0x1c]); // MS_SYNC
    b_sendCommandData8(0xd6, COG_data[0x1d]); // BVSS

    b_sendCommandData8(0xa7, 0x10);
    delay(2);
    b_sendCommandData8(0xa7, 0x00);
    delay(10);

    b_sendCommandData8(0x44, 0x00);
    b_sendCommandData8(0x45, 0x80);

    b_sendCommandData8(0xa7, 0x10);
    delay(2);
    b_sendCommandData8(0xa7, 0x00);
    delay(10);

    uint8_t indexTemperature;
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_343_PS_0B:
        case eScreen_EPD_343_PS_0B_Touch:

            switch (updateMode)
            {
                case UPDATE_FAST:

                    indexTemperature = (u_temperature < 22) ? 0xc9 : 0xca;
                    break;

                case UPDATE_GLOBAL:

                    indexTemperature = 2 * u_temperature + 0x50; // Temperature 0x82@25C
                    // indexTemperature = (u_temperature > 50) ? 0xb4 : indexTemperature;
                    indexTemperature = checkRange(indexTemperature, (uint8_t)0x50, (uint8_t)0xb4);
                    break;

                default:

                    break;
            }

        case eScreen_EPD_581_PS_0B:
        case eScreen_EPD_741_PS_0B:

            switch (updateMode)
            {
                case UPDATE_FAST:

                    indexTemperature = (u_temperature + 0x28) + 0x80;
                    // indexTemperature = (u_temperature > 50) ? 0xda : indexTemperature;
                    // indexTemperature = (u_temperature < 0) ? 0xa8 : indexTemperature;
                    indexTemperature = checkRange(indexTemperature, (uint8_t)0xa8, (uint8_t)0xda);
                    break;

                case UPDATE_GLOBAL:

                    indexTemperature = u_temperature + 0x28; // Temperature 0x41@25C
                    // indexTemperature = (u_temperature > 60) ? 0x64 : indexTemperature;
                    // indexTemperature = (u_temperature < -15) ? 0x19 : indexTemperature;
                    indexTemperature = checkRange(indexTemperature, (uint8_t)0x19, (uint8_t)0x64);
                    break;

                default:

                    break;
            }
            break;

        default:

            break;
    }

    b_sendCommandData8(0x44, 0x06);
    b_sendCommandData8(0x45, indexTemperature);

    b_sendCommandData8(0xa7, 0x10);
    delay(2);
    b_sendCommandData8(0xa7, 0x00);
    delay(10);

    b_sendCommandData8(0x60, COG_data[0x0b]); // TCON
    b_sendCommandData8(0x61, COG_data[0x1b]); // STV_DIR
    // No DCTL here
    b_sendCommandData8(0x02, COG_data[0x11]); // VCOM
    // Fast: no VCOM_CTRL here for Fast

    // DC/DC Soft-start
    // Application note § 3.3 DC/DC soft-start
    // DRIVER_B = 0x28, DRIVER_8 = 0x20
    uint8_t offsetFrame = 0x28;

    // Filter for register 0x09
    uint8_t _filter09 = 0xff;

    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_343_PS_0B:
        case eScreen_EPD_343_PS_0B_Touch:

            _filter09 = 0xfb;
            break;

        default:

            _filter09 = 0xff;
            break;
    }

    for (uint8_t stage = 0; stage < 4; stage += 1)
    {
        uint8_t offset = offsetFrame + 0x08 * stage;
        uint8_t FORMAT = COG_data[offset] & 0x80;
        uint8_t REPEAT = COG_data[offset] & 0x7f;

        if (FORMAT > 0) // Format 1
        {
            uint8_t PHL_PHH[2];
            PHL_PHH[0] = COG_data[offset + 1]; // PHL_INI
            PHL_PHH[1] = COG_data[offset + 2]; // PHH_INI
            uint8_t PHL_VAR = COG_data[offset + 3];
            uint8_t PHH_VAR = COG_data[offset + 4];
            uint8_t BST_SW_a = COG_data[offset + 5] & _filter09;
            uint8_t BST_SW_b = COG_data[offset + 6] & _filter09;
            uint8_t DELAY_SCALE = COG_data[offset + 7] & 0x80;
            uint16_t DELAY_VALUE = COG_data[offset + 7] & 0x7f;

            for (uint8_t i = 0; i < REPEAT; i += 1)
            {
                b_sendCommandData8(0x09, BST_SW_a);
                PHL_PHH[0] += PHL_VAR; // PHL
                PHL_PHH[1] += PHH_VAR; // PHH
                b_sendIndexData(0x51, PHL_PHH, 2);
                b_sendCommandData8(0x09, BST_SW_b);

                if (DELAY_SCALE > 0)
                {
                    delay(DELAY_VALUE); // ms
                }
                else
                {
                    delayMicroseconds(10 * DELAY_VALUE); //10 us
                }
            }
        }
        else // Format 2
        {
            uint8_t BST_SW_a = COG_data[offset + 1] & _filter09;
            uint8_t BST_SW_b = COG_data[offset + 2] & _filter09;
            uint8_t DELAY_a_SCALE = COG_data[offset + 3] & 0x80;
            uint16_t DELAY_a_VALUE = COG_data[offset + 3] & 0x7f;
            uint8_t DELAY_b_SCALE = COG_data[offset + 4] & 0x80;
            uint16_t DELAY_b_VALUE = COG_data[offset + 4] & 0x7f;

            for (uint8_t i = 0; i < REPEAT; i += 1)
            {
                b_sendCommandData8(0x09, BST_SW_a);

                if (DELAY_a_SCALE > 0)
                {
                    delay(DELAY_a_VALUE); // ms
                }
                else
                {
                    delayMicroseconds(10 * DELAY_a_VALUE); // 10 us
                }

                b_sendCommandData8(0x09, BST_SW_b);

                if (DELAY_b_SCALE > 0)
                {
                    delay(DELAY_b_VALUE); // ms
                }
                else
                {
                    delayMicroseconds(10 * DELAY_b_VALUE); // 10 us
                }
            }
        }
    }

    // Display Refresh Start
    // Application note § 4 Send updating command
    b_waitBusy();
    b_sendCommandData8(0x15, 0x3c);
}

void Screen_EPD_EXT3_Fast::COG_MediumP_powerOff()
{
    // Application note § 5. Turn-off DC/DC

    // DC-DC off
    b_waitBusy();

    // FILM_P already checked
    b_sendCommandData8(0x09, 0x7b);
    b_sendCommandData8(0x05, 0x5d);
    b_sendCommandData8(0x09, 0x7a);
    hV_HAL_delayMilliseconds(15);
    b_sendCommandData8(0x09, 0x00);

    b_waitBusy(HIGH); // added
}
//
// --- End of Medium screens with P film
//

//
// --- Small screens with P film
//
void Screen_EPD_EXT3_Fast::COG_SmallP_reset()
{
    // Application note § 2. Power on COG driver
    b_reset(5, 5, 10, 5, 5); // Small

    // Check after reset
    // No check
}

void Screen_EPD_EXT3_Fast::COG_SmallP_getDataOTP()
{
    // Read OTP
    uint8_t ui8 = 0;
    uint16_t _readBytes = 0;
    u_flagOTP = false;

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
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_150_KS_0J:
        case eScreen_EPD_152_KS_0J:
        case eScreen_EPD_290_KS_0F:

            u_flagOTP = true;
            mySerial.println("hV . OTP check passed - embedded PSR");
            return; // No PSR
            break;

        default:

            break;
    }

    // GPIO
    // COG_SmallP_reset(); // Although not mentioned, reset to ensure stable state

    // Read OTP
    _readBytes = 2;
    ui8 = 0;

    uint16_t offsetA5 = 0x0000;
    uint16_t offsetPSR = 0x0000;

    digitalWrite(b_pin.panelDC, LOW); // Command
    digitalWrite(b_pin.panelCS, LOW); // CS low = Select
    hV_HAL_SPI3_write(0xa2);
    digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect
    delay(10);

    digitalWrite(b_pin.panelDC, HIGH); // Data
    digitalWrite(b_pin.panelCS, LOW); // CS low = Select
    ui8 = hV_HAL_SPI3_read(); // Dummy
    digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect
    // mySerial.println(formatString("hV . Dummy read 0x%02x", ui8));

    digitalWrite(b_pin.panelCS, LOW); // CS low = Select
    ui8 = hV_HAL_SPI3_read(); // First byte to be checked
    digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect
    // mySerial.println(formatString("hV . ui 0x%02x", ui8));

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
                COG_data[0] = 0xcf;
                COG_data[1] = 0x82;
                return;
            }
            break;

        case eScreen_EPD_271_PS_09:
        // case eScreen_EPD_271_KS_09_Touch:
        case eScreen_EPD_271_PS_09_Touch:
        case eScreen_EPD_287_PS_09:

            offsetPSR = 0x004b;
            offsetA5 = 0x0000;

            break;

        case eScreen_EPD_154_KS_0C:
        case eScreen_EPD_154_PS_0C:
        case eScreen_EPD_266_KS_0C:
        case eScreen_EPD_266_PS_0C:
        case eScreen_EPD_271_KS_0C: // 2.71(A)
        case eScreen_EPD_350_KS_0C:
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
            digitalWrite(b_pin.panelCS, LOW); // CS low = Select
            ui8 = hV_HAL_SPI3_read();
            digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect
        }

        digitalWrite(b_pin.panelCS, LOW); // CS low = Select
        ui8 = hV_HAL_SPI3_read(); // First byte to be checked
        digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect

        if (ui8 != 0xa5)
        {
            mySerial.println();
            mySerial.println(formatString("hV * OTP check failed - Bank %i, first 0x%02x, expected 0x%02x", bank, ui8, 0xa5));
            mySerial.flush();
            while (0x01);
        }
    }

    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_271_KS_09:
        case eScreen_EPD_271_PS_09:
        case eScreen_EPD_271_PS_09_Touch:
        // case eScreen_EPD_287_KS_09:
        case eScreen_EPD_287_PS_09:

            mySerial.println(formatString("hV . OTP check passed - Bank %i, first 0x%02x %s", bank, ui8, (bank == 0) ? "as expected" : "not checked"));
            break;

        default:

            mySerial.println(formatString("hV . OTP check passed - Bank %i, first 0x%02x as expected", bank, ui8));
            break;
    }

    // Ignore bytes 1..offsetPSR
    for (uint16_t index = offsetA5 + 1; index < offsetPSR; index += 1)
    {
        digitalWrite(b_pin.panelCS, LOW); // CS low = Select
        ui8 = hV_HAL_SPI3_read();
        digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect
    }

    // Populate COG_data
    for (uint16_t index = 0; index < _readBytes; index += 1)
    {
        digitalWrite(b_pin.panelCS, LOW); // CS low = Select
        ui8 = hV_HAL_SPI3_read(); // Read OTP
        COG_data[index] = ui8;
        digitalWrite(b_pin.panelCS, HIGH); // CS high = Unselect
    }

    u_flagOTP = true;
}

void Screen_EPD_EXT3_Fast::COG_SmallP_initial(uint8_t updateMode)
{
    // Application note § 4. Input initial command
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_150_KS_0J:
        case eScreen_EPD_152_KS_0J:

            // Soft reset
            b_sendCommand8(0x12);
            digitalWrite(b_pin.panelDC, LOW);
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
            break;

        default:

            // Work settings
            uint8_t indexTemperature; // Temperature
            uint8_t index00_work[2]; // PSR

            // FILM_P already checked
            if (updateMode != UPDATE_GLOBAL) // Specific settings for fast update
            {
                indexTemperature = u_temperature | 0x40; // temperature | 0x40
                index00_work[0] = COG_data[0] | 0x10; // PSR0 | 0x10
                index00_work[1] = COG_data[1] | 0x02; // PSR1 | 0x02
            }
            else // Common settings
            {
                indexTemperature = u_temperature; // Temperature
                index00_work[0] = COG_data[0]; // PSR0
                index00_work[1] = COG_data[1]; // PSR1
            } // u_codeExtra updateMode

            // New algorithm
            b_sendCommandData8(0x00, 0x0e); // Soft-reset
            b_waitBusy();

            b_sendCommandData8(0xe5, indexTemperature); // Input Temperature
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
            // FILM_P already checked
            if (updateMode != UPDATE_GLOBAL)
            {
                b_sendCommandData8(0x50, 0x07); // Vcom and data interval setting
            }
            break;
    }
}

void Screen_EPD_EXT3_Fast::COG_SmallP_sendImageData(uint8_t updateMode)
{
    // Application note § 5. Input image to the EPD
    FRAMEBUFFER_TYPE nextBuffer = s_newImage;
    FRAMEBUFFER_TYPE previousBuffer = s_newImage + u_pageColourSize;

    // Send image data
    // Additional settings for fast update, 154 213 266 370 and 437 screens (s_flag50)
    if (s_flag50)
    {
        b_sendCommandData8(0x50, 0x27); // Vcom and data interval setting
    }

    b_sendIndexData(0x10, previousBuffer, u_pageColourSize); // First frame, blackBuffer
    b_sendIndexData(0x13, nextBuffer, u_pageColourSize); // Second frame, 0x00

    // Additional settings for fast update, 154 213 266 370 and 437 screens (s_flag50)
    if (s_flag50)
    {
        b_sendCommandData8(0x50, 0x07); // Vcom and data interval setting
    }

    // Copy next frame to previous frame
    memcpy(previousBuffer, nextBuffer, u_pageColourSize); // Copy displayed next to previous
}

void Screen_EPD_EXT3_Fast::COG_SmallP_update(uint8_t updateMode)
{
    // Application note § 6. Send updating command
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_150_KS_0J:
        case eScreen_EPD_152_KS_0J:

            b_waitBusy(LOW); // 152 specific
            b_sendCommand8(0x20); // Display Refresh
            digitalWrite(b_pin.panelCS, HIGH); // CS# = 1
            b_waitBusy(LOW); // 152 specific
            break;

        default:

            b_waitBusy();

            b_sendCommand8(0x04); // Power on
            b_waitBusy();

            b_sendCommand8(0x12); // Display Refresh
            b_waitBusy();
            break;
    }
}

void Screen_EPD_EXT3_Fast::COG_SmallP_powerOff()
{
    // Application note § 7. Turn-off DC/DC
    switch (u_eScreen_EPD)
    {
        case eScreen_EPD_150_KS_0J:
        case eScreen_EPD_152_KS_0J:

            break;

        default:

            b_sendCommand8(0x02); // Turn off DC/DC
            b_waitBusy();
            break;
    }
}
//
// --- End of Small screens with P film
//
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
    s_newImage = 0; // nullptr
    COG_data[0] = 0;
}

void Screen_EPD_EXT3_Fast::begin()
{
    // u_eScreen_EPD = eScreen_EPD_EXT3;
    u_codeSize = SCREEN_SIZE(u_eScreen_EPD);
    u_codeFilm = SCREEN_FILM(u_eScreen_EPD);
    u_codeDriver = SCREEN_DRIVER(u_eScreen_EPD);
    u_codeExtra = SCREEN_EXTRA(u_eScreen_EPD);
    v_screenColourBits = 2; // BWR and BWRY

    // Checks
    switch (u_codeFilm)
    {
        case FILM_P: // BW, fast update

            break;

        default:

            debugVariant(FILM_P);
            break;
    }

    //
    // === Touch section
    //

    //
    // === End of touch section
    //

    //
    // === Large screen section
    //
    // Check panelCSS for large screens
    if (((u_codeSize == SIZE_969) or (u_codeSize == SIZE_1198)) and (b_pin.panelCSS == NOT_CONNECTED))
    {
        mySerial.println();
        mySerial.println("hV * Required pin panelCSS is NOT_CONNECTED");
        while (0x01);
    }
    //
    // === End of Large screen section
    //

    // Configure board
    switch (u_codeSize)
    {
        case SIZE_343: // 3.43"
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

    //
    // === Touch section
    //

    //
    // === End of touch section
    //

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

        case SIZE_343: // 3.43" and 3.43"-Touch

            v_screenSizeV = 392; // vertical = wide size
            v_screenSizeH = 456; // horizontal = small size
            break;

        case SIZE_350: // 3.50"

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

        // Those screens are not available with embedded fast update or wide temperature
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
    mySerial.println(formatString("hV = Screen %s", WhoAmI().c_str()));
    mySerial.println(formatString("hV = Size %ix%i", screenSizeX(), screenSizeY()));
    mySerial.println(formatString("hV = Number %i-%cS-0%c", u_codeSize, u_codeFilm, u_codeDriver));
    mySerial.println(formatString("hV = PDLS %s v%i.%i.%i", SCREEN_EPD_EXT3_VARIANT, SCREEN_EPD_EXT3_RELEASE / 100, (SCREEN_EPD_EXT3_RELEASE / 10) % 10, SCREEN_EPD_EXT3_RELEASE % 10));
    mySerial.println();

    u_bufferDepth = v_screenColourBits; // 2 colours
    u_bufferSizeV = v_screenSizeV; // vertical = wide size
    u_bufferSizeH = v_screenSizeH / 8; // horizontal = small size 112 / 8, 1 bit per pixel

    // Force conversion for two unit16_t multiplication into uint32_t.
    // Actually for 1 colour; BWR requires 2 pages.
    u_pageColourSize = (uint32_t)u_bufferSizeV * (uint32_t)u_bufferSizeH;

#if defined(BOARD_HAS_PSRAM) // ESP32 PSRAM specific case

    if (s_newImage == 0)
    {
        static uint8_t * _newFrameBuffer;
        _newFrameBuffer = (uint8_t *) ps_malloc(u_pageColourSize * u_bufferDepth);
        s_newImage = (uint8_t *) _newFrameBuffer;
    }

#else // default case

    if (s_newImage == 0)
    {
        static uint8_t * _newFrameBuffer;
        _newFrameBuffer = new uint8_t[u_pageColourSize * u_bufferDepth];
        s_newImage = (uint8_t *) _newFrameBuffer;
    }

#endif // ESP32 BOARD_HAS_PSRAM

    memset(s_newImage, 0x00, u_pageColourSize * u_bufferDepth);

    setTemperatureC(25); // 25 Celsius = 77 Fahrenheit
    b_fsmPowerScreen = FSM_OFF;
    if (b_pin.panelPower != NOT_CONNECTED)
    {
        setPowerProfile(POWER_MODE_MANUAL, POWER_SCOPE_GPIO_ONLY);
    }

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

    //
    // === Touch section
    //

    //
    // === End of Touch section
    //
}

STRING_TYPE Screen_EPD_EXT3_Fast::WhoAmI()
{
    char work[64] = {0};
    u_WhoAmI(work);

    return formatString("iTC %i.%02i\"%s", v_screenDiagonal / 100, v_screenDiagonal % 100, work);
}

void Screen_EPD_EXT3_Fast::suspend(uint8_t suspendScope)
{
    if (((suspendScope & FSM_GPIO_MASK) == FSM_GPIO_MASK) and (b_pin.panelPower != NOT_CONNECTED))
    {
        if ((b_fsmPowerScreen & FSM_GPIO_MASK) == FSM_GPIO_MASK)
        {
            b_suspend();
        }
    }
}

void Screen_EPD_EXT3_Fast::resume()
{
    // Target   FSM_ON
    // Source   FSM_OFF
    //          FSM_SLEEP
    if (b_fsmPowerScreen != FSM_ON)
    {
        if ((b_fsmPowerScreen & FSM_GPIO_MASK) != FSM_GPIO_MASK)
        {
            b_resume(); // GPIO

            s_reset(); // Reset

            b_fsmPowerScreen |= FSM_GPIO_MASK;
        }

        // Check type and get tables
        if (u_flagOTP == false)
        {
            s_getDataOTP(); // 3-wire SPI read OTP memory

            s_reset(); // Reset
        }

        // Start SPI, with unicity check
        hV_HAL_SPI_begin(); // Standard 8 MHz
    }
}

void Screen_EPD_EXT3_Fast::s_reset()
{
    switch (b_family)
    {
        case FAMILY_MEDIUM:

            COG_MediumP_reset();
            break;

        case FAMILY_SMALL:

            COG_SmallP_reset();
            break;

        default:

            break;
    }
}

void Screen_EPD_EXT3_Fast::s_getDataOTP()
{
    hV_HAL_SPI_end(); // With unicity check

    hV_HAL_SPI3_begin(); // Define 3-wire SPI pins

    // Get data OTP
    switch (b_family)
    {
        case FAMILY_MEDIUM:

            COG_MediumP_getDataOTP();
            break;

        case FAMILY_SMALL:

            COG_SmallP_getDataOTP();
            break;

        default:

            break;
    }
}

void Screen_EPD_EXT3_Fast::s_flush(uint8_t updateMode)
{
    // Resume
    if (b_fsmPowerScreen != FSM_ON)
    {
        resume();
    }

    switch (b_family)
    {
        case FAMILY_MEDIUM:

            COG_MediumP_initial(updateMode); // Initialise
            COG_MediumP_sendImageData(updateMode); // Send image data
            COG_MediumP_update(updateMode); // Update
            COG_MediumP_powerOff(); // Power off
            break;

        case FAMILY_SMALL:

            COG_SmallP_initial(updateMode); // Initialise
            COG_SmallP_sendImageData(updateMode); // Send image data
            COG_SmallP_update(updateMode); // Update
            COG_SmallP_powerOff(); // Power off
            break;

        default:

            break;
    }

    // Suspend
    if (u_suspendMode == POWER_MODE_AUTO)
    {
        suspend(u_suspendScope);
    }
}

uint8_t Screen_EPD_EXT3_Fast::flushMode(uint8_t updateMode)
{
    updateMode = checkTemperatureMode(updateMode);

    switch (updateMode)
    {
        case UPDATE_FAST:
        case UPDATE_GLOBAL:

            s_flush(UPDATE_FAST);
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
                s_newImage[i * u_bufferSizeH + j] = pattern;
            }
        }
    }
    else if ((colour == myColours.white) xor u_invert)
    {
        // physical black 0-0
        memset(s_newImage, 0x00, u_pageColourSize);
    }
    else
    {
        // physical white 1-0
        memset(s_newImage, 0xff, u_pageColourSize);
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
        bitClear(s_newImage[z1], b1);
    }
    else if ((colour == myColours.black) xor u_invert)
    {
        // physical white 1-0
        bitSet(s_newImage[z1], b1);
    }
}

void Screen_EPD_EXT3_Fast::s_setOrientation(uint8_t orientation)
{
    v_orientation = orientation % 4;
}

bool Screen_EPD_EXT3_Fast::s_orientCoordinates(uint16_t & x, uint16_t & y)
{
    bool _flagResult = RESULT_ERROR;
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
    return 0x0000;
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

