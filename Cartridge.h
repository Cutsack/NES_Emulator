// Cartridge.h
#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Cartridge {
public:
    enum Mirror {
        HORIZONTAL,
        VERTICAL,
        FOUR_SCREEN,
        SINGLE_SCREEN
    };

    Cartridge(const std::string& filename);
    bool Load();

    std::vector<uint8_t> PRG_ROM;
    std::vector<uint8_t> CHR_ROM;
    uint8_t mapperID;
    Mirror mirror;

private:
    std::string filename;
};