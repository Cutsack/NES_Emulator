// CPU.cpp
#include "CPU.h"
#include <cstdio>
#include <cstring>

CPU::CPU(Memory* memory, PPU* ppu) : mem(memory), ppu(ppu), running(true) {
    InitializeOpcodeTable();
    Reset();
}

void CPU::Reset() {
    A = X = Y = 0;
    SP = 0xFD;
    P = 0x24;

    uint16_t low = Read(0xFFFC);
    uint16_t high = Read(0xFFFD);
    PC = (high << 8) | low;

    addr_abs = 0;
    addr_rel = 0;
    fetched = 0;

    cycles = 8;
}

void CPU::ExecuteInstruction() {
    if (!running) return;

    if (cycles == 0) {
        opcode = FetchByte();
        printf("PC: 0x%04X, Opcode: 0x%02X, A: 0x%02X, X: 0x%02X, Y: 0x%02X, SP: 0x%02X, P: 0x%02X\n",
            PC - 1, opcode, A, X, Y, SP, P);

        if (opcode >= lookup.size() || lookup[opcode].operate == nullptr) {
            printf("Unimplemented opcode: 0x%02X at PC: 0x%04X\n", opcode, PC - 1);
            running = false;
            return;
        }

        cycles = lookup[opcode].cycles;

        uint8_t additional_cycle1 = (this->*lookup[opcode].addrmode)();
        uint8_t additional_cycle2 = (this->*lookup[opcode].operate)();

        cycles += (additional_cycle1 & additional_cycle2);
    }

    cycles--;
}

void CPU::NMI() {
    Write(0x0100 + SP--, (PC >> 8) & 0xFF); // Push PC high byte
    Write(0x0100 + SP--, PC & 0xFF);        // Push PC low byte
    SetFlag(BREAK_FLAG, false);
    SetFlag(UNUSED, true);
    SetFlag(INTERRUPT, true);
    Write(0x0100 + SP--, P | UNUSED);       // Push P with U flag set
    uint16_t low = Read(0xFFFA);
    uint16_t high = Read(0xFFFB);
    PC = (high << 8) | low;                 // Set PC to NMI vector
    cycles = 7;
}


uint8_t CPU::FetchByte() {
    uint8_t data = Read(PC);
    PC++;
    return data;
}

uint16_t CPU::FetchWord() {
    uint8_t low = FetchByte();
    uint8_t high = FetchByte();
    return (high << 8) | low;
}

void CPU::SetFlag(uint8_t flag, bool condition) {
    if (condition)
        P |= flag;
    else
        P &= ~flag;
}

bool CPU::GetFlag(uint8_t flag) {
    return (P & flag) != 0;
}

