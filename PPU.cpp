// PPU.cpp
#include "PPU.h"
#include <cstring>

// NES color palette (simplified)
static const uint32_t nesPalette[64] = {
    0xFF757575, 0xFF271B8F, 0xFF0000AB, 0xFF47009F, 0xFF8F0077, 0xFFAB0013, 0xFFA70000, 0xFF7F0B00,
    0xFF432F00, 0xFF004700, 0xFF005100, 0xFF003F17, 0xFF1B3F5F, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFBCBCBC, 0xFF0073EF, 0xFF233BEF, 0xFF8300F3, 0xFFBF00BF, 0xFFE7005B, 0xFFDB2B00, 0xFFCB4F0F,
    0xFF8B7300, 0xFF009700, 0xFF00AB00, 0xFF00933B, 0xFF00838B, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFF3FBFFF, 0xFF5F97FF, 0xFFA78BFD, 0xFFF77BFF, 0xFFFF77B7, 0xFFFF7763, 0xFFFF9B3B,
    0xFFF3BF3F, 0xFF83D313, 0xFF4FDF4B, 0xFF58F898, 0xFF00EBDB, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFABE7FF, 0xFFC7D7FF, 0xFFD7CBFF, 0xFFFFC7FF, 0xFFFFC7DB, 0xFFFFBFB3, 0xFFFFDBAB,
    0xFFFFE7A3, 0xFFE3FFA3, 0xFFABF3BF, 0xFFB3FFCF, 0xFF9FFFF3, 0xFF000000, 0xFF000000, 0xFF000000
};

PPU::PPU(Cartridge* cart)
    : cartridge(cart), frameComplete(false), scanline(0), cycle(0), nmi(false) {
    Reset();
}

void PPU::Reset() {
    std::memset(nameTable, 0, sizeof(nameTable));
    std::memset(palette, 0, sizeof(palette));
    std::memset(OAM, 0, sizeof(OAM));
    std::memset(frameBuffer, 0, sizeof(frameBuffer));
    std::memset(chrRam, 0, sizeof(chrRam)); // Initialize CHR RAM if needed

    vramAddr = 0;
    tempAddr = 0;
    fineX = 0;
    writeToggle = 0;
    ppuDataBuffer = 0;
    regControl = 0;
    regMask = 0;
    regStatus = 0;
    regOAMAddr = 0;

    bgNextTileID = 0;
    bgNextTileAttrib = 0;
    bgNextTileLsb = 0;
    bgNextTileMsb = 0;
    bgShiftPatternLow = 0;
    bgShiftPatternHigh = 0;
    bgShiftAttribLow = 0;
    bgShiftAttribHigh = 0;
}

void PPU::Clock() {
    // Set NMI flag at the start of VBlank
    if (scanline == 241 && cycle == 1) {
        regStatus |= 0x80; // Set VBlank flag
        if (regControl & 0x80) {
            nmi = true;
        }
    }

    // Clear VBlank flag at the end of VBlank
    if (scanline == -1 && cycle == 1) {
        regStatus &= ~0x80; // Clear VBlank flag
        frameComplete = false;
    }

    // Visible scanlines (0-239)
    if (scanline >= 0 && scanline < 240) {
        if ((cycle >= 2 && cycle <= 257) || (cycle >= 321 && cycle <= 337)) {
            UpdateShifters();

            switch ((cycle - 1) % 8) {
            case 0:
                LoadBackgroundShifters();
                FetchBackgroundTile();
                break;
            case 2:
                FetchBackgroundTileAttrib();
                break;
            case 4:
                FetchBackgroundTileLsb();
                break;
            case 6:
                FetchBackgroundTileMsb();
                break;
            case 7:
                IncrementScrollX();
                break;
            }
        }

        if (cycle == 256) {
            IncrementScrollY();
        }

        if (cycle == 257) {
            LoadBackgroundShifters();
            TransferAddressX();
        }

        if (scanline == -1 && cycle >= 280 && cycle <= 304) {
            TransferAddressY();
        }

        // Render pixel
        if (cycle >= 1 && cycle <= 256) {
            RenderPixel();
        }
    }

    // Increment cycle and scanline
    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline = -1;
            frameComplete = true;
        }
    }
}

// CPU Interface
uint8_t PPU::CPURead(uint16_t addr) {
    uint8_t data = 0x00;
    addr &= 0x2007;

    switch (addr) {
    case 0x2002: // PPUSTATUS
        data = regStatus;
        regStatus &= ~0x80; // Clear VBlank flag
        writeToggle = 0;
        break;
    case 0x2004: // OAMDATA
        data = OAM[regOAMAddr];
        break;
    case 0x2007: // PPUDATA
        data = ppuDataBuffer;
        ppuDataBuffer = PPURead(vramAddr);

        // If reading palette data, return data immediately
        if (vramAddr >= 0x3F00) {
            data = ppuDataBuffer;
        }

        vramAddr += (regControl & 0x04) ? 32 : 1;
        break;
    default:
        // Other registers are not readable
        break;
    }

    return data;
}

