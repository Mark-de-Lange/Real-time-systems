#ifndef BRIDGEEVENTS_HPP
#define BRIDGEEVENTS_HPP

enum class BridgeEventType {
    NONE = 0,

    // Actuator (brugmotor)
    BRUG_OPEN,
    BRUG_DICHT,

    // Slagboom
    SLAGBOOM_OPEN,
    SLAGBOOM_DICHT,

    // Verkeersstoplicht weg
    VERKEERSLICHT_ROOD,

    // Botenstoplicht
    BOTEN_ROOD,
    BOTEN_GROEN,

    // Schipdetectie
    SCHIP_GEDETECTEERD,
    SCHIP_UITGEVAARD,
    GEEN_WACHTENDE_SCHEPEN,

    // Noodstop
    NOODSTOP
};

struct BridgeEventMsg {
    BridgeEventType type;
    int schipHoogte = 0;
    int schipBreedte = 0;
};

#endif
