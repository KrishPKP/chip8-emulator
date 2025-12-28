//
// Created by patel on 2025-08-18.
//

#ifndef CHIP8_EMULATOR_CHIP8_HPP
#define CHIP8_EMULATOR_CHIP8_HPP

#include <cstdint>
#include <string>

class chip8 {

private:
    // Current opcode
    uint16_t opcode;

    // Memory (4K)
    uint8_t memory[4096];

    // CPU registers (V0–VF, VF is flag)
    uint8_t V[16];

    // Index register
    uint16_t I;

    // Program counter
    uint16_t pc;

    // Graphics (64 × 32 monochrome display)
    uint8_t gfx[64 * 32];
    bool drawFlag;

    // Timers
    uint8_t delay_timer;
    uint8_t sound_timer;

    // Stack (16 levels) and stack pointer
    uint16_t stack[16];
    uint16_t sp;

    // Keypad (HEX-based, 0x0–0xF)
    uint8_t key[16];

public:
    chip8();
    void setKey(uint8_t k, bool pressed) { key[k] = pressed; }


    void loadROM(const std::string& filename);
    void emulateCycle();

    // Display access
    bool shouldDraw() const { return drawFlag; }
    void resetDrawFlag() { drawFlag = false; }
    uint8_t* getDisplay() { return gfx; }


};

#endif // CHIP8_EMULATOR_CHIP8_HPP
