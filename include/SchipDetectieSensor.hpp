#ifndef SCHIPDETECTIESENSOR_HPP
#define SCHIPDETECTIESENSOR_HPP

#include <cstdint>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "BridgeEvents.hpp"

class SchipDetectieSensor
{
public:
    SchipDetectieSensor(int pinAanwezig,
                        int pinAfmelden,
                        int pinHoogte,
                        int pinBreedte);

    static void SchipDetectieTask(void* pv) {
        static_cast<SchipDetectieSensor*>(pv)->taskLoop();
    }

    void setTaskHandle(TaskHandle_t h) { mTaskHandle = h; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

private:
    enum class SensorEvent {
        NONE,
        AANWEZIG,
        AFMELD
    };

    // --- ORDER FIXED BELOW ---
    int mPinAanwezig;
    int mPinAfmelden;
    int mPinHoogte;
    int mPinBreedte;

    adc1_channel_t mHoogteChannel;
    adc1_channel_t mBreedteChannel;

    bool mSchipAanwezig;
    int  mSchipHoogte;
    int  mSchipBreedte;

    TaskHandle_t   mTaskHandle;
    QueueHandle_t  mBridgeQueue;

    volatile SensorEvent mLastEvent;

    // ISR handlers
    static void IRAM_ATTR aanwezigISR(void* arg);
    static void IRAM_ATTR afmeldISR(void* arg);

    void IRAM_ATTR onAanwezigInterrupt();
    void IRAM_ATTR onAfmeldInterrupt();

    // Task loop
    void taskLoop();

    // Helpers
    bool CheckSchip();
    int  readHoogteRaw();
    int  readBreedteRaw();
    int  convertHoogte(int raw);
    int  convertBreedte(int raw);
    adc1_channel_t gpioToAdcChannel(int gpio);
};

#endif
