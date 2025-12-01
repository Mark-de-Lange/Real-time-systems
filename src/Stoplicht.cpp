#include "Stoplicht.hpp"
#include <cstdio>

Stoplicht::Stoplicht()
: mDoorlaat(false),
  mTegenhouden(false),
  mTask(nullptr)
{
}

void Stoplicht::startTask(const char* name, UBaseType_t prio, uint32_t stackWords)
{
    xTaskCreate(
        &Stoplicht::taskEntry,
        name,
        stackWords,
        this,
        prio,
        &mTask
    );
}

void Stoplicht::taskEntry(void* pv)
{
    auto* self = static_cast<Stoplicht*>(pv);
    if (self) {
        self->taskLoop();
    }
    vTaskDelete(nullptr);
}

void Stoplicht::taskLoop()
{
    for (;;)
    {
        // Wacht tot iemand SetDoorlaat/SetTegenhouden/SetKleur heeft aangeroepen
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Laat de afgeleide klasse de lampjes zetten
        FaseHandler();
    }
}

void Stoplicht::SetDoorlaat(bool v)
{
    mDoorlaat = v;
    if (v) {
        // Doorlaten en tegenhouden tegelijk is onlogisch → tegenhouden uit
        mTegenhouden = false;
    }

    if (mTask != nullptr) {
        xTaskNotifyGive(mTask);
    }
}

void Stoplicht::SetTegenhouden(bool v)
{
    mTegenhouden = v;
    if (v) {
        // Tegenhouden betekent: niet doorlaten
        mDoorlaat = false;
    }

    if (mTask != nullptr) {
        xTaskNotifyGive(mTask);
    }
}