void CPU::InitializeOpcodeTable() {
    lookup.resize(256);

    // Fill the opcode table with all instructions
    // For brevity, only a few examples are shown here. You need to fill in the rest.

    // ADC - Add with Carry
    lookup[0x69] = { "ADC", &CPU::ADC, &CPU::Immediate, 2 };
    lookup[0x65] = { "ADC", &CPU::ADC, &CPU::ZeroPage, 3 };
    lookup[0x75] = { "ADC", &CPU::ADC, &CPU::ZeroPageX, 4 };
    lookup[0x6D] = { "ADC", &CPU::ADC, &CPU::Absolute, 4 };
    lookup[0x7D] = { "ADC", &CPU::ADC, &CPU::AbsoluteX, 4 };
    lookup[0x79] = { "ADC", &CPU::ADC, &CPU::AbsoluteY, 4 };
    lookup[0x61] = { "ADC", &CPU::ADC, &CPU::IndirectX, 6 };
    lookup[0x71] = { "ADC", &CPU::ADC, &CPU::IndirectY, 5 };

    // AND - Logical AND
    lookup[0x29] = { "AND", &CPU::AND, &CPU::Immediate, 2 };
    lookup[0x25] = { "AND", &CPU::AND, &CPU::ZeroPage, 3 };
    lookup[0x35] = { "AND", &CPU::AND, &CPU::ZeroPageX, 4 };
    lookup[0x2D] = { "AND", &CPU::AND, &CPU::Absolute, 4 };
    lookup[0x3D] = { "AND", &CPU::AND, &CPU::AbsoluteX, 4 };
    lookup[0x39] = { "AND", &CPU::AND, &CPU::AbsoluteY, 4 };
    lookup[0x21] = { "AND", &CPU::AND, &CPU::IndirectX, 6 };
    lookup[0x31] = { "AND", &CPU::AND, &CPU::IndirectY, 5 };

    // ASL - Arithmetic Shift Left
    lookup[0x0A] = { "ASL", &CPU::ASL, &CPU::Accumulator, 2 };
    lookup[0x06] = { "ASL", &CPU::ASL, &CPU::ZeroPage, 5 };
    lookup[0x16] = { "ASL", &CPU::ASL, &CPU::ZeroPageX, 6 };
    lookup[0x0E] = { "ASL", &CPU::ASL, &CPU::Absolute, 6 };
    lookup[0x1E] = { "ASL", &CPU::ASL, &CPU::AbsoluteX, 7 };

    // BCC - Branch if Carry Clear
    lookup[0x90] = { "BCC", &CPU::BCC, &CPU::Relative, 2 };

    // LDA - it does something
    lookup[0xA9] = { "LDA", &CPU::LDA, &CPU::Immediate, 2 };
    lookup[0xA5] = { "LDA", &CPU::LDA, &CPU::ZeroPage, 3 };
    lookup[0xB5] = { "LDA", &CPU::LDA, &CPU::ZeroPageX, 4 };
    lookup[0xAD] = { "LDA", &CPU::LDA, &CPU::Absolute, 4 };
    lookup[0xBD] = { "LDA", &CPU::LDA, &CPU::AbsoluteX, 4 };
    lookup[0xB9] = { "LDA", &CPU::LDA, &CPU::AbsoluteY, 4 };
    lookup[0xA1] = { "LDA", &CPU::LDA, &CPU::IndirectX, 6 };
    lookup[0xB1] = { "LDA", &CPU::LDA, &CPU::IndirectY, 5 };

    // ... Continue filling in the rest of the opcodes ...

    // Implement NOP for unimplemented opcodes
    for (uint16_t i = 0; i < 256; i++) {
        if (lookup[i].name.empty()) {
            lookup[i] = { "NOP", &CPU::NOP, &CPU::Implied, 2 };
        }
    }
}

uint8_t CPU::Read(uint16_t address) {
    return mem->Read(address);
}

void CPU::Write(uint16_t address, uint8_t data) {
    mem->Write(address, data);
}

uint8_t CPU::Fetch() {
    if (!(lookup[opcode].addrmode == &CPU::Implied || lookup[opcode].addrmode == &CPU::Accumulator)) {
        fetched = Read(addr_abs);
    }
    else {
        fetched = A;
    }
    return fetched;
}

// Addressing Modes Implementations

uint8_t CPU::Implied() {
    fetched = A;
    return 0;
}

uint8_t CPU::Accumulator() {
    fetched = A;
    return 0;
}

uint8_t CPU::Immediate() {
    addr_abs = PC++;
    return 0;
}

