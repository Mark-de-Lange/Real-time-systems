#include "SchipDetectieSensor.hpp"
#include <cstdio>

SchipDetectieSensor::SchipDetectieSensor(
    int pinAanwezig,
    int pinHoogte,
    int pinBreedte
)
: mSchipAanwezig(false),
  mSchipHoogte(0),
  mSchipBreedte(0),
  mPin(pinAanwezig),
  mPinHoogte(pinHoogte),
  mPinBreedte(pinBreedte),
  mTaskHandle(nullptr),
  // ESP32 mapping:
  // GPIO35 -> ADC1_CHANNEL_7
  // GPIO36 -> ADC1_CHANNEL_0
  mHoogteChannel(ADC1_CHANNEL_7),
  mBreedteChannel(ADC1_CHANNEL_0)
{
    // --- GPIO aanwezigheids-knop ---
    gpio_config_t io_conf{};
    io_conf.intr_type    = GPIO_INTR_NEGEDGE;              // high -> low
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << mPin;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;             // interne pull-up
    gpio_config(&io_conf);

    // ISR handler (let op: gpio_install_isr_service(0) in main!)
    gpio_isr_handler_add(static_cast<gpio_num_t>(mPin),
                         &SchipDetectieSensor::isrHandler,
                         this);

    // --- ADC configuratie ---
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(mHoogteChannel,  ADC_ATTEN_DB_11);
    adc1_config_channel_atten(mBreedteChannel, ADC_ATTEN_DB_11);
}

// ---------- ISR ----------

void IRAM_ATTR SchipDetectieSensor::isrHandler(void* arg)
{
    auto* self = static_cast<SchipDetectieSensor*>(arg);
    if (self) {
        self->onInterrupt();
    }
}

void IRAM_ATTR SchipDetectieSensor::onInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (mTaskHandle != nullptr) {
        vTaskNotifyGiveFromISR(mTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// ---------- Task ----------

void SchipDetectieSensor::taskEntry(void* pvParameters)
{
    auto* self = static_cast<SchipDetectieSensor*>(pvParameters);
    if (self) {
        self->taskLoop();
    }
    vTaskDelete(nullptr);
}

void SchipDetectieSensor::taskLoop()
{
    for (;;)
    {
        std::printf("[SchipDetectieSensor] Wacht op notify...\n");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (CheckSchip()) {
            std::printf("[SchipDetectieSensor] Schip gedetecteerd! "
                        "hoogte=%d, breedte=%d\n",
                        mSchipHoogte, mSchipBreedte);
        } else {
            std::printf("[SchipDetectieSensor] Geen schip.\n");
        }
    }
}

// ---------- interne helpers ----------

int SchipDetectieSensor::readHoogteRaw()
{
    return adc1_get_raw(mHoogteChannel);
}

int SchipDetectieSensor::readBreedteRaw()
{
    return adc1_get_raw(mBreedteChannel);
}

// hier komt later je echte kalibratie
int SchipDetectieSensor::convertHoogte(int raw)
{
    // voorbeeld: 0..4095 -> 0..300 cm
    float scale = 300.0f / 4095.0f;
    return static_cast<int>(raw * scale);
}

int SchipDetectieSensor::convertBreedte(int raw)
{
    // voorbeeld: 0..4095 -> 0..500 cm
    float scale = 500.0f / 4095.0f;
    return static_cast<int>(raw * scale);
}

// ---------- UML: CheckSchip() ----------

bool SchipDetectieSensor::CheckSchip()
{
    int level = gpio_get_level(static_cast<gpio_num_t>(mPin));
    mSchipAanwezig = (level == 0);   // actief laag (knop naar GND)

    if (mSchipAanwezig) {
        int rawH = readHoogteRaw();
        int rawB = readBreedteRaw();

        mSchipHoogte  = convertHoogte(rawH);
        mSchipBreedte = convertBreedte(rawB);
    } else {
        mSchipHoogte  = 0;
        mSchipBreedte = 0;
    }

    return mSchipAanwezig;
}
