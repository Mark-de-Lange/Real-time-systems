#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "driver/gpio.h"
    #include "esp_system.h"
}

#include "SchipDetectieSensor.hpp"
#include "Ophaalbrug.hpp"
#include "Slagboom.hpp"
#include "Actuator.hpp"
#include "VerkeersStoplicht.hpp"
#include "BotenStoplicht.hpp"
#include "pinConfig.hpp"

extern "C" void app_main(void)
{
    // ------------------------------------------------------------
    // 1. ISR service MOET als eerste worden geïnstalleerd
    // ------------------------------------------------------------
    gpio_install_isr_service(0);

    setvbuf(stdout, nullptr, _IONBF, 0);

    // ------------------------------------------------------------
    // 2. Hardware componenten aanmaken
    // ------------------------------------------------------------

    static Slagboom slagboom(
        PIN_SLAGBOOM_OUT_OMHOOG,
        PIN_SLAGBOOM_OUT_OMLAAG,
        PIN_SLAGBOOM_IN_OMHOOG,
        PIN_SLAGBOOM_IN_OMLAAG
    );

    static VerkeersStoplicht verkeersStoplicht(PIN_VRK_ROOD, PIN_VRK_ORANJE, PIN_VRK_GROEN);
    static BotenStoplicht botenStoplicht(PIN_BOOT_ROOD, PIN_BOOT_GROEN);

    static Actuator actuator(
        PIN_BRUG_OUT_OMHOOG,
        PIN_BRUG_OUT_OMLAAG,
        PIN_BRUG_IN_OMHOOG,
        PIN_BRUG_IN_OMLAAG
    );

    const int MAX_HOOGTE  = 200;
    const int MAX_BREEDTE = 400;

    static Ophaalbrug brug(
        slagboom,
        verkeersStoplicht,
        botenStoplicht,
        actuator,
        MAX_HOOGTE,
        MAX_BREEDTE
    );

    // ------------------------------------------------------------
    // 3. Schipdetectiesensor met AANMELD + AFMELD ISR
    // ------------------------------------------------------------
    static SchipDetectieSensor sensor(
        PIN_SCHIP_KNOP,       // Aanmelden knop
        PIN_SCHIP_AFMELDEN,   // Afmelden knop
        PIN_SCHIP_HOOGTE,     // ADC hoogte
        PIN_SCHIP_BREEDTE     // ADC breedte
    );

    sensor.setBridgeQueue(brug.getEventQueue());

    // ------------------------------------------------------------
    // 4. Tasks starten
    // ------------------------------------------------------------
    xTaskCreate(&Ophaalbrug::OphaalbrugTask, "brugTask", 4096, &brug, 5, nullptr);

    TaskHandle_t sensH;
    xTaskCreate(&SchipDetectieSensor::SchipDetectieTask, "sensorTask", 4096, &sensor, 5, &sensH);
    sensor.setTaskHandle(sensH);

    TaskHandle_t boomH;
    xTaskCreate(&Slagboom::SlagboomTask, "boomTask", 4096, &slagboom, 5, &boomH);
    slagboom.setTaskHandle(boomH);
    slagboom.setBridgeQueue(brug.getEventQueue());

    verkeersStoplicht.setBridgeQueue(brug.getEventQueue());
    verkeersStoplicht.startTask("VerkeersStoplichtTask", 4, 4096);

    botenStoplicht.startTask("BotenStoplichtTask", 4, 4096);

    TaskHandle_t actH;
    xTaskCreate(&Actuator::ActuatorTask, "actuatorTask", 4096, &actuator, 5, &actH);
    actuator.setTaskHandle(actH);
    actuator.setBridgeQueue(brug.getEventQueue());

    printf("app_main: setup complete.\n");
}
