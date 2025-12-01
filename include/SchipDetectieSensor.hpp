#ifndef SCHIP_DETECTIE_SENSOR_HPP
#define SCHIP_DETECTIE_SENSOR_HPP

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "driver/gpio.h"
    #include "driver/adc.h"
}

// UML: SchipDetectieSensor
// - mSchipAanwezig : Boolean
// - mSchipHoogte   : Integer
// - mSchipBreedte  : Integer
// - mPin           : Integer  (aanwezigheidsdetectie)
// Extra:
// - mPinHoogte, mPinBreedte voor hoogte/breedte meting
// Logica:
// - Een eigen FreeRTOS-task per object
// - ISR triggert notify naar die task
class SchipDetectieSensor
{
private:
    bool            mSchipAanwezig;
    int             mSchipHoogte;
    int             mSchipBreedte;

    int             mPin;          // aanwezigheids-detectie (bijv. GPIO 5)
    int             mPinHoogte;    // hoogte-sensor (bijv. GPIO 35)
    int             mPinBreedte;   // breedte-sensor (bijv. GPIO 36)

    TaskHandle_t    mTaskHandle;   // FreeRTOS taak van deze sensor
    QueueHandle_t   mBridgeQueue;  // queue naar Ophaalbrug

    adc1_channel_t  mHoogteChannel;
    adc1_channel_t  mBreedteChannel;

    // ISR voor aanwezigheids-pin
    static void IRAM_ATTR isrHandler(void* arg);
    void IRAM_ATTR onInterrupt();

    // Task entry + echte loop
    static void taskEntry(void* pvParameters);
    void taskLoop();

    // interne helpers
    int  readHoogteRaw();
    int  readBreedteRaw();
    int  convertHoogte(int raw);
    int  convertBreedte(int raw);
    adc1_channel_t gpioToAdcChannel(int gpio);

public:
    // «create» SchipDetectieSensor()
    explicit SchipDetectieSensor(
        int pinAanwezig = 5,
        int pinHoogte   = 35,
        int pinBreedte  = 36
    );

    // Wordt aangeroepen vanuit main ná xTaskCreate
    void setTaskHandle(TaskHandle_t handle) { mTaskHandle = handle; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

    // Functie die je in main aan xTaskCreate geeft
    static void SchipDetectieTask(void* pvParameters) {
        taskEntry(pvParameters);
    }

    // UML: CheckSchip() : Boolean
    bool CheckSchip();

    // getters volgens UML
    bool isSchipAanwezig() const { return mSchipAanwezig; }
    int  getSchipHoogte()  const { return mSchipHoogte; }
    int  getSchipBreedte() const { return mSchipBreedte; }
};

#endif // SCHIP_DETECTIE_SENSOR_HPP
