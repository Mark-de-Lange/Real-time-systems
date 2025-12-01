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
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("app_main: start\n");

    // ISR service slechts één keer
    gpio_install_isr_service(0);

    // ---- Hardware-objecten (pinnen zijn voorbeeld) ----
    static Slagboom slagboom(
        PIN_SLAGBOOM_OUT_OMHOOG,
        PIN_SLAGBOOM_OUT_OMLAAG,
        PIN_SLAGBOOM_IN_OMHOOG,
        PIN_SLAGBOOM_IN_OMLAAG
    );

    static VerkeersStoplicht verkeersStoplicht(PIN_VRK_ROOD, PIN_VRK_ORANJE, PIN_VRK_GROEN);

    static BotenStoplicht botenStoplicht(PIN_BOOT_ROOD, PIN_BOOT_GROEN);

    static Actuator actuator(PIN_BRUG_ACTUATOR);                 // brug-actuator op GPIO18

    // Maximale maten van de brug (voorbeeldwaarden in cm)
    const int MAX_HOOGTE  = 300;
    const int MAX_BREEDTE = 500;

    // Ophaalbrug kent nu slagboom + beide stoplichten + actuator
    static Ophaalbrug brug(
        slagboom,
        verkeersStoplicht,
        botenStoplicht,
        actuator,
        MAX_HOOGTE,
        MAX_BREEDTE
    );

    // Schipdetectie (GPIO5 knop, 35/36 als analoge sensoren)
    static SchipDetectieSensor schipSensor(PIN_SCHIP_KNOP, PIN_SCHIP_HOOGTE, PIN_SCHIP_BREEDTE);

    // ---- Tasks aanmaken ----

    // Task voor Ophaalbrug (event-loop / state-machine)
    xTaskCreate(
        &Ophaalbrug::OphaalbrugTask,
        "OphaalbrugTask",
        4096,
        &brug,
        5,
        nullptr
    );

    // Task voor SchipDetectieSensor
    TaskHandle_t schipTaskHandle = nullptr;
    xTaskCreate(
        &SchipDetectieSensor::SchipDetectieTask,
        "SchipDetectieTask",
        4096,
        &schipSensor,
        5,
        &schipTaskHandle
    );
    schipSensor.setTaskHandle(schipTaskHandle);

    // Slagboom-task
    TaskHandle_t slagboomTaskHandle = nullptr;
    xTaskCreate(
        &Slagboom::SlagboomTask,
        "SlagboomTask",
        4096,
        &slagboom,
        5,
        &slagboomTaskHandle
    );
    slagboom.setTaskHandle(slagboomTaskHandle);
    slagboom.setBridgeQueue(brug.getEventQueue());

    // Stoplicht-taken starten (zij maken intern hun eigen notify-loop af)
    verkeersStoplicht.startTask("VerkeersStoplichtTask", 4, 2048);
    botenStoplicht.startTask("BotenStoplichtTask", 4, 2048);

    // Actuator-task
    TaskHandle_t actuatorTaskHandle = nullptr;
    xTaskCreate(
        &Actuator::ActuatorTask,
        "ActuatorTask",
        4096,
        &actuator,
        5,
        &actuatorTaskHandle
    );
    actuator.setTaskHandle(actuatorTaskHandle);
    actuator.setBridgeQueue(brug.getEventQueue());

    // ---- Queue koppelen: sensor -> ophaalbrug ----
    schipSensor.setBridgeQueue(brug.getEventQueue());

    std::printf("app_main: setup complete.\n");
}
