// Controller.cpp
#include "Controller.h"

void Controller::Write(uint8_t data) {
    strobe = data & 1;
    if (strobe) {
        shiftRegister = buttonStates;
    }
}

uint8_t Controller::Read() {
    uint8_t value = 0;
    if (strobe) {
        shiftRegister = buttonStates;
    }
    value = shiftRegister & 1;
    shiftRegister >>= 1;
    return value | 0x40; // Bit 6 high as per NES controller protocol
}

void Controller::SetButtonState(uint8_t button, bool pressed) {
    if (pressed)
        buttonStates |= (1 << button);
    else
        buttonStates &= ~(1 << button);
}
