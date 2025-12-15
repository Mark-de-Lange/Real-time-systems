#ifndef BRIDGEEVENTS_HPP
#define BRIDGEEVENTS_HPP

enum class BridgeEventType {
    SCHIP_GEDETECTEERD,
    SCHIP_AFGEMELD,     // <--- NIEUW
    VERKEERSLICHT_ROOD,
    SLAGBOOM_DICHT,
    BRUG_OPEN,
    GEEN_WACHTENDE_SCHEPEN,
    BRUG_DICHT,
    SLAGBOOM_OPEN,
    NOODSTOP
};

struct BridgeEventMsg {
    BridgeEventType type;
    int schipHoogte;
    int schipBreedte;
};

#endif
