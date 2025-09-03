// ConnectState.h
#pragma once
#include "State.h"

class ConnectState : public State {
public:
    const char* getName() override { return "ConnectState"; }
    void enter() override;
    State* update() override;
    void exit() override;
};
