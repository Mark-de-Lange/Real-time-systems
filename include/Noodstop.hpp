#ifndef NOODSTOP_HPP
#define NOODSTOP_HPP

using namespace std;

class Noodstop {
private:
    bool mSignaal;
    int mPort;
    int mPin;

public:
    Noodstop();
    
    void SetSignaalNoodstop();
    bool GetSignaalNoodstop();
};

#endif