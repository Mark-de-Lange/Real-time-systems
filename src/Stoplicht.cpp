#include "Stoplicht.hpp"
#include <cstdio>

Stoplicht::Stoplicht()
: mDoorlaat(false),
  mTegenhouden(true),   // standaard rood
  mTask(nullptr)
{
}

void Stoplicht::SetDoorlaat(bool state)
{
    if (mDoorlaat == state)
        return;

    mDoorlaat = state;

    if (mTask != nullptr)
        xTaskNotifyGive(mTask);
}

void Stoplicht::SetTegenhouden(bool state)
{
    if (mTegenhouden == state)
        return;

    mTegenhouden = state;

    if (mTask != nullptr)
        xTaskNotifyGive(mTask);
}
