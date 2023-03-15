///
/// @file Example_Fast_Orientation.ino
/// @brief Example of features for basic edition
///
/// @details Library for Pervasive Displays EXT3 - Basic level
///
/// @author Rei Vilo
/// @date 20 Mar 2023
/// @version 607
///
/// @copyright (c) Rei Vilo, 2010-2023
/// @copyright Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
///
/// @see ReadMe.txt for references
/// @n
///

// SDK
#if defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#else // Arduino general
#include "Arduino.h"
#endif // SDK

// Set parameters
#define DISPLAY_FAST_ORIENTATION 1

// Include application, user and local libraries
#include "SPI.h"
#include "PDLS_EXT3_Basic_Fast.h"

// Define structures and classes

// Define variables and constants
Screen_EPD_EXT3_Fast myScreen(eScreen_EPD_EXT3_271_09_Fast, boardRaspberryPiPico_RP2040);

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
        Serial.print(formatString(" > %i  \r", i));
        delay(1000);
    }
    Serial.print("         \r");
}

///
/// @brief Flush screen and display time
///
void flush_ms()
{
    uint32_t chrono = (uint32_t)millis();
    myScreen.flush();
    Serial.print(millis() - chrono);
    Serial.println(" ms");
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
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("=== " __FILE__);
    Serial.println("=== " __DATE__ " " __TIME__);
    Serial.println();

    Serial.print("begin... ");
    myScreen.begin();
    Serial.println(formatString("%s %ix%i", myScreen.WhoAmI().c_str(), myScreen.screenSizeX(), myScreen.screenSizeY()));

#if (DISPLAY_FAST_ORIENTATION == 1)
    Serial.print("DISPLAY_FAST_ORIENTATION... ");
    myScreen.clear();
    displayFastOrientation();
    Serial.println("done");

    wait(4);

#endif // DISPLAY_FAST_ORIENTATION

    Serial.print("White... ");
    myScreen.clear();
    flush_ms();

    Serial.println("=== ");
    Serial.println();

#if defined(ARDUINO_ARCH_PIDUINO)
    exit(0);
#endif
}

// Add loop code
///
/// @brief Loop, empty
///
void loop()
{
    delay(1000);
}
