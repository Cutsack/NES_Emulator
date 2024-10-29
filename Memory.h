// Memory.h
#pragma once
#include <cstdint>
#include "Cartridge.h"
#include "PPU.h"
#include "Controller.h"

class Memory {
public:
    Memory(Cartridge* cart);
    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t data);

    void ConnectPPU(PPU* ppu);
    void ConnectController(Controller* controller);

private:
    uint8_t RAM[2048]; // 2KB internal RAM
    Cartridge* cartridge;
    PPU* ppu;
    Controller* controller;
};
