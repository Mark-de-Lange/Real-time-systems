#ifndef PINCONFIG_HPP
#define PINCONFIG_HPP

extern "C" {
    #include "driver/gpio.h"
}

// Schipdetectie
#define PIN_SCHIP_KNOP       10
#define PIN_SCHIP_HOOGTE     1
#define PIN_SCHIP_BREEDTE    2

// Afmeldknop via ISR (falling edge)
#define PIN_SCHIP_AFMELDEN   15

// Slagboom
#define PIN_SLAGBOOM_OUT_OMHOOG   12
#define PIN_SLAGBOOM_OUT_OMLAAG   13
#define PIN_SLAGBOOM_IN_OMHOOG    4
#define PIN_SLAGBOOM_IN_OMLAAG    5

// Brug-actuator
#define PIN_BRUG_OUT_OMHOOG   6
#define PIN_BRUG_OUT_OMLAAG   7
#define PIN_BRUG_IN_OMHOOG    8
#define PIN_BRUG_IN_OMLAAG    9

// VerkeersStoplicht
#define PIN_VRK_ROOD         11
#define PIN_VRK_ORANJE       14
#define PIN_VRK_GROEN        18

// BotenStoplicht
#define PIN_BOOT_ROOD        46
#define PIN_BOOT_GROEN       47

#endif
