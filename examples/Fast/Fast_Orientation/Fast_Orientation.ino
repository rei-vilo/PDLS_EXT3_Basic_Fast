///
/// @file Fast_Orientation.ino
/// @brief Example of features for basic edition
///
/// @details Library for Pervasive Displays EXT3 - Basic level
///
/// @author Rei Vilo
/// @date 21 Jan 2025
/// @version 812
///
/// @copyright (c) Rei Vilo, 2010-2025
/// @copyright All rights reserved
/// @copyright For exclusive use with Pervasive Displays screens
///
/// * Basic edition: for hobbyists and for basic usage
/// @n Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
///
/// * Evaluation edition: for professionals or organisations, no commercial usage
/// @n All rights reserved
///
/// * Commercial edition: for professionals or organisations, commercial usage
/// @n All rights reserved
///
/// @see ReadMe.txt for references
/// @n
///

// Screen
#include "PDLS_EXT3_Basic_Fast.h"

// SDK
// #include <Arduino.h>
#include "hV_HAL_Peripherals.h"

// Include application, user and local libraries
// #include <SPI.h>

// Configuration
#include "hV_Configuration.h"

// Set parameters
#define DISPLAY_FAST_ORIENTATION 1

// Define structures and classes

// Define variables and constants
Screen_EPD_EXT3_Fast myScreen(eScreen_EPD_271_PS_09, boardRaspberryPiPico_RP2040);

// Prototypes

// Utilities
///
/// @brief Wait with countdown
/// @param second duration, s
///
void wait(uint8_t second)
{
    for (uint8_t i = second; i > 0; i--)
    {
        mySerial.print(formatString(" > %i  \r", i));
        delay(1000);
    }
    mySerial.print("         \r");
}

// Functions
///
/// @brief Flush screen and display time
///
void flush_ms()
{
    uint32_t chrono = (uint32_t)millis();
    myScreen.flush();
    mySerial.print(millis() - chrono);
    mySerial.println(" ms");
}

#if (DISPLAY_FAST_ORIENTATION == 1)

void displayFastOrientation()
{
    myScreen.clear();
    myScreen.selectFont(2);

    for (uint8_t i = 0; i < 4; i++)
    {
        myScreen.setOrientation(i);
        myScreen.gText(4, 4, formatString("> Orientation %i", i));
        flush_ms();
    }
}

#endif // DISPLAY_FAST_ORIENTATION

// Add setup code
///
/// @brief Setup
///
void setup()
{
    mySerial.begin(115200);
    delay(500);
    mySerial.println();
    mySerial.println("=== " __FILE__);
    mySerial.println("=== " __DATE__ " " __TIME__);
    mySerial.println();

    mySerial.print("begin");
    myScreen.begin();
    mySerial.println(formatString("%s %ix%i", myScreen.WhoAmI().c_str(), myScreen.screenSizeX(), myScreen.screenSizeY()));

#if (DISPLAY_FAST_ORIENTATION == 1)

    mySerial.println("DISPLAY_FAST_ORIENTATION");
    myScreen.clear();
    displayFastOrientation();
    wait(4);

#endif // DISPLAY_FAST_ORIENTATION

    mySerial.println("Regenerate");
    myScreen.regenerate();

    mySerial.println("=== ");
    mySerial.println();
}

// Add loop code
///
/// @brief Loop, empty
///
void loop()
{
    delay(1000);
}
