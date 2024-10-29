// Controller.h
#pragma once
#include <cstdint>

class Controller {
public:
    void Write(uint8_t data);
    uint8_t Read();

    void SetButtonState(uint8_t button, bool pressed);

private:
    uint8_t buttonStates = 0;
    uint8_t shiftRegister = 0;
    bool strobe = false;
};
