///
/// @file Example_Fast_Temperature.ino
/// @brief Example of features for fast edition
///
/// @details Library for Pervasive Displays EXT3 - Basic level
///
/// @author Rei Vilo
/// @date 21 Mar 2024
/// @version 801
///
/// @copyright (c) Rei Vilo, 2010-2024
/// @copyright Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
/// @copyright For exclusive use with Pervasive Displays screens
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

// Define structures and classes

// Define variables and constants
Screen_EPD_EXT3_Fast myScreen(eScreen_EPD_271_PS_09, boardRaspberryPiPico_RP2040);
// Screen_EPD_EXT3_Fast myScreen(eScreen_EPD_271_KS_09, boardRaspberryPiPico_RP2040);

// Prototypes

// Utilities

// Functions
void check(int8_t temperatureC, uint8_t expectedMode)
{
    const char * stringMode[] = { "NONE", "GLOBAL", "FAST", "PARTIAL" };
    myScreen.setTemperatureC(temperatureC);
    uint8_t recommendedMode = myScreen.checkTemperatureMode(expectedMode);

    // Raspberry Pi SDK core for RP2040
    // mySerial.printf("Temperature= %+3i C - Mode: %8s -> %-8s", temperatureC, stringMode[expectedMode], stringMode[recommendedMode]);

    // Arduino core for RP2040
    mySerial.print("Temperature= ");
    mySerial.print(temperatureC);
    mySerial.print(" C - Mode: ");
    mySerial.print(stringMode[expectedMode]);
    mySerial.print(" -> ");
    mySerial.print(stringMode[recommendedMode]);
    mySerial.println();
}

void performTest()
{
    check(+70, UPDATE_FAST);

    check(+60, UPDATE_FAST);
    check(+50, UPDATE_FAST);
    check(+25, UPDATE_FAST);
    check(-15, UPDATE_FAST);
    check(-25, UPDATE_FAST);
}

// Add setup code
///
/// @brief Setup
///
void setup()
{
    mySerial.begin(115200);

    mySerial.println();
    mySerial.println("=== " __FILE__);
    mySerial.println("=== " __DATE__ " " __TIME__);
    mySerial.println();

    // Screen
    myScreen.begin();

    mySerial.println(formatString("=== %s %ix%i", myScreen.WhoAmI().c_str(), myScreen.screenSizeX(), myScreen.screenSizeY()));

    performTest();

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
