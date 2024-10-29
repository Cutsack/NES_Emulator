// PPU.h
#pragma once
#include <cstdint>
#include "Cartridge.h"

class PPU {
public:
    PPU(Cartridge* cart);
    void Reset();
    void Clock();

    // CPU Interface
    uint8_t CPURead(uint16_t addr);
    void CPUWrite(uint16_t addr, uint8_t data);

    // PPU Memory Interface
    uint8_t PPURead(uint16_t addr);
    void PPUWrite(uint16_t addr, uint8_t data);

    // Rendering
    bool FrameReady();
    uint32_t* GetFrameBuffer();

    bool nmi;

    // OAM for DMA access
    uint8_t OAM[256];
    uint8_t regOAMAddr;

private:
    Cartridge* cartridge;

    // PPU Memory
    uint8_t nameTable[2][1024]; // Two name tables for mirroring
    uint8_t palette[32];        // Palette RAM

    // Internal Registers
    uint16_t vramAddr;    // Current VRAM address (15 bits)
    uint16_t tempAddr;    // Temporary VRAM address
    uint8_t fineX;        // Fine X scroll (3 bits)
    uint8_t writeToggle;  // First or second write toggle
    uint8_t ppuDataBuffer; // Buffer for PPUDATA reads

    // PPU Registers
    uint8_t regControl;
    uint8_t regMask;
    uint8_t regStatus;

    // Rendering
    uint32_t frameBuffer[256 * 240];
    int scanline;
    int cycle;
    bool frameComplete;

    // Background rendering variables
    uint8_t bgNextTileID;
    uint8_t bgNextTileAttrib;
    uint8_t bgNextTileLsb;
    uint8_t bgNextTileMsb;
    uint16_t bgShiftPatternLow;
    uint16_t bgShiftPatternHigh;
    uint16_t bgShiftAttribLow;
    uint16_t bgShiftAttribHigh;

    // Methods
    uint8_t GetColorFromPaletteRAM(uint8_t paletteNum, uint8_t pixel);
    void SetPixel(int x, int y, uint8_t color);

    void LoadBackgroundShifters();
    void UpdateShifters();
    void IncrementScrollX();
    void IncrementScrollY();
    void TransferAddressX();
    void TransferAddressY();

    uint8_t FineY();

    void FetchBackgroundData();

    uint8_t chrRam[8192]; // 8KB of CHR RAM

    // Helper functions for rendering
    void FetchBackgroundTile();
    void FetchBackgroundTileAttrib();
    void FetchBackgroundTileLsb();
    void FetchBackgroundTileMsb();
    void RenderPixel();

    // Mirroring helper
    uint16_t MirrorAddress(uint16_t addr);
};
