#pragma once

#include <string>

class State {
public:
    virtual void enter() = 0;
    virtual State* update() = 0;
    virtual void exit() = 0;
    virtual const char* getName() = 0; 
    virtual ~State() {}
};
