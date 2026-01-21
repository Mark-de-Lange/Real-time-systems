#include "SchipDetectieSensor.hpp"
#include <cstdio>

SchipDetectieSensor::SchipDetectieSensor(
    int pinAanwezig,
    int pinAfmelden,
    int pinHoogte,
    int pinBreedte
)
: mPinAanwezig(pinAanwezig),
  mPinAfmelden(pinAfmelden),
  mPinHoogte(pinHoogte),
  mPinBreedte(pinBreedte),
  mHoogteChannel(gpioToAdcChannel(pinHoogte)),
  mBreedteChannel(gpioToAdcChannel(pinBreedte)),
  mSchipAanwezig(false),
  mSchipHoogte(0),
  mSchipBreedte(0),
  mTaskHandle(nullptr),
  mBridgeQueue(nullptr),
  mLastEvent(SensorEvent::NONE)
{
    // Configureer AANWEZIG knop (falling edge)
    gpio_config_t io{};
    io.intr_type    = GPIO_INTR_NEGEDGE;
    io.mode         = GPIO_MODE_INPUT;
    io.pin_bit_mask = 1ULL << mPinAanwezig;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io);

    // ISR → aanwezigISR()
    gpio_isr_handler_add(
        static_cast<gpio_num_t>(mPinAanwezig),
        &SchipDetectieSensor::aanwezigISR,
        this
    );

    // Configureer AFMELD knop (falling edge)
    gpio_config_t io2{};
    io2.intr_type    = GPIO_INTR_NEGEDGE;
    io2.mode         = GPIO_MODE_INPUT;
    io2.pin_bit_mask = 1ULL << mPinAfmelden;
    io2.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io2.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io2);

    gpio_isr_handler_add(
        static_cast<gpio_num_t>(mPinAfmelden),
        &SchipDetectieSensor::afmeldISR,
        this
    );
    
    // ADC configuratie    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(mHoogteChannel,  ADC_ATTEN_DB_11);
    adc1_config_channel_atten(mBreedteChannel, ADC_ATTEN_DB_11);
}

// ISR IMPLEMENTATIES
void IRAM_ATTR SchipDetectieSensor::aanwezigISR(void* arg)
{
    auto* self = static_cast<SchipDetectieSensor*>(arg);
    if (self) self->onAanwezigInterrupt();
}

void IRAM_ATTR SchipDetectieSensor::afmeldISR(void* arg)
{
    auto* self = static_cast<SchipDetectieSensor*>(arg);
    if (self) self->onAfmeldInterrupt();
}

void IRAM_ATTR SchipDetectieSensor::onAanwezigInterrupt()
{
    if (!mTaskHandle) return;

    mLastEvent = SensorEvent::AANWEZIG;

    BaseType_t w = pdFALSE;
    vTaskNotifyGiveFromISR(mTaskHandle, &w);
    portYIELD_FROM_ISR(w);
}

void IRAM_ATTR SchipDetectieSensor::onAfmeldInterrupt()
{
    if (!mTaskHandle) return;

    mLastEvent = SensorEvent::AFMELD;

    BaseType_t w = pdFALSE;
    vTaskNotifyGiveFromISR(mTaskHandle, &w);
    portYIELD_FROM_ISR(w);
}

// TASK LOOP
void SchipDetectieSensor::taskLoop()
{
    for (;;)
    {
        std::printf("[SchipDetectieSensor] Wacht op ISR notify...\n");

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // -------- AANWEZIG ----------
        if (mLastEvent == SensorEvent::AANWEZIG)
        {
            bool schip = CheckSchip();

            BridgeEventMsg msg{};
            msg.schipHoogte  = mSchipHoogte;
            msg.schipBreedte = mSchipBreedte;

            if (schip)
            {
                printf("[SchipDetectieSensor] Schip gedetecteerd: H=%d B=%d\n",
                        mSchipHoogte, mSchipBreedte);

                msg.type = BridgeEventType::SCHIP_GEDETECTEERD;
            }
            else
            {
                printf("[SchipDetectieSensor] Geen schip aanwezig.\n");

                msg.type = BridgeEventType::GEEN_WACHTENDE_SCHEPEN;
            }

            xQueueSend(mBridgeQueue, &msg, 0);
        }

        // AFMELDEN
        else if (mLastEvent == SensorEvent::AFMELD)
        {
            printf("[SchipDetectieSensor] Afmeldknop ISR → schip afmelden.\n");

            BridgeEventMsg msg{};
            msg.type = BridgeEventType::SCHIP_AFGEMELD;
            msg.schipHoogte = 0;
            msg.schipBreedte = 0;

            xQueueSend(mBridgeQueue, &msg, 0);
        }

        mLastEvent = SensorEvent::NONE;
    }
}

// HELPER FUNCTIES
bool SchipDetectieSensor::CheckSchip()
{
    int level = gpio_get_level(static_cast<gpio_num_t>(mPinAanwezig));
    mSchipAanwezig = (level == 0);

    if (mSchipAanwezig)
    {
        int rawH = readHoogteRaw();
        int rawB = readBreedteRaw();

        mSchipHoogte  = convertHoogte(rawH);
        mSchipBreedte = convertBreedte(rawB);
    }
    else
    {
        mSchipHoogte = 0;
        mSchipBreedte = 0;
    }

    return mSchipAanwezig;
}

int SchipDetectieSensor::readHoogteRaw()
{
    return adc1_get_raw(mHoogteChannel);
}

int SchipDetectieSensor::readBreedteRaw()
{
    return adc1_get_raw(mBreedteChannel);
}

int SchipDetectieSensor::convertHoogte(int raw)
{
    return static_cast<int>((raw * 300.0f) / 4095.0f);
}

int SchipDetectieSensor::convertBreedte(int raw)
{
    return static_cast<int>((raw * 500.0f) / 4095.0f);
}

adc1_channel_t SchipDetectieSensor::gpioToAdcChannel(int gpio)
{
    switch (gpio) {
        case 1: return ADC1_CHANNEL_0;
        case 2: return ADC1_CHANNEL_1;
        case 3: return ADC1_CHANNEL_2;
        case 4: return ADC1_CHANNEL_3;
        case 5: return ADC1_CHANNEL_4;
        case 6: return ADC1_CHANNEL_5;
        case 7: return ADC1_CHANNEL_6;
        case 8: return ADC1_CHANNEL_7;
        case 9: return ADC1_CHANNEL_8;
        case 10:return ADC1_CHANNEL_9;
        default:return ADC1_CHANNEL_0;
    }
}