uint8_t CPU::ZeroPage() {
    addr_abs = Read(PC++);
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t CPU::ZeroPageX() {
    addr_abs = (Read(PC++) + X) & 0x00FF;
    return 0;
}

uint8_t CPU::ZeroPageY() {
    addr_abs = (Read(PC++) + Y) & 0x00FF;
    return 0;
}

uint8_t CPU::Relative() {
    addr_rel = Read(PC++);
    if (addr_rel & 0x80) {
        addr_rel |= 0xFF00;
    }
    return 0;
}

uint8_t CPU::Absolute() {
    uint16_t low = Read(PC++);
    uint16_t high = Read(PC++);
    addr_abs = (high << 8) | low;
    return 0;
}

uint8_t CPU::AbsoluteX() {
    uint16_t low = Read(PC++);
    uint16_t high = Read(PC++);
    addr_abs = (high << 8) | low;
    addr_abs += X;

    if ((addr_abs & 0xFF00) != (high << 8)) {
        return 1;
    }
    else {
        return 0;
    }
}

uint8_t CPU::AbsoluteY() {
    uint16_t low = Read(PC++);
    uint16_t high = Read(PC++);
    addr_abs = (high << 8) | low;
    addr_abs += Y;

    if ((addr_abs & 0xFF00) != (high << 8)) {
        return 1;
    }
    else {
        return 0;
    }
}

uint8_t CPU::Indirect() {
    uint16_t ptr = Read(PC++);
    ptr |= (Read(PC++) << 8);

    if ((ptr & 0x00FF) == 0x00FF) {
        // Simulate page boundary hardware bug
        addr_abs = (Read(ptr & 0xFF00) << 8) | Read(ptr);
    }
    else {
        addr_abs = (Read(ptr + 1) << 8) | Read(ptr);
    }
    return 0;
}

uint8_t CPU::IndirectX() {
    uint8_t zp_addr = Read(PC++);
    uint8_t ptr = (zp_addr + X) & 0x00FF;
    uint8_t low = Read(ptr & 0x00FF);
    uint8_t high = Read((ptr + 1) & 0x00FF);
    addr_abs = (high << 8) | low;
    return 0;
}

uint8_t CPU::IndirectY() {
    uint8_t zp_addr = Read(PC++);
    uint8_t low = Read(zp_addr & 0x00FF);
    uint8_t high = Read((zp_addr + 1) & 0x00FF);
    addr_abs = (high << 8) | low;
    addr_abs += Y;

    if ((addr_abs & 0xFF00) != (high << 8)) {
        return 1;
    }
    else {
        return 0;
    }
}

// Instruction Implementations

uint8_t CPU::ADC() {
    Fetch();
    uint16_t temp = (uint16_t)A + (uint16_t)fetched + (uint16_t)(GetFlag(CARRY) ? 1 : 0);
    SetFlag(CARRY, temp > 255);
    SetFlag(ZERO, (temp & 0x00FF) == 0);
    SetFlag(OVERFLOW_FLAG, (~((uint16_t)A ^ (uint16_t)fetched) & ((uint16_t)A ^ temp)) & 0x0080);
    SetFlag(NEGATIVE, temp & 0x80);
    A = temp & 0x00FF;
    return 1;
}

uint8_t CPU::AND() {
    Fetch();
    A = A & fetched;
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 1;
}

uint8_t CPU::ASL() {
    Fetch();
    uint16_t temp = (uint16_t)fetched << 1;
    SetFlag(CARRY, (temp & 0xFF00) > 0);
    SetFlag(ZERO, (temp & 0x00FF) == 0x00);
    SetFlag(NEGATIVE, temp & 0x80);
    if (lookup[opcode].addrmode == &CPU::Accumulator) {
        A = temp & 0x00FF;
    }
    else {
        Write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

uint8_t CPU::BCC() {
    if (!GetFlag(CARRY)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BCS() {
    if (GetFlag(CARRY)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BEQ() {
    if (GetFlag(ZERO)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BIT() {
    Fetch();
    uint8_t temp = A & fetched;
    SetFlag(ZERO, (temp & 0x00FF) == 0x00);
    SetFlag(NEGATIVE, fetched & (1 << 7));
    SetFlag(OVERFLOW_FLAG, fetched & (1 << 6));
    return 0;
}

uint8_t CPU::BMI() {
    if (GetFlag(NEGATIVE)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BNE() {
    if (!GetFlag(ZERO)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BPL() {
    if (!GetFlag(NEGATIVE)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BRKInstruction() {
    PC++;
    Write(0x0100 + SP--, (PC >> 8) & 0xFF); // Push PC high byte
    Write(0x0100 + SP--, PC & 0xFF);        // Push PC low byte
    SetFlag(BREAK_FLAG, true);
    Write(0x0100 + SP--, P | BREAK_FLAG | UNUSED); // Push P with B and U flags set
    SetFlag(INTERRUPT, true);
    SetFlag(BREAK_FLAG, false);
    PC = Read(0xFFFE) | (Read(0xFFFF) << 8); // Set PC to IRQ vector
    cycles = 7;
    return 0;
}


uint8_t CPU::BVC() {
    if (!GetFlag(OVERFLOW_FLAG)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::BVS() {
    if (GetFlag(OVERFLOW_FLAG)) {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) {
            cycles++;
        }
        PC = addr_abs;
    }
    return 0;
}

uint8_t CPU::CLC() {
    SetFlag(CARRY, false);
    return 0;
}

uint8_t CPU::CLD() {
    SetFlag(DECIMAL, false);
    return 0;
}

uint8_t CPU::CLI() {
    SetFlag(INTERRUPT, false);
    return 0;
}

uint8_t CPU::CLV() {
    SetFlag(OVERFLOW_FLAG, false);
    return 0;
}

uint8_t CPU::CMP() {
    Fetch();
    uint16_t temp = (uint16_t)A - (uint16_t)fetched;
    SetFlag(CARRY, A >= fetched);
    SetFlag(ZERO, (temp & 0x00FF) == 0x0000);
    SetFlag(NEGATIVE, temp & 0x0080);
    return 1;
}

uint8_t CPU::CPX() {
    Fetch();
    uint16_t temp = (uint16_t)X - (uint16_t)fetched;
    SetFlag(CARRY, X >= fetched);
    SetFlag(ZERO, (temp & 0x00FF) == 0x0000);
    SetFlag(NEGATIVE, temp & 0x0080);
    return 0;
}

uint8_t CPU::CPY() {
    Fetch();
    uint16_t temp = (uint16_t)Y - (uint16_t)fetched;
    SetFlag(CARRY, Y >= fetched);
    SetFlag(ZERO, (temp & 0x00FF) == 0x0000);
    SetFlag(NEGATIVE, temp & 0x0080);
    return 0;
}

uint8_t CPU::DEC() {
    Fetch();
    uint8_t temp = fetched - 1;
    Write(addr_abs, temp);
    SetFlag(ZERO, temp == 0x00);
    SetFlag(NEGATIVE, temp & 0x80);
    return 0;
}

uint8_t CPU::DEX() {
    X--;
    SetFlag(ZERO, X == 0x00);
    SetFlag(NEGATIVE, X & 0x80);
    return 0;
}

uint8_t CPU::DEY() {
    Y--;
    SetFlag(ZERO, Y == 0x00);
    SetFlag(NEGATIVE, Y & 0x80);
    return 0;
}

uint8_t CPU::EOR() {
    Fetch();
    A = A ^ fetched;
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 1;
}

uint8_t CPU::INC() {
    Fetch();
    uint8_t temp = fetched + 1;
    Write(addr_abs, temp);
    SetFlag(ZERO, temp == 0x00);
    SetFlag(NEGATIVE, temp & 0x80);
    return 0;
}

uint8_t CPU::INX() {
    X++;
    SetFlag(ZERO, X == 0x00);
    SetFlag(NEGATIVE, X & 0x80);
    return 0;
}

uint8_t CPU::INY() {
    Y++;
    SetFlag(ZERO, Y == 0x00);
    SetFlag(NEGATIVE, Y & 0x80);
    return 0;
}

uint8_t CPU::JMP() {
    PC = addr_abs;
    return 0;
}

uint8_t CPU::JSR() {
    PC--;

    Write(0x0100 + SP--, (PC >> 8) & 0xFF);
    Write(0x0100 + SP--, PC & 0xFF);

    PC = addr_abs;
    return 0;
}

uint8_t CPU::LDA() {
    Fetch();
    A = fetched;
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 1;
}

uint8_t CPU::LDX() {
    Fetch();
    X = fetched;
    SetFlag(ZERO, X == 0x00);
    SetFlag(NEGATIVE, X & 0x80);
    return 1;
}

uint8_t CPU::LDY() {
    Fetch();
    Y = fetched;
    SetFlag(ZERO, Y == 0x00);
    SetFlag(NEGATIVE, Y & 0x80);
    return 1;
}

uint8_t CPU::LSR() {
    Fetch();
    SetFlag(CARRY, fetched & 0x01);
    uint8_t temp = fetched >> 1;
    SetFlag(ZERO, temp == 0x00);
    SetFlag(NEGATIVE, temp & 0x80);
    if (lookup[opcode].addrmode == &CPU::Accumulator) {
        A = temp;
    }
    else {
        Write(addr_abs, temp);
    }
    return 0;
}

uint8_t CPU::NOP() {
    // NOP does nothing
    return 0;
}

uint8_t CPU::ORA() {
    Fetch();
    A = A | fetched;
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 1;
}

uint8_t CPU::PHA() {
    Write(0x0100 + SP--, A);
    return 0;
}

uint8_t CPU::PHP() {
    Write(0x0100 + SP--, P | BREAK_FLAG | UNUSED);
    return 0;
}

uint8_t CPU::PLA() {
    SP++;
    A = Read(0x0100 + SP);
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 0;
}

uint8_t CPU::PLP() {
    SP++;
    P = Read(0x0100 + SP);
    SetFlag(UNUSED, true);
    return 0;
}

uint8_t CPU::ROL() {
    Fetch();
    uint16_t temp = (uint16_t)(fetched << 1) | (GetFlag(CARRY) ? 1 : 0);
    SetFlag(CARRY, temp & 0xFF00);
    SetFlag(ZERO, (temp & 0x00FF) == 0x00);
    SetFlag(NEGATIVE, temp & 0x80);
    if (lookup[opcode].addrmode == &CPU::Accumulator) {
        A = temp & 0x00FF;
    }
    else {
        Write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

uint8_t CPU::ROR() {
    Fetch();
    uint16_t temp = (uint16_t)(GetFlag(CARRY) ? 0x80 : 0x00) | (fetched >> 1);
    SetFlag(CARRY, fetched & 0x01);
    SetFlag(ZERO, (temp & 0x00FF) == 0x00);
    SetFlag(NEGATIVE, temp & 0x80);
    if (lookup[opcode].addrmode == &CPU::Accumulator) {
        A = temp & 0x00FF;
    }
    else {
        Write(addr_abs, temp & 0x00FF);
    }
    return 0;
}

uint8_t CPU::RTI() {
    SP++;
    P = Read(0x0100 + SP);
    P &= ~BREAK_FLAG;
    P &= ~UNUSED;

    SP++;
    uint16_t low = Read(0x0100 + SP);
    SP++;
    uint16_t high = Read(0x0100 + SP);
    PC = (high << 8) | low;
    return 0;
}

uint8_t CPU::RTS() {
    SP++;
    uint16_t low = Read(0x0100 + SP);
    SP++;
    uint16_t high = Read(0x0100 + SP);
    PC = (high << 8) | low;
    PC++;
    return 0;
}

uint8_t CPU::SBC() {
    Fetch();
    uint16_t value = ((uint16_t)fetched) ^ 0x00FF;
    uint16_t temp = (uint16_t)A + value + (uint16_t)(GetFlag(CARRY) ? 1 : 0);
    SetFlag(CARRY, temp & 0xFF00);
    SetFlag(ZERO, ((temp & 0x00FF) == 0));
    SetFlag(OVERFLOW_FLAG, (temp ^ (uint16_t)A) & (temp ^ value) & 0x0080);
    SetFlag(NEGATIVE, temp & 0x80);
    A = temp & 0x00FF;
    return 1;
}

uint8_t CPU::SEC() {
    SetFlag(CARRY, true);
    return 0;
}

uint8_t CPU::SED() {
    SetFlag(DECIMAL, true);
    return 0;
}

uint8_t CPU::SEI() {
    SetFlag(INTERRUPT, true);
    return 0;
}

uint8_t CPU::STA() {
    Write(addr_abs, A);
    return 0;
}

uint8_t CPU::STX() {
    Write(addr_abs, X);
    return 0;
}

uint8_t CPU::STY() {
    Write(addr_abs, Y);
    return 0;
}

uint8_t CPU::TAX() {
    X = A;
    SetFlag(ZERO, X == 0x00);
    SetFlag(NEGATIVE, X & 0x80);
    return 0;
}

uint8_t CPU::TAY() {
    Y = A;
    SetFlag(ZERO, Y == 0x00);
    SetFlag(NEGATIVE, Y & 0x80);
    return 0;
}

uint8_t CPU::TSX() {
    X = SP;
    SetFlag(ZERO, X == 0x00);
    SetFlag(NEGATIVE, X & 0x80);
    return 0;
}

uint8_t CPU::TXA() {
    A = X;
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 0;
}

uint8_t CPU::TXS() {
    SP = X;
    return 0;
}

uint8_t CPU::TYA() {
    A = Y;
    SetFlag(ZERO, A == 0x00);
    SetFlag(NEGATIVE, A & 0x80);
    return 0;
}
