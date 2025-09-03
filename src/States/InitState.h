// InitState.h
#pragma once
#include "State.h"

class InitState : public State {
public:
    const char* getName() override { return "InitState"; }
    void enter() override;
    State* update() override;
    void exit() override;
};
