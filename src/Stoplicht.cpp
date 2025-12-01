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
        return; // geen verandering → geen notify

    mDoorlaat = state;

    if (mTask != nullptr)
        xTaskNotifyGive(mTask);
}

void Stoplicht::SetTegenhouden(bool state)
{
    if (mTegenhouden == state)
        return; // geen verandering → geen notify

    mTegenhouden = state;

    if (mTask != nullptr)
        xTaskNotifyGive(mTask);
}
