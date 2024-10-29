// Memory.cpp
#include "Memory.h"
#include <cstring>

Memory::Memory(Cartridge* cart) : cartridge(cart), ppu(nullptr), controller(nullptr) {
    std::memset(RAM, 0, sizeof(RAM));
}

void Memory::ConnectPPU(PPU* ppu) {
    this->ppu = ppu;
}

void Memory::ConnectController(Controller* controller) {
    this->controller = controller;
}

uint8_t Memory::Read(uint16_t address) {
    if (address < 0x2000) {
        // Internal RAM mirrored every 2KB
        return RAM[address % 0x0800];
    }
    else if (address >= 0x2000 && address < 0x4000) {
        // PPU registers mirrored every 8 bytes
        return ppu->CPURead(0x2000 + (address % 8));
    }
    else if (address == 0x4016) {
        // Controller port 1
        return controller->Read();
    }
    else if (address >= 0x8000) {
        // PRG ROM
        size_t index = (address - 0x8000) % cartridge->PRG_ROM.size();
        return cartridge->PRG_ROM[index];
    }
    else {
        // Other memory regions
        return 0x00;
    }
}

void Memory::Write(uint16_t address, uint8_t data) {
    if (address < 0x2000) {
        // Internal RAM mirrored every 2KB
        RAM[address % 0x0800] = data;
    }
    else if (address >= 0x2000 && address < 0x4000) {
        // PPU registers mirrored every 8 bytes
        ppu->CPUWrite(0x2000 + (address % 8), data);
    }
    else if (address == 0x4014) {
        // OAMDMA (DMA transfer to OAM)
        uint16_t dmaAddress = data << 8;
        for (int i = 0; i < 256; i++) {
            ppu->OAM[ppu->regOAMAddr++] = Read(dmaAddress + i);
        }
        // Add CPU cycle penalty
        // Assuming you have access to the CPU cycles, adjust accordingly
    }
    else if (address == 0x4016) {
        // Controller port 1
        controller->Write(data);
    }
    else if (address >= 0x8000) {
        // PRG ROM is read-only; ignore writes
    }
    else {
        // Other memory regions
    }
}
