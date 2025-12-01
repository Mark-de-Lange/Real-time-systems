#ifndef PINCONFIG_HPP
#define PINCONFIG_HPP

extern "C" {
    #include "driver/gpio.h"
}

// Schipdetectie
#define PIN_SCHIP_KNOP       5
#define PIN_SCHIP_HOOGTE     35   // ADC1_CHANNEL_7
#define PIN_SCHIP_BREEDTE    36   // ADC1_CHANNEL_0

// Slagboom (4 pinnen: 2 outputs + 2 inputs)
#define PIN_SLAGBOOM_OUT_OMHOOG  21   // output: slagboom omhoog
#define PIN_SLAGBOOM_OUT_OMLAAG  11   // output: slagboom omlaag
#define PIN_SLAGBOOM_IN_OMHOOG   19   // input sensor: slagboom bereikt omhoog
#define PIN_SLAGBOOM_IN_OMLAAG   20   // input sensor: slagboom bereikt omlaag

// Brug-actuator
#define PIN_BRUG_ACTUATOR    18

// VerkeersStoplicht (auto’s)
#define PIN_VRK_ROOD         25
#define PIN_VRK_ORANJE       26
#define PIN_VRK_GROEN        27

// BotenStoplicht
#define PIN_BOOT_ROOD        32
#define PIN_BOOT_GROEN       33

#endif // PINCONFIG_HPP