void PPU::CPUWrite(uint16_t addr, uint8_t data) {
    addr &= 0x2007;

    switch (addr) {
    case 0x2000: // PPUCTRL
        regControl = data;
        tempAddr = (tempAddr & 0xF3FF) | ((data & 0x03) << 10);
        break;
    case 0x2001: // PPUMASK
        regMask = data;
        break;
    case 0x2003: // OAMADDR
        regOAMAddr = data;
        break;
    case 0x2004: // OAMDATA
        OAM[regOAMAddr++] = data;
        break;
    case 0x2005: // PPUSCROLL
        if (writeToggle == 0) {
            fineX = data & 0x07;
            tempAddr = (tempAddr & 0xFFE0) | (data >> 3);
            writeToggle = 1;
        }
        else {
            tempAddr = (tempAddr & 0x8FFF) | ((data & 0x07) << 12);
            tempAddr = (tempAddr & 0xFC1F) | ((data & 0xF8) << 2);
            writeToggle = 0;
        }
        break;
    case 0x2006: // PPUADDR
        if (writeToggle == 0) {
            tempAddr = (tempAddr & 0x00FF) | ((data & 0x3F) << 8);
            writeToggle = 1;
        }
        else {
            tempAddr = (tempAddr & 0xFF00) | data;
            vramAddr = tempAddr;
            writeToggle = 0;
        }
        break;
    case 0x2007: // PPUDATA
        PPUWrite(vramAddr, data);
        vramAddr += (regControl & 0x04) ? 32 : 1;
        break;
    default:
        // Other registers are not writable
        break;
    }
}

// PPU Memory Interface
uint8_t PPU::PPURead(uint16_t addr) {
    addr &= 0x3FFF;
    uint8_t data = 0x00;

    if (addr < 0x2000) {
        // Pattern tables (CHR ROM or CHR RAM)
        if (cartridge->CHR_ROM.size() == 0) {
            // CHR RAM
            data = chrRam[addr];
        }
        else {
            // CHR ROM
            data = cartridge->CHR_ROM[addr];
        }
    }
    else if (addr >= 0x2000 && addr < 0x3F00) {
        // Name tables with mirroring
        addr &= 0x0FFF;
        uint16_t mirroredAddr = MirrorAddress(addr);
        data = nameTable[(mirroredAddr / 0x0400) % 2][mirroredAddr % 0x0400];
    }
    else if (addr >= 0x3F00 && addr < 0x4000) {
        // Palette RAM indexes
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        data = palette[addr];
    }

    return data;
}

void PPU::PPUWrite(uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        // Pattern tables (CHR RAM)
        if (cartridge->CHR_ROM.size() == 0) {
            // CHR RAM
            chrRam[addr] = data;
        }
        // For CHR ROM, writes are ignored
    }
    else if (addr >= 0x2000 && addr < 0x3F00) {
        // Name tables with mirroring
        addr &= 0x0FFF;
        uint16_t mirroredAddr = MirrorAddress(addr);
        nameTable[(mirroredAddr / 0x0400) % 2][mirroredAddr % 0x0400] = data;
    }
    else if (addr >= 0x3F00 && addr < 0x4000) {
        // Palette RAM indexes
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        palette[addr] = data;
    }
}

// Rendering
bool PPU::FrameReady() {
    return frameComplete;
}

uint32_t* PPU::GetFrameBuffer() {
    return frameBuffer;
}

uint8_t PPU::GetColorFromPaletteRAM(uint8_t paletteNum, uint8_t pixel) {
    if (pixel == 0) {
        return PPURead(0x3F00) & 0x3F;
    }
    else {
        return PPURead(0x3F00 + (paletteNum << 2) + pixel) & 0x3F;
    }
}

void PPU::SetPixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < 256 && y >= 0 && y < 240) {
        frameBuffer[y * 256 + x] = nesPalette[color];
    }
}

// Background Rendering Helper Functions
void PPU::FetchBackgroundTile() {
    bgNextTileID = PPURead(0x2000 | (vramAddr & 0x0FFF));
}

void PPU::FetchBackgroundTileAttrib() {
    uint16_t attribAddr = 0x23C0 | (vramAddr & 0x0C00) | ((vramAddr >> 4) & 0x38) | ((vramAddr >> 2) & 0x07);
    uint8_t attrib = PPURead(attribAddr);
    if ((vramAddr & 0x0040) != 0) attrib >>= 4;
    if ((vramAddr & 0x0002) != 0) attrib >>= 2;
    bgNextTileAttrib = attrib & 0x03;
}

