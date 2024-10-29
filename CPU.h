// CPU.h
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "Memory.h"
#include "PPU.h"

class CPU {
public:
    CPU(Memory* memory, PPU* ppu);
    void Reset();
    void ExecuteInstruction();
    void NMI();

    // Registers
    uint8_t  A;  // Accumulator
    uint8_t  X;  // X Register
    uint8_t  Y;  // Y Register
    uint8_t  SP; // Stack Pointer
    uint16_t PC; // Program Counter
    uint8_t  P;  // Status Register

    uint8_t cycles;

    bool running; // Flag to indicate if the CPU should continue executing

private:
    Memory* mem;
    PPU* ppu;

    // Helper methods
    uint8_t FetchByte();
    uint16_t FetchWord();
    void SetFlag(uint8_t flag, bool condition);
    bool GetFlag(uint8_t flag);

    // Status Flags
    enum StatusFlags {
        CARRY = (1 << 0),
        ZERO = (1 << 1),
        INTERRUPT = (1 << 2),
        DECIMAL = (1 << 3),
        BREAK_FLAG = (1 << 4),
        UNUSED = (1 << 5),
        OVERFLOW_FLAG = (1 << 6),
        NEGATIVE = (1 << 7)
    };

    // Opcode table
    struct Instruction {
        std::string name;
        uint8_t(CPU::* operate)(void);
        uint8_t(CPU::* addrmode)(void);
        uint8_t cycles;
    };
    std::vector<Instruction> lookup;

    // Opcode implementations
    uint8_t ADC();
    uint8_t AND();
    uint8_t ASL();
    uint8_t BCC();
    uint8_t BCS();
    uint8_t BEQ();
    uint8_t BIT();
    uint8_t BMI();
    uint8_t BNE();
    uint8_t BPL();
    uint8_t BRKInstruction();
    uint8_t BVC();
    uint8_t BVS();
    uint8_t CLC();
    uint8_t CLD();
    uint8_t CLI();
    uint8_t CLV();
    uint8_t CMP();
    uint8_t CPX();
    uint8_t CPY();
    uint8_t DEC();
    uint8_t DEX();
    uint8_t DEY();
    uint8_t EOR();
    uint8_t INC();
    uint8_t INX();
    uint8_t INY();
    uint8_t JMP();
    uint8_t JSR();
    uint8_t LDA();
    uint8_t LDX();
    uint8_t LDY();
    uint8_t LSR();
    uint8_t NOP();
    uint8_t ORA();
    uint8_t PHA();
    uint8_t PHP();
    uint8_t PLA();
    uint8_t PLP();
    uint8_t ROL();
    uint8_t ROR();
    uint8_t RTI();
    uint8_t RTS();
    uint8_t SBC();
    uint8_t SEC();
    uint8_t SED();
    uint8_t SEI();
    uint8_t STA();
    uint8_t STX();
    uint8_t STY();
    uint8_t TAX();
    uint8_t TAY();
    uint8_t TSX();
    uint8_t TXA();
    uint8_t TXS();
    uint8_t TYA();

    // Addressing modes
    uint8_t Implied();
    uint8_t Accumulator();
    uint8_t Immediate();
    uint8_t ZeroPage();
    uint8_t ZeroPageX();
    uint8_t ZeroPageY();
    uint8_t Relative();
    uint8_t Absolute();
    uint8_t AbsoluteX();
    uint8_t AbsoluteY();
    uint8_t Indirect();
    uint8_t IndirectX();
    uint8_t IndirectY();

    // Internal variables
    uint8_t opcode;
    uint8_t fetched;
    uint16_t addr_abs;
    uint16_t addr_rel;

    void InitializeOpcodeTable();
    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t data);
    uint8_t Fetch();
};
