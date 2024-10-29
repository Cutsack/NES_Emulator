// Cartridge.cpp
#include "Cartridge.h"
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& filename) : filename(filename), mirror(HORIZONTAL) {}

bool Cartridge::Load() {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Could not open ROM file: " << filename << std::endl;
        return false;
    }

    uint8_t header[16];
    file.read(reinterpret_cast<char*>(header), 16);

    // Verify NES file format
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
        std::cout << "Invalid NES ROM file." << std::endl;
        return false;
    }

    uint8_t prgSize = header[4];
    uint8_t chrSize = header[5];
    mapperID = ((header[6] >> 4) & 0x0F) | (header[7] & 0xF0);

    // Set mirroring type
    if (header[6] & 0x01) {
        mirror = VERTICAL;
    }
    else {
        mirror = HORIZONTAL;
    }

    // Skip trainer if present
    if (header[6] & 0x04) {
        file.seekg(512, std::ios::cur);
    }

    // Load PRG ROM
    PRG_ROM.resize(prgSize * 16384);
    file.read(reinterpret_cast<char*>(PRG_ROM.data()), PRG_ROM.size());

    // Load CHR ROM
    if (chrSize == 0) {
        // Some games use CHR RAM instead of ROM
        CHR_ROM.resize(8192); // 8KB of CHR RAM
    }
    else {
        CHR_ROM.resize(chrSize * 8192);
        file.read(reinterpret_cast<char*>(CHR_ROM.data()), CHR_ROM.size());
    }

    file.close();
    std::cout << "Loaded ROM: " << filename << std::endl;
    std::cout << "Mapper ID: " << (int)mapperID << std::endl;
    std::cout << "PRG ROM Size: " << PRG_ROM.size() << " bytes" << std::endl;
    std::cout << "CHR ROM Size: " << CHR_ROM.size() << " bytes" << std::endl;
    std::cout << "Mirroring Type: " << (mirror == VERTICAL ? "Vertical" : "Horizontal") << std::endl;

    if (mapperID != 0) {
        std::cout << "Unsupported Mapper ID: " << (int)mapperID << std::endl;
        return false;
    }

    return true;
}