void PPU::FetchBackgroundTileLsb() {
    uint16_t baseAddr = (regControl & 0x10) << 8;
    uint16_t tileAddr = baseAddr + (bgNextTileID << 4) + FineY();
    bgNextTileLsb = PPURead(tileAddr);
}

void PPU::FetchBackgroundTileMsb() {
    uint16_t baseAddr = (regControl & 0x10) << 8;
    uint16_t tileAddr = baseAddr + (bgNextTileID << 4) + FineY() + 8;
    bgNextTileMsb = PPURead(tileAddr);
}

void PPU::LoadBackgroundShifters() {
    bgShiftPatternLow = (bgShiftPatternLow & 0xFF00) | bgNextTileLsb;
    bgShiftPatternHigh = (bgShiftPatternHigh & 0xFF00) | bgNextTileMsb;

    uint8_t attrib = bgNextTileAttrib;
    bgShiftAttribLow = (bgShiftAttribLow & 0xFF00) | ((attrib & 0x01) ? 0xFF : 0x00);
    bgShiftAttribHigh = (bgShiftAttribHigh & 0xFF00) | ((attrib & 0x02) ? 0xFF : 0x00);
}

void PPU::UpdateShifters() {
    if (regMask & 0x08) { // Background rendering enabled
        bgShiftPatternLow <<= 1;
        bgShiftPatternHigh <<= 1;
        bgShiftAttribLow <<= 1;
        bgShiftAttribHigh <<= 1;
    }
}

void PPU::RenderPixel() {
    int x = cycle - 1;
    int y = scanline;

    uint8_t bg_pixel = 0;
    uint8_t bg_palette = 0;

    if (regMask & 0x08) { // Background rendering enabled
        uint16_t bitMux = 0x8000 >> fineX;

        uint8_t p0_pixel = (bgShiftPatternLow & bitMux) > 0;
        uint8_t p1_pixel = (bgShiftPatternHigh & bitMux) > 0;
        bg_pixel = (p1_pixel << 1) | p0_pixel;

        uint8_t attrib0 = (bgShiftAttribLow & bitMux) > 0;
        uint8_t attrib1 = (bgShiftAttribHigh & bitMux) > 0;
        bg_palette = (attrib1 << 1) | attrib0;
    }

    uint8_t color = GetColorFromPaletteRAM(bg_palette, bg_pixel);

    SetPixel(x, y, color);
}

void PPU::IncrementScrollX() {
    if ((regMask & 0x08) || (regMask & 0x10)) {
        if ((vramAddr & 0x001F) == 31) {
            vramAddr &= ~0x001F;
            vramAddr ^= 0x0400;
        }
        else {
            vramAddr++;
        }
    }
}

void PPU::IncrementScrollY() {
    if ((regMask & 0x08) || (regMask & 0x10)) {
        if ((vramAddr & 0x7000) != 0x7000) {
            vramAddr += 0x1000;
        }
        else {
            vramAddr &= ~0x7000;
            uint16_t y = (vramAddr & 0x03E0) >> 5;
            if (y == 29) {
                y = 0;
                vramAddr ^= 0x0800;
            }
            else if (y == 31) {
                y = 0;
            }
            else {
                y++;
            }
            vramAddr = (vramAddr & ~0x03E0) | (y << 5);
        }
    }
}

void PPU::TransferAddressX() {
    if ((regMask & 0x08) || (regMask & 0x10)) {
        vramAddr = (vramAddr & 0xFBE0) | (tempAddr & 0x041F);
    }
}

void PPU::TransferAddressY() {
    if ((regMask & 0x08) || (regMask & 0x10)) {
        vramAddr = (vramAddr & 0x841F) | (tempAddr & 0x7BE0);
    }
}

uint8_t PPU::FineY() {
    return (vramAddr >> 12) & 0x07;
}

uint16_t PPU::MirrorAddress(uint16_t addr) {
    if (cartridge->mirror == Cartridge::VERTICAL) {
        return addr;
    }
    else if (cartridge->mirror == Cartridge::HORIZONTAL) {
        if (addr >= 0x0000 && addr < 0x0400)
            return addr;
        else if (addr >= 0x0400 && addr < 0x0800)
            return addr - 0x0400;
        else if (addr >= 0x0800 && addr < 0x0C00)
            return addr - 0x0800;
        else
            return addr - 0x0C00;
    }
    else {
        // Handle other mirroring types if necessary
        return addr;
    }
}
