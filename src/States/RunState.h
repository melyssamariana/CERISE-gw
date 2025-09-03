// RunState.h
#pragma once
#include "State.h"

class RunState : public State {
public:
    const char* getName() override { return "RunState"; }
    void enter() override;
    State* update() override;
    void exit() override;
};
